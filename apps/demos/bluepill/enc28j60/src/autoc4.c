/*
 * Copyright (c) 2014 by Philip Matura <ike@tura-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <string.h>

#include "stm32f1xx.h"

#include "mqtt.h"
#include "autoc4.h"

#define AUTOC4_CONFIG_FILE "plenarsaal.c"
#include AUTOC4_CONFIG_FILE

/* #include "services/dmx-storage/dmx_storage.h" */
/* #include "hardware/ws2812b/ws2812b.h" */


static void autoc4_connack_callback(void);
static void autoc4_poll(void);
static void autoc4_publish_callback(char const *topic, uint16_t topic_length,
    const void *payload, uint16_t payload_length, bool retained);

static void autoc4_emergency_switch_toggled(void);
static void autoc4_read_inputs(void);
static void autoc4_init_input_states(void);
static void autoc4_set_output(uint8_t index, uint8_t state);
static uint8_t autoc4_get_output(uint8_t index);

static void autoc4_init_fan_control(void);
static void autoc4_init_status_light(void);
static void autoc4_update_status_light(void);


static bool logicer_state;
static bool locked_mode;

static const autoc4_config_t *autoc4_config = &config;

typedef struct
{
  bool prev_state;
  bool mqtt_dirty; // whether there is a new state to be published
} autoc4_input_state_t;
static autoc4_input_state_t pin_input_states[sizeof(outputs) / sizeof(autoc4_output_config)];
typedef struct
{
  uint8_t value;
  uint8_t timer;
} autoc4_output_state_t;
static autoc4_output_state_t output_states[sizeof(inputs) / sizeof(autoc4_input_config)];


static const uint8_t blink_timeouts[4] = { 1, 5, 25, 30 }; // in 100 ms
static const uint8_t zero_one[2] = { 0, 1 };


static void autoc4_emergency_switch_toggled(void)
{
  /* // zero dmx */
  /* for (uint16_t channel=0; channel<DMX_STORAGE_CHANNELS; channel++) */
  /*   set_dmx_channel(AUTOC4_DMX_UNIVERSE, channel, 0); */

  // check if any outputs that should be toggled are active
  uint8_t any_output_on = 0;
  for (uint8_t i=0; i<autoc4_config->output_count; i++)
    if (autoc4_config->output_configs[i].emergency_toggled && autoc4_get_output(i))
    {
      any_output_on = 1;
      break;
    }

  // toggle/zero outputs, depending on configuration
  for (uint8_t i=0; i<autoc4_config->output_count; i++)
  {
    if (autoc4_config->output_configs[i].emergency_toggled)
      autoc4_set_output(i, !any_output_on);
    else if (autoc4_config->output_configs[i].emergency_zeroed)
      autoc4_set_output(i, 0);
  }
}


static void autoc4_connack_callback(void)
{
  // mark every input as dirty, queue for mqtt resending
  for (int i=0; i<autoc4_config->input_count; i++)
    pin_input_states[i].mqtt_dirty = true;

  // send heartbeat message
  mqtt_construct_publish_packet(AUTOC4_HEARTBEAT_TOPIC, zero_one + 1, 1, true);

  logicer_state = false;
}

static void autoc4_poll(void)
{
  for (int i=0; i<autoc4_config->input_count; i++)
    if (pin_input_states[i].mqtt_dirty)
    {
      const uint8_t *data;

      if (pin_input_states[i].prev_state)
        data = &zero_one[0];
      else
        data = &zero_one[1];

      if (mqtt_construct_publish_packet(autoc4_config->input_configs[i].topic, data, 1, true))
      {
        // publish successful
        pin_input_states[i].mqtt_dirty = false;
      }
      else
      {
        // the mqtt buffer might be full, cancel sending more publishes
        return;
      }
    }
}

