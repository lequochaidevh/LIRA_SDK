/* Pure C-Header Stream Decoder State Machine */
#pragma once

#ifndef LIRA_BUFFER_PARSER_H
#define LIRA_BUFFER_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "frame/msg.h"
#include "msgid/msg_set.h"

#define LIRA_MAGIC_BYTE 0xFD

typedef enum {
    LIRA_PARSE_STATE_UNINIT = 0,
    LIRA_PARSE_STATE_GOT_MAGIC,
    LIRA_PARSE_STATE_GOT_LEN,
    LIRA_PARSE_STATE_GOT_SEQ,
    LIRA_PARSE_STATE_GOT_SYSID,
    LIRA_PARSE_STATE_GOT_COMPID,
    LIRA_PARSE_STATE_GOT_MSGID_0,
    LIRA_PARSE_STATE_GOT_MSGID_1,
    LIRA_PARSE_STATE_GOT_MSGID_2,
    LIRA_PARSE_STATE_GOT_MSGID_3,
    LIRA_PARSE_STATE_GOT_PAYLOAD
} lira_parse_state_t;

typedef struct __lira_status {
    lira_parse_state_t parse_state;
    uint8_t            packet_idx;
    uint16_t           expected_len;
} lira_status_t;

/**
 * @brief Streams an incoming network byte into a structured lira_message_t framework envelope.
 * @return true if a complete message frame is verified and assembled.
 */
static inline bool lira_parse_char(uint8_t c, lira_message_t* rxmsg, lira_status_t* status) {
    switch (status->parse_state) {
        case LIRA_PARSE_STATE_UNINIT:
            if (c == LIRA_MAGIC_BYTE) {
                rxmsg->magic        = c;
                status->parse_state = LIRA_PARSE_STATE_GOT_MAGIC;
            }
            break;

        case LIRA_PARSE_STATE_GOT_MAGIC:
            rxmsg->len          = c;
            status->parse_state = LIRA_PARSE_STATE_GOT_LEN;
            break;

        case LIRA_PARSE_STATE_GOT_LEN:
            rxmsg->seq          = c;
            status->parse_state = LIRA_PARSE_STATE_GOT_SEQ;
            break;

        case LIRA_PARSE_STATE_GOT_SEQ:
            rxmsg->sysid        = c;
            status->parse_state = LIRA_PARSE_STATE_GOT_SYSID;
            break;

        case LIRA_PARSE_STATE_GOT_SYSID:
            rxmsg->compid       = c;
            status->parse_state = LIRA_PARSE_STATE_GOT_COMPID;
            break;

        /* Extract 32-bit msgid over 4 sequential wire bytes */
        case LIRA_PARSE_STATE_GOT_COMPID:
            rxmsg->msgid        = (uint32_t)c;
            status->parse_state = LIRA_PARSE_STATE_GOT_MSGID_0;
            break;
        case LIRA_PARSE_STATE_GOT_MSGID_0:
            rxmsg->msgid |= ((uint32_t)c << 8);
            status->parse_state = LIRA_PARSE_STATE_GOT_MSGID_1;
            break;
        case LIRA_PARSE_STATE_GOT_MSGID_1:
            rxmsg->msgid |= ((uint32_t)c << 16);
            status->parse_state = LIRA_PARSE_STATE_GOT_MSGID_2;
            break;
        case LIRA_PARSE_STATE_GOT_MSGID_2:
            rxmsg->msgid |= ((uint32_t)c << 24);
            status->expected_len = lira_get_message_length(rxmsg->msgid);
            status->packet_idx   = 0;

            // Handle edge case: empty message package parameters
            if (rxmsg->len == 0) {
                status->parse_state = LIRA_PARSE_STATE_UNINIT;
                return true;
            }
            status->parse_state = LIRA_PARSE_STATE_GOT_PAYLOAD;
            break;

        case LIRA_PARSE_STATE_GOT_PAYLOAD:
            rxmsg->payload[status->packet_idx++] = c;
            if (status->packet_idx >= rxmsg->len) {
                status->parse_state = LIRA_PARSE_STATE_UNINIT;  // Reset tracker
                return true;                                    /* Complete payload stream acquired successfully! */
            }
            break;

        default:
            status->parse_state = LIRA_PARSE_STATE_UNINIT;
            break;
    }
    return false;
}

#endif /* LIRA_BUFFER_PARSER_H */