static const autoc4_output_config outputs[] = {
  { GPIOC, GPIO_PIN_13, "licht/test1", false, false, true, false },
};

static const autoc4_input_config inputs[] = {
  { GPIOB, GPIO_PIN_8, "schalter/test1", true, false, true  },
};

static const autoc4_dmx_config dmxs[] = {
  /* { string_dmx_1,  1, 8 }, */
};

#define AUTOC4_CLIENT_ID "testid2"
#define AUTOC4_HEARTBEAT_TOPIC "heartbeat/" AUTOC4_CLIENT_ID
#define AUTOC4_LOCKED_MODE_TOPIC "lock/testid/"
#define AUTOC4_LOGICER_HEARTBEAT_TOPIC "heartbeat/logicer"

static const uint8_t will_message[]  = { 0x00 };

static char const * const auto_subscribe_topics[] = {
  AUTOC4_LOGICER_HEARTBEAT_TOPIC,
  "sudo/#",
  "licht/test1",
  NULL
};

static const mqtt_connection_config_t mqtt_config = {
  .client_id = AUTOC4_CLIENT_ID,
  .user = NULL,
  .pass = NULL,
  .will_topic = AUTOC4_HEARTBEAT_TOPIC,
  .will_qos = 0,
  .will_retain = 1,
  .will_message = will_message,
  .will_message_length = sizeof(will_message),
  .target_ip = { HTONS(((172) << 8) | (23)), HTONS(((23) << 8) | (110)) },
  /* .target_ip = { HTONS(((192) << 8) | (168)), HTONS(((0) << 8) | (5)) }, */

  .auto_subscribe_topics = auto_subscribe_topics,
};

static const autoc4_config_t config = {
  .output_count = sizeof(outputs) / sizeof(autoc4_output_config),
  .input_count = sizeof(inputs) / sizeof(autoc4_input_config),
  .dmx_count = sizeof(dmxs) / sizeof(autoc4_dmx_config),
  .output_configs = outputs,
  .input_configs = inputs,
  .dmx_configs = dmxs,
  .dmx_topic = "dmx/plenar",
  .mqtt_con_config = &mqtt_config,
};
