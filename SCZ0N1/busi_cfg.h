#ifndef _BUSI_CFG_H_
#define _BUSI_CFG_H_
#include "common.h"
#include "busi_pro.h"
#define BUSI_INI_FILENAME "/busi.ini"
#define UTCM_CFG_FILENAME "/utcm.cfg"
#define BUSI_CFG_FILENAME "/busi.cfg"
#define BSAM_CFG_FILENAME "/bsam.cfg"
//#define BSCFG_CFG_FILENAME "/incli.cfg"//采集组件配置
//#define BSPCA1_CFG_FILENAME "/spca1.cfg"
#define BUSI_SCZ_CFG_FILENAME "/scz0.cfg"

typedef struct
{
    unsigned char def;
    unsigned char running_mode;
    unsigned char odr;
    unsigned char range;
    unsigned char axis;

    unsigned char def_s;
    unsigned char running_mode_s;
    unsigned char odr_s;
    unsigned char range_s;
    unsigned char axis_s;
} __attribute__((packed)) busi_sam_cfg_t;



typedef struct					//定义SCZ存储的采样参数结构体
{
    unsigned char sam_mode[2];     //采样模式，保留
    unsigned char local_ch[2];     //主机通道，是否使能
    unsigned short ex_ch[2];       //扩展器通道
    unsigned short midfreq[17] ;			//传感器中心频率，0为本机通道，1-16为扩展器通道
}__attribute__((packed)) vw_cfg_reg_t;



#define SAM_CFG_DEF { \
    DEF,              \
    RUN_MODE,         \
    ODR,              \
    RANGE,            \
    AXIS,             \
                      \
    DEF_S,            \
    RUN_MODE_S,       \
    ODR_S,            \
    RANGE_S,          \
    AXIS_S,           \
};

/**ODR_1000Hz = 2,ODR_500Hz = 3,ODR_250Hz  = 4,ODR_125Hz= 5,ODR_62_5Hz= 6,
    ODR_31_25Hz= 7,ODR_15_625Hz=8,ODR_7_813Hz=9,ODR_3_906Hz =10,
*/
enum
{
    DEF = 1,      //使用系统当前值
    RUN_MODE = 1, //不储存
    ODR = 1,      //50Hz
    RANGE = 0,    //2.5G
    AXIS = 4,     //Z轴

    DEF_S = 1, //使用系统当前值
    RUN_MODE_S = 1,
    ODR_S = 1, //50Hz
    RANGE_S = 0,
    AXIS_S = 4,

    DEF_V = 1, //使用系统当前值
    ODR_V = 1, //50Hz
    RANGE_V = 0,
    AXIS_V = 4,
    PERIOD_V = 600, //10分钟

} e_sam_cfg;
typedef struct
{
    unsigned char PRO_LOG_LVL;
    unsigned short PRODUCT_TYPE;
    unsigned short VERSION;
    unsigned short INTERVAL;
    unsigned char CLK_IS_ON;
    unsigned char CLK_NUM;
    unsigned char BATTERY_TYPE;
    unsigned char BATTERY_NUM;
} __attribute__((packed)) pro_ini_t;

typedef enum
{
    LOG_LVL = 7,
    PRODUCT_TYPE = 258,
    VERSION = 0,
    INTERVAL = 900,
    CLK_IS_ON = 1,
    CLK_NUM = 0,
    BATTERY_TYPE = 0,
    BATTERY_NUM = 2,
} pro_ini_def;

typedef struct
{
    unsigned char is_saved_flag; //标记是否更改配置信息，如果更改配置信息才进行读取
    unsigned char save_mode;
    unsigned char current_id;
    unsigned char is_spca_cfg;
    double kn[3][5];
    unsigned short peaks[5];
} __attribute__((packed)) busp_cable_cfg_t;

#define PRO_INI_DEF                                                  \
    {                                                                \
        LOG_LVL, PRODUCT_TYPE, VERSION, INTERVAL, CLK_IS_ON, CLK_NUM \
    }

extern vw_cfg_reg_t scz_cfg_par;

extern busi_sam_cfg_t sam_cfg;

extern pro_ini_t pro_ini;
extern unsigned char sn[SN_LENGTH + 1];
extern UTC_CLOCK_CONFIG_STRU clock_cfg;

extern unsigned char BUSI_sam_cfg_save(busi_sam_cfg_t *s_cfg);
extern int get_pro_ini();

extern void set_clk1_sam_def_cfg();
extern void set_clk2_sam_def_cfg();
extern void get_vib_def_cfg();
extern unsigned char nodeid;
extern char my_strchr(char const *p, char c);

extern busp_cable_cfg_t cable_cfg;
extern unsigned char get_com_cfg(char *filename, void *d_p, unsigned short d_l);


extern unsigned char get_scz0_cfg(void);
//extern unsigned char save_scz0_cfg(vw_sam_reg_t scz_par);
//extern unsigned char save_scz0_cfg(void);
extern unsigned char save_scz0_cfg(vw_cfg_reg_t *s_cfg);


//extern Init_info_t init_info;
#endif