/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.3 */

#ifndef PB_SENSORMESSAGES_SENSORMESSAGES_PB_H_INCLUDED
#define PB_SENSORMESSAGES_SENSORMESSAGES_PB_H_INCLUDED
#include "pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _SystemBootInfo_SBIRes {
    SystemBootInfo_SBIRes_SYS_POWERUP = 0,
    SystemBootInfo_SBIRes_SYS_BROWNOUT = 1,
    SystemBootInfo_SBIRes_SYS_WATCHDOG = 2,
    SystemBootInfo_SBIRes_SYS_ADBS_ACT = 3,
    SystemBootInfo_SBIRes_BUSI_NORMAL = 10,
    SystemBootInfo_SBIRes_BUSI_SAM_TIMEOUT = 11
} SystemBootInfo_SBIRes;

/* Struct definitions */
typedef struct _ClockTaskSampleInfo {
    int32_t sample_pattern;
    int32_t local_channel_bit;
    int32_t ext_channel_bit;
} ClockTaskSampleInfo;

typedef struct _ClockTaskTimeInfoSensorNode {
    int32_t clock1_is_on;
    int32_t clock1_hour;
    int32_t clock1_minute;
    int32_t clock1_second;
    int32_t clock1_interval;
    int32_t clock1_number;
    int32_t clock2_is_on;
    int32_t clock2_hour;
    int32_t clock2_minute;
    int32_t clock2_second;
    int32_t clock2_interval;
    int32_t clock2_number;
    int32_t clock0_is_on;
    int32_t clock0_hour;
    int32_t clock0_minute;
    int32_t clock0_second;
    int32_t clock0_interval;
    int32_t clock0_number;
} ClockTaskTimeInfoSensorNode;

typedef struct _ConfigClockTask {
    int32_t sample_pattern;
    int32_t local_channel_bit;
    int32_t ext_channel_bit;
    int32_t clock_num;
    int32_t is_on;
    int32_t hour;
    int32_t minute;
    int32_t second;
    int32_t interval;
    int32_t number;
} ConfigClockTask;

typedef struct _ConfigVibraWireParameter {
    uint32_t ch0_midfreq;
    int32_t ch1_midfreq;
    int32_t ch2_midfreq;
    int32_t ch3_midfreq;
    int32_t ch4_midfreq;
    int32_t ch5_midfreq;
    int32_t ch6_midfreq;
    int32_t ch7_midfreq;
    int32_t ch8_midfreq;
    int32_t ch9_midfreq;
    int32_t ch10_midfreq;
    int32_t ch11_midfreq;
    int32_t ch12_midfreq;
    int32_t ch13_midfreq;
    int32_t ch14_midfreq;
    int32_t ch15_midfreq;
    int32_t ch16_midfreq;
} ConfigVibraWireParameter;

typedef struct _SystemBootInfo {
    int32_t version;
    SystemBootInfo_SBIRes boot_src;
} SystemBootInfo;

typedef struct _TopInfoNodeData {
    double battery_voltage;
    char product_type[12];
    int32_t sampling;
    int32_t version;
    int32_t worktime;
    int32_t battery_num;
    int32_t battery_type;
} TopInfoNodeData;

typedef struct _VibraWireData {
    double freq;
    double temp;
    double voltage;
} VibraWireData;

typedef struct _VibraWireInfo {
    uint32_t ch0_midfreq;
    uint32_t ch1_midfreq;
    uint32_t ch2_midfreq;
    uint32_t ch3_midfreq;
    uint32_t ch4_midfreq;
    uint32_t ch5_midfreq;
    uint32_t ch6_midfreq;
    uint32_t ch7_midfreq;
    uint32_t ch8_midfreq;
    uint32_t ch9_midfreq;
    uint32_t ch10_midfreq;
    uint32_t ch11_midfreq;
    uint32_t ch12_midfreq;
    uint32_t ch13_midfreq;
    uint32_t ch14_midfreq;
    uint32_t ch15_midfreq;
    uint32_t ch16_midfreq;
} VibraWireInfo;


