#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Import general universal C utils wrapper
#include <lira_metadata/utils/msg.h>
#include <lira_metadata/utils/msg_parser.h>

// Import global enum lookup map
#include <lira_metadata/message_type/msg_definition.h>
#include <lira_metadata/message_type/msg_set.h>

#include <lira_metadata/message_type/command_type.h>
#include <lira_metadata/message_type/param_config.h>

using lira_protocol_message_t = lira_message_t;

// Todo: add message definition
// lira_protocol_status_t* lira_protocol_get_channel_status(uint8_t chan);

#ifdef __cplusplus
}
#endif