static void autoc4_set_output(uint8_t index, uint8_t state)
{
  GPIO_PinState t = state ? GPIO_PIN_SET : GPIO_PIN_RESET;
  HAL_GPIO_WritePin(autoc4_config->output_configs[index].gpio_port, autoc4_config->output_configs[index].gpio_pin, t);
}
static uint8_t autoc4_get_output(uint8_t index)
{
  return HAL_GPIO_ReadPin(autoc4_config->output_configs[index].gpio_port, autoc4_config->output_configs[index].gpio_pin) == GPIO_PIN_SET;
}
static uint8_t autoc4_get_input(uint8_t index)
{
  bool t = HAL_GPIO_ReadPin(autoc4_config->input_configs[index].gpio_port, autoc4_config->input_configs[index].gpio_pin) == GPIO_PIN_SET;
  if (autoc4_config->input_configs[index].inverted)
    t = !t;
  return t;
}


#define AUTOC4_SUDO_PREFIX "sudo/"

static void autoc4_publish_callback(char const *topic,
    uint16_t topic_length, const void *payload, uint16_t payload_length, bool retained)
{
  if (topic_length >= sizeof(AUTOC4_SUDO_PREFIX) && strncmp(topic, AUTOC4_SUDO_PREFIX, sizeof(AUTOC4_SUDO_PREFIX)-1) == 0)
  {
    // sudo message, ignore locked mode
    topic += sizeof(AUTOC4_SUDO_PREFIX) - 1;
    // fall through
  }
  else if (locked_mode)
  {
    // non-sudo message, ignore
    return;
  }

  if (payload_length > 0)
  {
    // Set digital outputs
    for (int i=0; i<autoc4_config->output_count; i++)
      if (strncmp(topic, autoc4_config->output_configs[i].topic, topic_length) == 0)
      {
        // save value for blinking outputs
        output_states[i].value = ((uint8_t*)payload)[0];
        output_states[i].timer = 0;
        autoc4_set_output(i, ((uint8_t*)payload)[0]);
        return;
      }

    /* // Set raw DMX channels */
    /* if (strncmp(topic, autoc4_config->dmx_topic, topic_length) == 0) */
    /* { */
    /*   set_dmx_channels(payload, AUTOC4_DMX_UNIVERSE, 0, payload_length); */
    /*   return; */
    /* } */
    /*  */
    /* // Set individual DMX devices */
    /* for (int i=0; i<autoc4_config->dmx_count; i++) */
    /*   if (strncmp(topic, autoc4_config->dmx_configs[i].topic, topic_length) == 0) */
    /*   { */
    /*     if (payload_length > autoc4_config->dmx_configs[i].channel_count) */
    /*       payload_length = autoc4_config->dmx_configs[i].channel_count; */
    /*     set_dmx_channels(payload, AUTOC4_DMX_UNIVERSE, autoc4_config->dmx_configs[i].start_channel - 1, payload_length); */
    /*     return; */
    /*   } */

    // Save logicer heartbeat state
    if (strncmp(topic, AUTOC4_LOGICER_HEARTBEAT_TOPIC, topic_length) == 0)
    {
      logicer_state = (bool) ((uint8_t*)payload)[0];
      return;
    }

    // Enter locked mode
    if (strncmp(topic, AUTOC4_LOCKED_MODE_TOPIC, topic_length) == 0)
    {
      locked_mode = true;
      return;
    }
  }
}

static void autoc4_gpio_init(void)
{
  // Configure outputs
  for (int i=0; i<autoc4_config->output_count; i++)
  {
    const autoc4_output_config *oc = &autoc4_config->output_configs[i];

    HAL_GPIO_Init(oc->gpio_port, &(GPIO_InitTypeDef){
        .Pin    = oc->gpio_pin,
        .Mode   = oc->opendrain ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP,
        .Pull   = GPIO_NOPULL,
        .Speed  = GPIO_SPEED_HIGH,
      });
  }

  // Configure inputs
  for (int i=0; i<autoc4_config->input_count; i++)
  {
    const autoc4_input_config *ic = &autoc4_config->input_configs[i];

    HAL_GPIO_Init(ic->gpio_port, &(GPIO_InitTypeDef){
        .Pin    = ic->gpio_pin,
        .Mode   = GPIO_MODE_INPUT,
        .Pull   = ic->pullup ? GPIO_PULLUP : GPIO_NOPULL,
        .Speed  = GPIO_SPEED_HIGH,
      });
  }
}


