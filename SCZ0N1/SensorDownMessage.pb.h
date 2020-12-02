/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.3 */

#ifndef PB_SENSORMESSAGES_SENSORDOWNMESSAGE_PB_H_INCLUDED
#define PB_SENSORMESSAGES_SENSORDOWNMESSAGE_PB_H_INCLUDED
#include "pb.h"
#include "SensorMessages.pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _SensorMessages_SensorDownMessage {
    char instruction_name[40];
    char gateway_id[16];
    int32_t node_id;
    uint32_t unix_time;
    char instruction_id[25];
    pb_size_t which_msg;
    union {
        ConfigClockTask configclocktask;
        ConfigVibraWireParameter configvibrawireparameter;
    } msg;
} SensorMessages_SensorDownMessage;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define SensorMessages_SensorDownMessage_init_default {"", "", 0, 0, "", 0, {ConfigClockTask_init_default}}
#define SensorMessages_SensorDownMessage_init_zero {"", "", 0, 0, "", 0, {ConfigClockTask_init_zero}}

/* Field tags (for use in manual encoding/decoding) */
#define SensorMessages_SensorDownMessage_instruction_name_tag 1
#define SensorMessages_SensorDownMessage_gateway_id_tag 2
#define SensorMessages_SensorDownMessage_node_id_tag 3
#define SensorMessages_SensorDownMessage_unix_time_tag 5
#define SensorMessages_SensorDownMessage_instruction_id_tag 6
#define SensorMessages_SensorDownMessage_configclocktask_tag 31
#define SensorMessages_SensorDownMessage_configvibrawireparameter_tag 75

/* Struct field encoding specification for nanopb */
#define SensorMessages_SensorDownMessage_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, STRING,   instruction_name,   1) \
X(a, STATIC,   SINGULAR, STRING,   gateway_id,        2) \
X(a, STATIC,   SINGULAR, INT32,    node_id,           3) \
X(a, STATIC,   SINGULAR, UINT32,   unix_time,         5) \
X(a, STATIC,   SINGULAR, STRING,   instruction_id,    6) \
X(a, STATIC,   ONEOF,    MESSAGE,  (msg,configclocktask,msg.configclocktask),  31) \
X(a, STATIC,   ONEOF,    MESSAGE,  (msg,configvibrawireparameter,msg.configvibrawireparameter),  75)
#define SensorMessages_SensorDownMessage_CALLBACK NULL
#define SensorMessages_SensorDownMessage_DEFAULT NULL
#define SensorMessages_SensorDownMessage_msg_configclocktask_MSGTYPE ConfigClockTask
#define SensorMessages_SensorDownMessage_msg_configvibrawireparameter_MSGTYPE ConfigVibraWireParameter

extern const pb_msgdesc_t SensorMessages_SensorDownMessage_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define SensorMessages_SensorDownMessage_fields &SensorMessages_SensorDownMessage_msg

/* Maximum encoded size of messages (where known) */
#define SensorMessages_SensorDownMessage_size    289

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