/* Helper constants for enums */
#define _SystemBootInfo_SBIRes_MIN SystemBootInfo_SBIRes_SYS_POWERUP
#define _SystemBootInfo_SBIRes_MAX SystemBootInfo_SBIRes_BUSI_SAM_TIMEOUT
#define _SystemBootInfo_SBIRes_ARRAYSIZE ((SystemBootInfo_SBIRes)(SystemBootInfo_SBIRes_BUSI_SAM_TIMEOUT+1))


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define ConfigClockTask_init_default             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define ConfigVibraWireParameter_init_default    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define TopInfoNodeData_init_default             {0, "", 0, 0, 0, 0, 0}
#define ClockTaskTimeInfoSensorNode_init_default {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define ClockTaskSampleInfo_init_default         {0, 0, 0}
#define VibraWireInfo_init_default               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define VibraWireData_init_default               {0, 0, 0}
#define SystemBootInfo_init_default              {0, _SystemBootInfo_SBIRes_MIN}
#define ConfigClockTask_init_zero                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define ConfigVibraWireParameter_init_zero       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define TopInfoNodeData_init_zero                {0, "", 0, 0, 0, 0, 0}
#define ClockTaskTimeInfoSensorNode_init_zero    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define ClockTaskSampleInfo_init_zero            {0, 0, 0}
#define VibraWireInfo_init_zero                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define VibraWireData_init_zero                  {0, 0, 0}
#define SystemBootInfo_init_zero                 {0, _SystemBootInfo_SBIRes_MIN}

