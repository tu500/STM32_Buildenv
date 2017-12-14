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

#ifndef AUTOC4_H
#define AUTOC4_H

#include <stdbool.h>


typedef struct {
  GPIO_TypeDef* gpio_port;
  uint16_t gpio_pin;
  char const *topic;
  bool opendrain;
  bool enable_blinking;
  bool emergency_toggled;
  bool emergency_zeroed;
} autoc4_output_config;

typedef struct {
  GPIO_TypeDef* gpio_port;
  uint16_t gpio_pin;
  char const *topic;
  bool pullup, inverted;
  bool is_emergency_switch;
} autoc4_input_config;

typedef struct {
  char const *topic;
  uint16_t start_channel, channel_count;
} autoc4_dmx_config;


typedef struct {
  uint8_t output_count;
  uint8_t input_count;
  uint8_t dmx_count;
  const autoc4_output_config *output_configs;
  const autoc4_input_config *input_configs;
  const autoc4_dmx_config *dmx_configs;
  char const *dmx_topic;
  const mqtt_connection_config_t *mqtt_con_config;
} autoc4_config_t;


void autoc4_init(void);
void autoc4_periodic(void);


#endif /* MOTD_H */
