#ifndef _BUSI_PRO_H_
#define _BUSI_PRO_H_
#include "dfs_posix.h"
#include "minIni.h"
#include "busi_cfg.h"
#define _ _
#define BUSI_SEM_WAIT_TIME (500)
#define BUSI_MSG_SEM_WAIT_TIME (500)

#define BUSI_MODBUS_CMD_PRO_NMSG (10)
#define BUSI_MODBUS_CMD_PRO_POOL_SIZE (BUSI_MODBUS_CMD_PRO_NMSG*sizeof(modbus_cmd_t))

#define BUSI_CONT_MAX_NMSG (10)
#define BUSI_CONT_MAX_MSG_SIZE (sizeof(void *)*BUSI_CONT_MAX_NMSG)
#define CDUMB_MAX_LEN    10
#define CDUMB_DATA_LEN   128+2

#define PKT_DATA_COUNT 64
#define MODBUS_EINVAL 2
typedef struct
{
 //动态库函数指针
 //void *(pfun_)(void); 
 //将动态库中的函数逐一装入到这些函数指针中，以备使用
} DL_FUNCS_STRU;





typedef struct
{
    unsigned char func       ;
    unsigned short data_addr ;
    unsigned short data[4]  ;
} __attribute__((packed)) modbus_cmd_t;
/**
 * 0:2400 1:4800 2:9600 3:19200 4:38400 5:57600 6:115200
 * 7:230400 8:460800 9:921600 10:2000000 11:3000000
 */
typedef enum{	
	ADU_MAX_LEN  = 128,
    BAUD_RATE    = 8,
    RS_MODE      = 1,
    RESP_TIMEOUT = 1000000,
    _0X_LEN      = 0,
    _1X_LEN      = 0,
    _4X_LEN      = (sizeof(input_reg_t)/2),
    _3X_LEN      = (sizeof(hold_reg_t)/2),
    RESERVED     = 0,
} modbus_cfg_def;

#define  MODBUS_CFG_DEF {\
ADU_MAX_LEN,\
BAUD_RATE,\
RS_MODE,\
RESP_TIMEOUT,\
_0X_LEN,\
_1X_LEN,\
_4X_LEN,\
_3X_LEN,\
RESERVED}

enum{
	CAIL_ZERO = 0,
	CAIL_FULL = 1,
}cali_sample_e;
enum{
	SINGLE_SAMPLE = 0,
	CONT_SAMPLE = 1,
}sample_type;
enum{
	STOP_SAMPLE_M = 0,
	CONT_SAMPLE_M = 1,
	RS_CONT_SAMPLE_M = 2,
	RS_SINGLE_SAMPLE_M= 3,
	ZERO_CAIL_SAMPLE_M = 4,
	FULL_CAIL_SAMPLE_M = 5,
}run_mode;

typedef enum{				//保持寄存器下标序号
	HOLD_REG_0 = 0,
	HOLD_REG_1,
	HOLD_REG_2,
	HOLD_REG_3,
	HOLD_REG_4,
	HOLD_REG_5,
	HOLD_REG_6,
	HOLD_REG_7,
	HOLD_REG_8,
	HOLD_REG_9,
	HOLD_REG_10,
	HOLD_REG_11,
	HOLD_REG_12,
	HOLD_REG_13,
	HOLD_REG_14,
	HOLD_REG_15,
	HOLD_REG_16,
	HOLD_REG_17,
	HOLD_REG_18,
	HOLD_REG_19,
	HOLD_REG_20,
	HOLD_REG_21,
	HOLD_REG_22,
	HOLD_REG_LAST
}hold_reg_off;





#define BUSI_COMA_DATA_HEARD_LEN 2
typedef struct
{
	unsigned short      pkt;
	unsigned char       data[128];
}__attribute__((packed)) busi_coma_data_t;

typedef struct
{
	unsigned char pkt_data_count ;
	unsigned char odr ;
	void * cdumb ;
	void * cont_sam_pipe ;
}__attribute__((packed)) bs_cont_sample_msg_t ;

typedef struct
{
	unsigned short k_value;
	unsigned short s_value;
}__attribute__((packed)) bs_set_sensor_config_msg_t;

typedef struct
{
	unsigned char odr;
	void *stress_h;
	void *stress_l;
}__attribute__((packed)) bs_rs_cont_sample_msg_t;

typedef struct
{
	void *stress_h;
	void *stress_l;
}__attribute__((packed)) bs_rs_single_sample_msg_t;
typedef struct
{
	unsigned char type;
	unsigned short *calu_value_h;
	unsigned short *calu_value_l;
}__attribute__((packed)) bs_rs_cali_msg_t;



//------------向SPVW采集组件发送的指令结构体,用于解析
//手动采样指令数据结构体
typedef struct
{
    unsigned char mid_freq[19]; //中心频率数据,19个通道
    unsigned char local_ch;  //本机通道选择
    unsigned char extend_ch[2]; //扩展通道选择
} __attribute__((packed)) SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU;

//高速采样指令数据结构体
typedef struct
{
    unsigned char mid_freq; //中心频率数据,本机通道1
} __attribute__((packed)) SCZ_SAM_SPVW0_HS_SAMPLE_PARAM_STRU;

//------------主任务向采样任务发送的指令结构体
typedef struct
{
    unsigned char cmd;                    //任务指令
    unsigned char d_src;                  //消息源
    SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU param; //采样参数
    SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU hs_param;  //高速采样参数
} __attribute__((packed)) SCZ_SAM_SPVW0_SAMPLE_ORDER_STRU;
//------------组件返回数据的结构体
typedef struct
{
    unsigned char channel;  //通道号
    unsigned char db;       //db值
    unsigned short err_cnt; //错误计数值
    float freq;             //频率值
    float temp;             //温度值
    float e_vol;            //扩展器电池电压
} __attribute__((packed)) SCZ_SAM_SPVW0_DAT_RES_STRU;






extern struct  rt_thread  thread_busi_modbus_cmd_pro;
extern unsigned char thread_busi_modbus_cmd_pro_stack[THREAD_STACK_SIZE];


extern unsigned char busi_modbus_cmd_pro_pool[BUSI_MODBUS_CMD_PRO_POOL_SIZE];


extern struct rt_thread rthread_cont_sam_msg_pro; //BUSI连续数据处理
extern unsigned char thread_cont_sam_msg_pro_stack[THREAD_STACK_SIZE];

extern void modbus_cmd_pro_entry(void *p);
extern void BUSI_cont_sam_pro_entry(void *p);
extern void load_input_reg_def(void);

extern int scz0b1_timer_sample(void);
extern unsigned char scz_calc_bit_1(unsigned short data);

void scz_sample_thread_entry(void *parameter);

extern input_reg_t input_reg;

extern rt_mq_t busi_modbus_cmd_pro_pipe ; 

extern const char bbi4[];
#endif