/* Field tags (for use in manual encoding/decoding) */
#define ClockTaskSampleInfo_sample_pattern_tag   9
#define ClockTaskSampleInfo_local_channel_bit_tag 14
#define ClockTaskSampleInfo_ext_channel_bit_tag  15
#define ClockTaskTimeInfoSensorNode_clock1_is_on_tag 1
#define ClockTaskTimeInfoSensorNode_clock1_hour_tag 2
#define ClockTaskTimeInfoSensorNode_clock1_minute_tag 3
#define ClockTaskTimeInfoSensorNode_clock1_second_tag 4
#define ClockTaskTimeInfoSensorNode_clock1_interval_tag 5
#define ClockTaskTimeInfoSensorNode_clock1_number_tag 6
#define ClockTaskTimeInfoSensorNode_clock2_is_on_tag 7
#define ClockTaskTimeInfoSensorNode_clock2_hour_tag 8
#define ClockTaskTimeInfoSensorNode_clock2_minute_tag 9
#define ClockTaskTimeInfoSensorNode_clock2_second_tag 10
#define ClockTaskTimeInfoSensorNode_clock2_interval_tag 11
#define ClockTaskTimeInfoSensorNode_clock2_number_tag 12
#define ClockTaskTimeInfoSensorNode_clock0_is_on_tag 13
#define ClockTaskTimeInfoSensorNode_clock0_hour_tag 14
#define ClockTaskTimeInfoSensorNode_clock0_minute_tag 15
#define ClockTaskTimeInfoSensorNode_clock0_second_tag 16
#define ClockTaskTimeInfoSensorNode_clock0_interval_tag 17
#define ClockTaskTimeInfoSensorNode_clock0_number_tag 18
#define ConfigClockTask_sample_pattern_tag       19
#define ConfigClockTask_local_channel_bit_tag    25
#define ConfigClockTask_ext_channel_bit_tag      26
#define ConfigClockTask_clock_num_tag            101
#define ConfigClockTask_is_on_tag                105
#define ConfigClockTask_hour_tag                 106
#define ConfigClockTask_minute_tag               107
#define ConfigClockTask_second_tag               108
#define ConfigClockTask_interval_tag             109
#define ConfigClockTask_number_tag               110
#define ConfigVibraWireParameter_ch0_midfreq_tag 1
#define ConfigVibraWireParameter_ch1_midfreq_tag 2
#define ConfigVibraWireParameter_ch2_midfreq_tag 3
#define ConfigVibraWireParameter_ch3_midfreq_tag 4
#define ConfigVibraWireParameter_ch4_midfreq_tag 5
#define ConfigVibraWireParameter_ch5_midfreq_tag 6
#define ConfigVibraWireParameter_ch6_midfreq_tag 7
#define ConfigVibraWireParameter_ch7_midfreq_tag 8
#define ConfigVibraWireParameter_ch8_midfreq_tag 9
#define ConfigVibraWireParameter_ch9_midfreq_tag 10
#define ConfigVibraWireParameter_ch10_midfreq_tag 11
#define ConfigVibraWireParameter_ch11_midfreq_tag 12
#define ConfigVibraWireParameter_ch12_midfreq_tag 13
#define ConfigVibraWireParameter_ch13_midfreq_tag 14
#define ConfigVibraWireParameter_ch14_midfreq_tag 15
#define ConfigVibraWireParameter_ch15_midfreq_tag 16
#define ConfigVibraWireParameter_ch16_midfreq_tag 17
#define SystemBootInfo_version_tag               101
#define SystemBootInfo_boot_src_tag              102
#define TopInfoNodeData_battery_voltage_tag      3
#define TopInfoNodeData_product_type_tag         4
#define TopInfoNodeData_sampling_tag             7
#define TopInfoNodeData_version_tag              8
#define TopInfoNodeData_worktime_tag             12
#define TopInfoNodeData_battery_num_tag          101
#define TopInfoNodeData_battery_type_tag         102
#define VibraWireData_freq_tag                   2
#define VibraWireData_temp_tag                   3
#define VibraWireData_voltage_tag                5
#define VibraWireInfo_ch0_midfreq_tag            1
#define VibraWireInfo_ch1_midfreq_tag            2
#define VibraWireInfo_ch2_midfreq_tag            3
#define VibraWireInfo_ch3_midfreq_tag            4
#define VibraWireInfo_ch4_midfreq_tag            5
#define VibraWireInfo_ch5_midfreq_tag            6
#define VibraWireInfo_ch6_midfreq_tag            7
#define VibraWireInfo_ch7_midfreq_tag            8
#define VibraWireInfo_ch8_midfreq_tag            9
#define VibraWireInfo_ch9_midfreq_tag            10
#define VibraWireInfo_ch10_midfreq_tag           11
#define VibraWireInfo_ch11_midfreq_tag           12
#define VibraWireInfo_ch12_midfreq_tag           13
#define VibraWireInfo_ch13_midfreq_tag           14
#define VibraWireInfo_ch14_midfreq_tag           15
#define VibraWireInfo_ch15_midfreq_tag           16
#define VibraWireInfo_ch16_midfreq_tag           17

/* Struct field encoding specification for nanopb */
#define ConfigClockTask_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    sample_pattern,   19) \
X(a, STATIC,   SINGULAR, INT32,    local_channel_bit,  25) \
X(a, STATIC,   SINGULAR, INT32,    ext_channel_bit,  26) \
X(a, STATIC,   SINGULAR, INT32,    clock_num,       101) \
X(a, STATIC,   SINGULAR, INT32,    is_on,           105) \
X(a, STATIC,   SINGULAR, INT32,    hour,            106) \
X(a, STATIC,   SINGULAR, INT32,    minute,          107) \
X(a, STATIC,   SINGULAR, INT32,    second,          108) \
X(a, STATIC,   SINGULAR, INT32,    interval,        109) \
X(a, STATIC,   SINGULAR, INT32,    number,          110)
#define ConfigClockTask_CALLBACK NULL
#define ConfigClockTask_DEFAULT NULL

