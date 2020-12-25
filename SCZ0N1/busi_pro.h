#ifndef _BUSI_PRO_H_
#define _BUSI_PRO_H_
#include "dfs_posix.h"
#include "minIni.h"
#include "busi_cfg.h"

#include "pb_decode.h"
#include "pb_encode.h"
#include "pb.h"
#include "SensorMessages.pb.h"
#include "SensorUpMessage.pb.h"
#include "SensorDownMessage.pb.h"
#include "SensorAckMessage.pb.h"

#define _ _
#define BUSI_SEM_WAIT_TIME (500)
#define BUSI_MSG_SEM_WAIT_TIME (100)
#define BUSI_COM_MSG_SEM_WAIT_TIME (5100)
#define BUSI_COM_MSG_MTX_WAIT_TIME (10000)

#define BUSI_CMD_PRO_NMSG (3)
#define BUSI_CMD_PRO_POOL_SIZE (BUSI_CMD_PRO_NMSG * sizeof(busi_cmd_buf_t))

#define BUSI_CLK_PRO_NMSG (8)

#define BUSI_CONT_MAX_NMSG (16)
#define BUSI_CONT_MAX_MSG_SIZE (BUSI_CONT_MAX_NMSG * sizeof(SCZ_SAM_SPVW0_DAT_RES_STRU))
#define CDUMB_MAX_LEN 10
#define CDUMB_DATA_LEN 128 + 2
#define F_DATA_COUNT 25
#define F_DATA_LEN (F_DATA_COUNT * 2)
//#define PKT_DATA_COUNT 64
#define IS_SEND_TEST 0






//
//  SCZ定义开始
//
//定义通道信息
#define	SCZ_HOST_CH0  0
#define	SCZ_EX_CH0 	  1
#define	SCZ_EX_CH1 	  2
#define	SCZ_EX_CH2 	  3
#define	SCZ_EX_CH3 	  4
#define	SCZ_EX_CH4 	  5
#define	SCZ_EX_CH5 	  6
#define	SCZ_EX_CH6 	  7
#define	SCZ_EX_CH7 	  8
#define	SCZ_EX_CH8 	  9
#define	SCZ_EX_CH9 	  10
#define	SCZ_EX_CH10   11
#define	SCZ_EX_CH11	  12
#define	SCZ_EX_CH12   13
#define	SCZ_EX_CH13   14
#define	SCZ_EX_CH14   15
#define	SCZ_EX_CH15   16


// typedef struct					//定义SCZ存储的采样参数结构体
// {
//     unsigned char sam_mode[2];     //采样模式，保留
//     unsigned char local_ch[2];     //主机通道，是否使能
//     unsigned short ex_ch[2];       //扩展器通道
//     unsigned short midfreq[17] ;			//传感器中心频率，0为本机通道，1-16为扩展器通道
// }__attribute__((packed)) vw_cfg_reg_t;



// typedef struct					//定义SCZ采样时用到的采样参数结构体
// {
//     unsigned char sam_mode;     //采样模式，保留
//     unsigned char local_ch;     //主机通道，是否使能
//     unsigned short ex_ch;       //扩展器通道
//     unsigned short midfreq[17] ;			//传感器中心频率，0为本机通道，1-16为扩展器通道
// }__attribute__((packed)) vw_sam_reg_t;



typedef struct					//定义SCZ采样数据缓存结构体
{
    unsigned int vw_u_time[17];     //振弦采样时间戳数据，0为主机，1-16位扩展器
    float        vw_f_data[17];     //振弦频率数据，0为主机数据，1-16位扩展器数据
    float        vw_t_data[17];     //振弦温度数据，0为主机数据，1-16位扩展器数据
    float        vw_e_vlotage[17];  //扩展器电池电压，V,其中数据0不用
}__attribute__((packed)) vw_data_reg_t;

typedef struct                  //定义SCZ上传数据时的数据结构体
{
    float ret_voltage;
    unsigned char ret_ch;
    float ret_freq;
    float ret_temp;
} __attribute__((__packed__)) vw_sam_data_t;


