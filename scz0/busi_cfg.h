#ifndef _BUSI_CFG_H_
#define _BUSI_CFG_H_

#include "common.h"

#define BUSI_INI_FILENAME "/busi.ini"
#define UTCM_CFG_FILENAME "/utcm.cfg"
#define BUSI_CFG_FILENAME "/busi.cfg"
#define BSAM_CFG_FILENAME "/bsam.cfg"

typedef struct					//定义保持寄存器结构体
{
    unsigned short d_addr ;			//设备地址
	unsigned short reboot;			//重启寄存器
    unsigned short d_group;			//器件分组
    unsigned short is_hs;			//是否使用高速采样
    unsigned short is_use_ex;		//是否使用扩展器
    unsigned short ch;				//通道配置，eg:0x0003,使用通道0和通道1进行采样
    unsigned short s_mode;			//采样模式，定时配置采样（0x00）,定时全自动采样（0x01）,单次采样（0x02）
    unsigned short time_duration;	//定时采样间隔（ch*6-65536），单位S
	unsigned short host_ex0_mf;		//主机或扩展器通道0中心频率,默认800，取整（Hz）
	unsigned short host_ex1_mf;		//主机或扩展器通道1中心频率
	unsigned short host_ex2_mf;		//主机或扩展器通道2中心频率
	unsigned short ex3_mf;			//扩展器通道3中心频率，下同
	unsigned short ex4_mf;
	unsigned short ex5_mf;
	unsigned short ex6_mf;
	unsigned short ex7_mf;
	unsigned short ex8_mf;
	unsigned short ex9_mf;
	unsigned short ex10_mf;
	unsigned short ex11_mf;
	unsigned short ex12_mf;
	unsigned short ex13_mf;
	unsigned short ex14_mf;
	unsigned short ex15_mf;
}__attribute__((packed)) hold_reg_t;

typedef enum{	
	DEVICE_ADDR    	= 	1,
	REBOOT			=	0,
	DEVICE_GROUP   	= 	0,
	IS_HS  			= 	0,
	IS_USE_EX		=	0x00,
	CH              =	0x0000,
	S_MODE          =	0x00,
	TD 				=	90,
	HOST_EX0_MF		=	800,
	HOST_EX1_MF		=	800,
	HOST_EX2_MF		=	800,
	EX3_MF			=	800,
	EX4_MF			=	800,
	EX5_MF			=	800,
	EX6_MF			=	800,
	EX7_MF			=	800,
	EX8_MF			=	800,
	EX9_MF			=	800,
	EX10_MF			=	800,
	EX11_MF			=	800,
	EX12_MF			=	800,
	EX13_MF			=	800,
	EX14_MF			=	800,
	EX15_MF			=	800
}hold_reg_def;

//保持寄存器默认值
#define  HOLD_REG_DEF {\			
DEVICE_ADDR,\
REBOOT,\
DEVICE_GROUP,\
IS_HS,\ 		
IS_USE_EX,\	
CH,\          
S_MODE,\      
TD,\ 			
HOST_EX0_MF,\
HOST_EX1_MF,\
HOST_EX2_MF,\
EX3_MF,\
EX4_MF,\
EX5_MF,\
EX6_MF,\
EX7_MF,\
EX8_MF,\
EX9_MF,\
EX10_MF,\
EX11_MF,\
EX12_MF,\
EX13_MF,\
EX14_MF,\
EX15_MF}


typedef struct
{
	unsigned char  PRO_LOG_LVL ;
	unsigned short PRODUCT_TYPE ;
	unsigned short VERSION ;
	unsigned char  RS_BAUD_RATE ;
	unsigned char  RS_4X_LEN  ;
	unsigned char  RS_3X_LEN  ;
}__attribute__((packed)) pro_ini_t;
typedef enum{	
	LOG_LVL       = 7,
	PRODUCT_TYPE  = 0,
	VERSION       = 0,
}pro_ini_def;

#define  PRO_INI_DEF {\
LOG_LVL,\
PRODUCT_TYPE,\
VERSION}

typedef struct					//定义输入寄存器结构体
{
	unsigned short device_type;		//设备类型
	unsigned short version ;		//软件版本号
	unsigned short sn[8];			//SN号
	unsigned short worktime_h;		//工作时间高8位
	unsigned short worktime_l;		//工作时间低8位
	unsigned short HOST_EX_CH0_F;	//主机或扩展器通道0频率值，F*10
	 		 short HOST_EX_CH0_T;	//主机或扩展器通道0温度值，T*100
	unsigned short HOST_EX_CH1_F;	//主机或扩展器通道1频率值，F*10
	 		 short EX_CH1_T;		//扩展器通道1温度值，T*100
	unsigned short HOST_EX_CH2_F;	//主机或扩展器通道2频率值，F*10
	 		 short EX_CH2_T;		//扩展器通道2温度值，T*100
	unsigned short EX_CH3_F;		//扩展器通道3频率值，F*10，下同
	 		 short EX_CH3_T;		//扩展器通道3温度值，T*100，下同
	unsigned short EX_CH4_F;
	 		 short EX_CH4_T;
	unsigned short EX_CH5_F;
	 		 short EX_CH5_T;
	unsigned short EX_CH6_F;
	 		 short EX_CH6_T;
	unsigned short EX_CH7_F;
	 		 short EX_CH7_T;
	unsigned short EX_CH8_F;
	 		 short EX_CH8_T;
	unsigned short EX_CH9_F;
	 		 short EX_CH9_T;
	unsigned short EX_CH10_F;
	 		 short EX_CH10_T;
	unsigned short EX_CH11_F;
	 		 short EX_CH11_T;
	unsigned short EX_CH12_F;
	 		 short EX_CH12_T;
	unsigned short EX_CH13_F;
	 		 short EX_CH13_T;
	unsigned short EX_CH14_F;
	 		 short EX_CH14_T;
	unsigned short EX_CH15_F;
	 		 short EX_CH15_T;
	unsigned short EX_V;			//扩展器电池电压，V*100
}__attribute__((packed)) input_reg_t;


#define	SCZ_HOST_CH0  0
#define	SCZ_HOST_CH1  20
#define	SCZ_HOST_CH2  30
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


enum{
	MAX_DEVICE_ADDR    = 247,
	MAX_ODR            = 18,
    MAX_SAMPLE_MODE    = 4
};

extern pro_ini_t pro_ini;
extern unsigned char sn[SN_LENGTH + 1];
extern hold_reg_t hold_reg;
extern input_reg_t * input_r ;
extern hold_reg_t  * hold_r  ;
extern pro_ini_t pro_ini; ;

extern unsigned char BUSI_sam_cfg_save(hold_reg_t *s_cfg);
extern int get_pro_ini();

extern unsigned char get_sam_cfg(); //获取采样相关参数
extern unsigned char get_sn();

#endif