#define ConfigVibraWireParameter_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UINT32,   ch0_midfreq,       1) \
X(a, STATIC,   SINGULAR, INT32,    ch1_midfreq,       2) \
X(a, STATIC,   SINGULAR, INT32,    ch2_midfreq,       3) \
X(a, STATIC,   SINGULAR, INT32,    ch3_midfreq,       4) \
X(a, STATIC,   SINGULAR, INT32,    ch4_midfreq,       5) \
X(a, STATIC,   SINGULAR, INT32,    ch5_midfreq,       6) \
X(a, STATIC,   SINGULAR, INT32,    ch6_midfreq,       7) \
X(a, STATIC,   SINGULAR, INT32,    ch7_midfreq,       8) \
X(a, STATIC,   SINGULAR, INT32,    ch8_midfreq,       9) \
X(a, STATIC,   SINGULAR, INT32,    ch9_midfreq,      10) \
X(a, STATIC,   SINGULAR, INT32,    ch10_midfreq,     11) \
X(a, STATIC,   SINGULAR, INT32,    ch11_midfreq,     12) \
X(a, STATIC,   SINGULAR, INT32,    ch12_midfreq,     13) \
X(a, STATIC,   SINGULAR, INT32,    ch13_midfreq,     14) \
X(a, STATIC,   SINGULAR, INT32,    ch14_midfreq,     15) \
X(a, STATIC,   SINGULAR, INT32,    ch15_midfreq,     16) \
X(a, STATIC,   SINGULAR, INT32,    ch16_midfreq,     17)
#define ConfigVibraWireParameter_CALLBACK NULL
#define ConfigVibraWireParameter_DEFAULT NULL

#define TopInfoNodeData_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, DOUBLE,   battery_voltage,   3) \
X(a, STATIC,   SINGULAR, STRING,   product_type,      4) \
X(a, STATIC,   SINGULAR, INT32,    sampling,          7) \
X(a, STATIC,   SINGULAR, INT32,    version,           8) \
X(a, STATIC,   SINGULAR, INT32,    worktime,         12) \
X(a, STATIC,   SINGULAR, INT32,    battery_num,     101) \
X(a, STATIC,   SINGULAR, INT32,    battery_type,    102)
#define TopInfoNodeData_CALLBACK NULL
#define TopInfoNodeData_DEFAULT NULL

#define ClockTaskTimeInfoSensorNode_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    clock1_is_on,      1) \
X(a, STATIC,   SINGULAR, INT32,    clock1_hour,       2) \
X(a, STATIC,   SINGULAR, INT32,    clock1_minute,     3) \
X(a, STATIC,   SINGULAR, INT32,    clock1_second,     4) \
X(a, STATIC,   SINGULAR, INT32,    clock1_interval,   5) \
X(a, STATIC,   SINGULAR, INT32,    clock1_number,     6) \
X(a, STATIC,   SINGULAR, INT32,    clock2_is_on,      7) \
X(a, STATIC,   SINGULAR, INT32,    clock2_hour,       8) \
X(a, STATIC,   SINGULAR, INT32,    clock2_minute,     9) \
X(a, STATIC,   SINGULAR, INT32,    clock2_second,    10) \
X(a, STATIC,   SINGULAR, INT32,    clock2_interval,  11) \
X(a, STATIC,   SINGULAR, INT32,    clock2_number,    12) \
X(a, STATIC,   SINGULAR, INT32,    clock0_is_on,     13) \
X(a, STATIC,   SINGULAR, INT32,    clock0_hour,      14) \
X(a, STATIC,   SINGULAR, INT32,    clock0_minute,    15) \
X(a, STATIC,   SINGULAR, INT32,    clock0_second,    16) \
X(a, STATIC,   SINGULAR, INT32,    clock0_interval,  17) \
X(a, STATIC,   SINGULAR, INT32,    clock0_number,    18)
#define ClockTaskTimeInfoSensorNode_CALLBACK NULL
#define ClockTaskTimeInfoSensorNode_DEFAULT NULL

#define ClockTaskSampleInfo_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    sample_pattern,    9) \
X(a, STATIC,   SINGULAR, INT32,    local_channel_bit,  14) \
X(a, STATIC,   SINGULAR, INT32,    ext_channel_bit,  15)
#define ClockTaskSampleInfo_CALLBACK NULL
#define ClockTaskSampleInfo_DEFAULT NULL