//------------向SPVW采集组件发送的指令结构体,用于解析
//手动采样指令数据结构体
typedef struct
{
    unsigned char mid_freq[19]; //中心频率数据,19个通道，0、1、2对应本地通道，3-19对应扩展器，手动采样频率分为3段，400-1200,1000-2000,1800-3600
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
    unsigned int  unix;     //unix时间
    unsigned char channel;  //通道号
    unsigned char db;       //db值
    unsigned short err_cnt; //错误计数值
    float freq;             //频率值
    float temp;             //温度值
    float e_vol;            //扩展器电池电压
} __attribute__((packed)) SCZ_SAM_SPVW0_DAT_RES_STRU;



//extern vw_cfg_reg_t scz_cfg_par; //采集参数，采集模式、通道配置
//extern vw_sam_reg_t scz_sam_par; //采集参数，采集模式、通道配置

extern struct rt_semaphore one_ch_complete_sem;
extern struct rt_semaphore one_sam_complete_sem;
extern struct rt_semaphore start_sample_sem;

extern unsigned char current_clk_num;

extern unsigned char scz_sam_mode;
extern unsigned char scz_sam_l_ch;	 //本机通道
extern unsigned short scz_sam_e_ch; //扩展通道
extern unsigned short scz_sam_midfreq[17];

//
//  SCZ定义结束
//













typedef struct
{
    //动态库函数指针
    //void *(pfun_)(void);
    //将动态库中的函数逐一装入到这些函数指针中，以备使用
} DL_FUNCS_STRU;

typedef struct
{
    unsigned short d_len;
    unsigned char data[256];
} __attribute__((packed)) busi_cmd_buf_t;

typedef struct
{
    unsigned char save_mode;
    unsigned char sample_range;
    unsigned char sample_odr;
    unsigned char sample_axis;
    rt_mq_t sample_res_mq;
} __attribute__((packed)) bu_sp_sam_cfg_t;

typedef struct
{
    unsigned char sample_range;
    unsigned char sample_odr;
    unsigned char sample_axis;
    unsigned short sample_period;
    rt_mq_t sample_res_mq;
} __attribute__((packed)) bu_sp_get_vib_t; //参数顺序不可改变，必须和采集组件结构体相同

// typedef struct
// {
//     unsigned char cmd;
//     void *p;
//     rt_sem_t cmd_done;
//     void *r_p;
// } __attribute__((__packed__)) bu_sp_cmd_t;

typedef struct
{
    float ret_primary;
    float ret_secondary;
    float ret_angle;
} __attribute__((__packed__)) sp_sam_res_t;

/*****************************************************
*采集组件相关start
*****************************************************/
typedef struct
{
    rt_uint8_t message_cmd;
    rt_uint8_t data;
} __attribute__((__packed__)) SPIL0_RECV_MSG_STRUCT;

//回复采样数据
typedef struct
{
    float ret_primary;
    float ret_secondary;
    float ret_angle;
} __attribute__((__packed__)) sp_sam_data_t;

typedef struct 
{
    rt_uint8_t  message_cmd;
    rt_uint8_t  data;
}__attribute__((__packed__)) bu_sp_cmd_t;
/*****************************************************
 *采集组件相关end
*****************************************************/
typedef struct __attribute__((__packed__))
{
    unsigned char sample_mode;        //采样模式
    rt_err_t error;                   //采样错误
    unix_ms_t sample_begin_timestamp; //采样开始时间戳
    float sample_calibration;         //实际采样频率
    unsigned char axis;               //加速度采样轴
    unsigned char range;              //采样量程
    unsigned char odr;                //采样频率
    char pre_data_name[30];           //保存文件名
} sp_pre_cable_sample_result_t;

typedef struct
{
    unsigned int unix_time;
    unsigned int microsecond;
    unsigned int pkt_num;
    unsigned int seqno;
    float freq;
    unsigned char f_data_count;
    short f_data[F_DATA_COUNT];
    char axis;
    char odr;
} __attribute__((__packed__)) bu_vib_data_t;