static void autoc4_poll_blinking(void)
{
  for (int i=0; i<autoc4_config->output_count; i++)
  {
    if (!autoc4_config->output_configs[i].enable_blinking)
      continue;

    if (output_states[i].value == 0)
    {
      autoc4_set_output(i, 0);
      continue;
    }

    if (output_states[i].timer == 0)
    {
      // check current output level
      if (autoc4_get_output(i))
      {
        // output HIGH

        // set timer
        output_states[i].timer = blink_timeouts[(output_states[i].value & 0xc0) >> 6] * (((output_states[i].value & 0x30)>>4) + 1);

        // special case, value == 0b0000 -> don't set output
        if (output_states[i].timer == 1)
          return;

        // toggle output
        autoc4_set_output(i, 0);
      }
      else
      {
        // output LOW

        // set timer
        output_states[i].timer = blink_timeouts[(output_states[i].value & 0x0c) >> 2] * (((output_states[i].value & 0x03)>>0) + 1);

        // special case, value == 0b0000 -> don't set output
        if (output_states[i].timer == 1)
          return;

        // toggle it
        autoc4_set_output(i, 1);
      }
    }

    output_states[i].timer--;
  }
}


static void autoc4_init_input_states(void)
{
  for (int i=0; i<autoc4_config->input_count; i++)
  {
    bool input = (bool) autoc4_get_input(i);
    pin_input_states[i].prev_state = input;
    pin_input_states[i].mqtt_dirty = true;
  }
}

static void autoc4_read_inputs(void)
{
  for (int i=0; i<autoc4_config->input_count; i++)
  {
    bool input = (bool) autoc4_get_input(i);
    if ((input && !pin_input_states[i].prev_state) || (!input && pin_input_states[i].prev_state)) // input has changed
    {
      if (!mqtt_is_connected() || !logicer_state)
      {
        // Emergency switch mode
        if (autoc4_config->input_configs[i].is_emergency_switch)
          autoc4_emergency_switch_toggled();
      }
      else
      {
        // Normal mode
        pin_input_states[i].mqtt_dirty = true;
      }
    }
    pin_input_states[i].prev_state = input;
  }
}


static void
autoc4_init_fan_control(void)
{
  /* // simply turn on */
  /* DDRD |= (1<<PD7); */
  /* PORTD |= (1<<PD7); */
}

static void
autoc4_init_status_light(void)
{
  /* // power and output pin */
  /* DDRB |= (1<<PB0) | (1<<PB1); */
  /*  */
  /* // power on */
  /* PORTB |= (1<<PB0); */
}

static void
autoc4_update_status_light(void)
{
  /* uint8_t blue = locked_mode ? 0xff : 0x00; */
  /*  */
  /* if (!mqtt_is_connected()) */
  /*   ws2812b_write_rgb_n(0xff, 0x00, blue, 4); */
  /* else if (!logicer_state) */
  /*   ws2812b_write_rgb_n(0xff, 0xff, blue, 4); */
  /* else */
  /*   ws2812b_write_rgb_n(0x00, 0xff, blue, 4); */
}


void
autoc4_periodic(void)
{
  static uint8_t counter_100ms = 10;
  if (--counter_100ms == 0)
  {
    // every 100ms
    counter_100ms = 10;
    autoc4_poll_blinking();
  }

  static uint8_t counter_1s = 100;
  if (--counter_1s == 0)
  {
    // every 1s
    counter_1s = 100;
    autoc4_update_status_light();
  }

  autoc4_read_inputs();
}

static const mqtt_callback_config_t callback_config = {
    .connack_callback = autoc4_connack_callback,
    .poll_callback = autoc4_poll,
    .close_callback = NULL,
    .publish_callback = autoc4_publish_callback,
  };

void
autoc4_init(void)
{
  /* ws2812b_init(); */
  /* autoc4_init_status_light(); */
  /* autoc4_init_fan_control(); */

  mqtt_register_callback(&callback_config);

  logicer_state = false;
  locked_mode = false;

  autoc4_gpio_init();
  autoc4_init_input_states();
  mqtt_set_connection_config(autoc4_config->mqtt_con_config);
}

/*
  -- Ethersex META --
  header(services/autoc4/autoc4.h)
  init(autoc4_init)
  timer(1, autoc4_periodic())
*/
