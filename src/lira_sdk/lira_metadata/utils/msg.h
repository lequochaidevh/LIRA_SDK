/* Pure C-Header Message Frame Structure */
#pragma once

#ifndef LIRA_MESSAGE_H
#define LIRA_MESSAGE_H

#include <stdint.h>

#define LIRA_MAX_PAYLOAD_LEN 255

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct __lira_message {
    uint8_t  magic;                         /* Protocol magic marker (e.g., 0xFD) */
    uint8_t  len;                           /* Length of the payload segment */
    uint8_t  seq;                           /* Sequence counter for packet loss tracking */
    uint8_t  sysid;                         /* System ID (Drone ID) */
    uint8_t  compid;                        /* Component ID (Autopilot, Camera, etc.) */
    uint32_t msgid;                         /* Unique Message Type ID */
    uint8_t  payload[LIRA_MAX_PAYLOAD_LEN]; /* Core structural content storage memory */
    uint16_t checksum;                      /* CRC16 validation integrity anchor */
} lira_message_t;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* LIRA_MESSAGE_H */