typedef union
{
    sp_sam_data_t sam_res;
    sp_pre_cable_sample_result_t pre_cable_sam_res;
} __attribute__((__packed__)) d_res_t;

typedef struct
{
    unsigned char type;
    void *d_p;
} __attribute__((packed)) data_check_t;

enum
{
    REBOOT = 0,
    Q_ALL,
    CFG_CLK,
    CFG_PAR,
} INS_NAME_E;

enum
{
    AXIS_X = 0,
    AXIS_Y = 1,
    AXIS_Z = 2,
    O_100 = 0,
    O_50 = 1,
    R_2G = 0,
} SAM_CFG_MAC_E; //字符在字符串数组中的位置

enum
{
    UP,
    BATCH
} TOPIC_E; //字符在字符串数组中的位置

enum
{
    END = 0,
    READ_DATA,
    SEND_DATA,
    RETRY,
} STATUS;


enum
{
    CODE_SUCCESS = 0,
    CODE_FAIL = 1,
    CODE_ESIZE = 2,
    CODE_ECANCEL = 3,
    CODE_EOFF = 4,
    CODE_EBUSY = 5,
    CODE_EINVAL = 6,
    CODE_ERETRY = 7,
    CODE_ERSERVE = 8,
    CODE_EALREADY = 9,
    CODE_ENOMEM = 10,
    CODE_ENOACK = 11,
} BUSI_ERR_CODE;

extern struct rt_thread thread_busi_cmd_pro;
extern unsigned char thread_busi_cmd_pro_stack[7 * THREAD_STACK_SIZE / 2];
extern unsigned char busi_cmd_pro_pool[BUSI_CMD_PRO_POOL_SIZE];

extern struct rt_thread thread_busi_clock_pro;
extern unsigned char thread_busi_clock_pro_stack[2 * THREAD_STACK_SIZE];

extern rt_mutex_t mtx_com_send;

extern struct rt_thread rthread_sam_msg_pro; //BUSI连续数据处理
extern unsigned char thread_cont_sam_msg_pro_stack[THREAD_STACK_SIZE * 2];


//数据发送线程
extern struct rt_thread thread_vw_data_send_pro;
extern unsigned char thread_vw_data_send_pro_stack[2 * THREAD_STACK_SIZE];
//extern unsigned char busi_data_send_pro_pool[BUSI_CMD_PRO_POOL_SIZE];

extern void BUSI_vw_data_send_pro_entry(void *p);


//创建一个测试线程，用于测试数据采集
extern struct rt_thread thread_vw_sam_test_pro;
extern unsigned char thread_vw_sam_test_pro_stack[2 * THREAD_STACK_SIZE];
extern void BUSI_vw_sam_test_pro_entry(void *p);


extern void BUSI_cmd_pro_entry(void *p);
extern void BUSI_sam_pro_entry(void *p);

extern void BUSI_clock_pro_entry(void *p);
extern void BUSI_clock1_pro_entry(void *p);
extern void BUSI_clock2_pro_entry(void *p);

extern rt_mq_t busi_mq_cmd_pro_pipe;
extern rt_mq_t busi_mq_clk_pro_pipe;
extern unsigned char clk_num;

extern struct rt_messagequeue busi_mq_sam_res_pipe;
extern unsigned int busi_work_time;
extern unsigned int busi_normal_time;
unsigned char busi_com_send_data(unsigned short tag, void *d_p, unsigned char topic);

extern void busi_com_send_ack(unsigned char *cmd_name, unsigned char res, char *cmd_id);

extern unsigned char get_axis_id(unsigned char id);
extern unsigned char get_odr_id(unsigned char id);
extern unsigned char get_range_id(unsigned char id);
extern unsigned char is_busy();
extern void set_busy();
extern void clr_busy();
extern void busi_down_data();
unsigned char stop_sample();
unsigned char data_check(unsigned char type, void *d_p);
unsigned char busi_com_send_msg(unsigned char cmd, unsigned char des, void *pd, unsigned short len);
void BUSI_clock_pro_entry_test(void );
#endif