#define VibraWireInfo_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UINT32,   ch0_midfreq,       1) \
X(a, STATIC,   SINGULAR, UINT32,   ch1_midfreq,       2) \
X(a, STATIC,   SINGULAR, UINT32,   ch2_midfreq,       3) \
X(a, STATIC,   SINGULAR, UINT32,   ch3_midfreq,       4) \
X(a, STATIC,   SINGULAR, UINT32,   ch4_midfreq,       5) \
X(a, STATIC,   SINGULAR, UINT32,   ch5_midfreq,       6) \
X(a, STATIC,   SINGULAR, UINT32,   ch6_midfreq,       7) \
X(a, STATIC,   SINGULAR, UINT32,   ch7_midfreq,       8) \
X(a, STATIC,   SINGULAR, UINT32,   ch8_midfreq,       9) \
X(a, STATIC,   SINGULAR, UINT32,   ch9_midfreq,      10) \
X(a, STATIC,   SINGULAR, UINT32,   ch10_midfreq,     11) \
X(a, STATIC,   SINGULAR, UINT32,   ch11_midfreq,     12) \
X(a, STATIC,   SINGULAR, UINT32,   ch12_midfreq,     13) \
X(a, STATIC,   SINGULAR, UINT32,   ch13_midfreq,     14) \
X(a, STATIC,   SINGULAR, UINT32,   ch14_midfreq,     15) \
X(a, STATIC,   SINGULAR, UINT32,   ch15_midfreq,     16) \
X(a, STATIC,   SINGULAR, UINT32,   ch16_midfreq,     17)
#define VibraWireInfo_CALLBACK NULL
#define VibraWireInfo_DEFAULT NULL

#define VibraWireData_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, DOUBLE,   freq,              2) \
X(a, STATIC,   SINGULAR, DOUBLE,   temp,              3) \
X(a, STATIC,   SINGULAR, DOUBLE,   voltage,           5)
#define VibraWireData_CALLBACK NULL
#define VibraWireData_DEFAULT NULL

#define SystemBootInfo_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    version,         101) \
X(a, STATIC,   SINGULAR, UENUM,    boot_src,        102)
#define SystemBootInfo_CALLBACK NULL
#define SystemBootInfo_DEFAULT NULL

extern const pb_msgdesc_t ConfigClockTask_msg;
extern const pb_msgdesc_t ConfigVibraWireParameter_msg;
extern const pb_msgdesc_t TopInfoNodeData_msg;
extern const pb_msgdesc_t ClockTaskTimeInfoSensorNode_msg;
extern const pb_msgdesc_t ClockTaskSampleInfo_msg;
extern const pb_msgdesc_t VibraWireInfo_msg;
extern const pb_msgdesc_t VibraWireData_msg;
extern const pb_msgdesc_t SystemBootInfo_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define ConfigClockTask_fields &ConfigClockTask_msg
#define ConfigVibraWireParameter_fields &ConfigVibraWireParameter_msg
#define TopInfoNodeData_fields &TopInfoNodeData_msg
#define ClockTaskTimeInfoSensorNode_fields &ClockTaskTimeInfoSensorNode_msg
#define ClockTaskSampleInfo_fields &ClockTaskSampleInfo_msg
#define VibraWireInfo_fields &VibraWireInfo_msg
#define VibraWireData_fields &VibraWireData_msg
#define SystemBootInfo_fields &SystemBootInfo_msg

/* Maximum encoded size of messages (where known) */
#define ConfigClockTask_size                     120
#define ConfigVibraWireParameter_size            184
#define TopInfoNodeData_size                     79
#define ClockTaskTimeInfoSensorNode_size         201
#define ClockTaskSampleInfo_size                 33
#define VibraWireInfo_size                       104
#define VibraWireData_size                       27
#define SystemBootInfo_size                      15

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif