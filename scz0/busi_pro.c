
#include "rtthread.h"
#include "common.h"
#include "busi_pro.h"
#include "busi_manager.h"
#include "seq_cdumb.h"
#define LOG_TAG "SCZ_PRO"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#include "board.h"
#include <rtthread.h>

//此文件主要是组件的具体内部实现
//mapp_template中的消息数据处理尽可能简短(防止msgpro进程长时间阻塞)
//将消息对应的具体工作抛出到单独的进程中进行处理
unsigned IS_DEBUG = 0;
pro_ini_t pro_ini = PRO_INI_DEF;
const char bbi4[] = "busi.ini";
#define IS_SEND_TEST 0

struct rt_thread thread_busi_modbus_cmd_pro;
unsigned char thread_busi_modbus_cmd_pro_stack[THREAD_STACK_SIZE];
rt_mq_t busi_modbus_cmd_pro_pipe = RT_NULL;
unsigned char busi_modbus_cmd_pro_pool[BUSI_MODBUS_CMD_PRO_POOL_SIZE] = {0};

static rt_sem_t sem_cont_sam_send_done = RT_NULL;
static rt_sem_t sem_busi_msg_send_done = RT_NULL;
static rt_timer_t cont_sam_timer;

static struct rt_messagequeue BUSI_cont_sam_pipe;				//BUSI组件管道
unsigned char BUSI_cont_sam_pool[BUSI_CONT_MAX_MSG_SIZE] = {0}; //BUSI组件管道缓冲
struct rt_thread rthread_cont_sam_msg_pro;						//BUSI连续数据处理
unsigned char thread_cont_sam_msg_pro_stack[THREAD_STACK_SIZE];
void *cdumb = NULL;
seq_cdumb_t *seq_cdumb;
unsigned short pkt_count;

static struct rt_semaphore scz_spvw_sem;
static RESP_STRU scz_spvw_resp;
uint8_t scz_spvw_cmd_dat;



/* 定时器的控制块*/
static struct rt_timer scz0b1_timer1; //定时
/* 定时器1 超时函数,每1000个OS节拍中断一次*/
static uint32_t scz0b1_timer1_count = 0;
static uint8_t scz0b1_sam_sm = 0;
void scz0b1_timeout1(void *parameter)
{
	scz0b1_timer1_count++; //一直计数
	input_r->worktime_h = (scz0b1_timer1_count&0xffff0000)>>16;
	input_r->worktime_l = scz0b1_timer1_count&0xffff;
}
int scz0b1_timer_sample(void)
{
	/* 初始化定时器1 周期定时器*/
	rt_timer_init(&scz0b1_timer1, "scz0b1_timer1", /* 定时器名字是timer1 */
				  scz0b1_timeout1,				   /* 超时时回调的处理函数*/
				  RT_NULL,						   /* 超时函数的入口参数*/
				  1000,							   /* 定时长度， 1000 Tick */
				  RT_TIMER_FLAG_PERIODIC);		   /* 周期性定时器*/
	rt_timer_start(&scz0b1_timer1);
	return 0;
}

void load_input_reg_def()
{
	unsigned char i;
	input_reg.device_type = pro_ini.PRODUCT_TYPE;
	input_reg.version = pro_ini.VERSION;
	rt_memcpy(input_reg.sn, sn, sizeof(sn));
	input_reg.sn[0] = l_to_b_16(input_reg.sn[0]);
	input_reg.sn[1] = l_to_b_16(input_reg.sn[1]);
	input_reg.sn[2] = l_to_b_16(input_reg.sn[2]);
	input_reg.sn[3] = l_to_b_16(input_reg.sn[3]);
	input_reg.sn[4] = l_to_b_16(input_reg.sn[4]);
	input_reg.sn[5] = l_to_b_16(input_reg.sn[5]);
	input_reg.sn[6] = l_to_b_16(input_reg.sn[6]);
	input_reg.sn[7] = l_to_b_16(input_reg.sn[7]);
}

void modbus_cmd_pro_entry(void *p)
{
	modbus_cmd_t cmd;
	ulog_tag_lvl_filter_set(LOG_TAG, pro_ini.PRO_LOG_LVL);
	sem_busi_msg_send_done = rt_sem_create("busi_sem_msg_send_done", 0, RT_IPC_FLAG_FIFO);
	//cont_sam_timer = rt_timer_create("cont_sam_timer", cont_sam_timeout, RT_NULL, 1000 * 60, RT_TIMER_FLAG_ONE_SHOT);
	LOG_D("modbus_cmd_pro_entry running...");
	while (1)
	{
		if (rt_mq_recv(busi_modbus_cmd_pro_pipe, &cmd, sizeof(modbus_cmd_t), RT_WAITING_FOREVER) == RT_EOK) //从OTA_PRO_RECV_PIECE管道获取消息
		{
			LOG_D("rt_mq_recv %d %d", cmd.func, cmd.data_addr);
			switch (cmd.func) //modbus功能码
			{
			case 0x06:				   //写单个保持寄存器
				switch (cmd.data_addr) //寄存器地址
				{
				case HOLD_REG_0:
					hold_reg.d_addr = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_2:
					hold_reg.d_group = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_3:
					hold_reg.is_hs = cmd.data[0];
					LOG_D("hold_reg.is_hs：0x%x", hold_reg.is_hs);
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_4:
					hold_reg.is_use_ex = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					LOG_D("HOLD_REG_4= %d", hold_reg.is_use_ex);
					break;
				case HOLD_REG_5:
					hold_reg.ch = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					LOG_D("cont_dlen = %d", hold_reg.ch);
					break;
				case HOLD_REG_6:
					hold_reg.s_mode = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_7:
					hold_reg.time_duration = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_8:
					hold_reg.host_ex0_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_9:
					hold_reg.host_ex1_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_10:
					hold_reg.host_ex2_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_11:
					hold_reg.ex3_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_12:
					hold_reg.ex4_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_13:
					hold_reg.ex5_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_14:
					hold_reg.ex6_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_15:
					hold_reg.ex7_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_16:
					hold_reg.ex8_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_17:
					hold_reg.ex9_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_18:
					hold_reg.ex10_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_19:
					hold_reg.ex11_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_20:
					hold_reg.ex12_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_21:
					hold_reg.ex13_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_22:
					hold_reg.ex14_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				case HOLD_REG_LAST:
					hold_reg.ex15_mf = cmd.data[0];
					BUSI_sam_cfg_save(&hold_reg);
					break;
				default:
					break;
				}

				break;
			case 0x10:
				switch (cmd.data_addr)
				{
				}
				break;

			default:
				LOG_D("func default");
				break;
			}
		}
	}
}

unsigned char scz_calc_bit_1(unsigned short data)
{
	unsigned char i = 0, n = 0;
	for (i = 0; i < 16; i++)
	{
		if (data & 0x01)
			n++;
		data >>= 1;
	}
	return n;
}

//向主管道发送消息
#define TESTCOMP_REG_SEM_WAIT_TIME (3000)
static int Test_Send_Msg(DM_GMS_STRU *dat)
{
	rt_sem_init(&scz_spvw_sem, "scz_spvw_sem", 0, RT_IPC_FLAG_FIFO);
	rt_mq_send(busi_ctl.p_mainpipe, dat, sizeof(DM_GMS_STRU)); //向主管道发送数据
															   //rt_mq_send(&busi_pipe, dat, sizeof(DM_GMS_STRU)); //向主管道发送数据

	if (RT_EOK == rt_sem_take(&scz_spvw_sem, TESTCOMP_REG_SEM_WAIT_TIME)) //等待接收者释放信号量
	{
		rt_sem_detach(&scz_spvw_sem);
		if (dat->dm_gms[0].d_des == scz_spvw_resp.r_src && 0 == scz_spvw_resp.r_res)
		{
			LOG_I("send cmd suc!");
			scz_spvw_cmd_dat = dat->dm_gms[0].d_cmd.cmd; //将指令赋值给全局变量
			return RT_EOK;
		}
		else
		{
			return RT_ERROR;
		}
	}
	else //超时表示发送失败
	{
		LOG_E("test send cmd to! fatal error ");
		return RT_ERROR;
	}
}

#define scz_test_sample

//数据采集线程入口函数
void scz_sample_thread_entry(void *parameter)
{
	static DM_GMS_STRU scz_spvw_dmgms = {0};
	//static GMS_STRU rsuc_gms = {0};

	unsigned short samp_time_delay = 0;

	int err;

	unsigned char ixx = 0;

	static SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU param; //SPVW输入参数
	static uint32_t scz_sample_timing = 0;

	// unsigned char s_mode=0;
	// unsigned char is_use_ex=0;
	// unsigned char is_hs=0;
	// unsigned char ch=0;

#ifdef scz_test_sample
	char i = 19;
	while (i--)
	{
		param.mid_freq[i] = 800 / 20; //实际频率值除20（整数）
	}
#else
	param.mid_freq[0] = hold_r->host_ex0_mf / 20; //实际频率值除20（整数）
	param.mid_freq[1] = hold_r->host_ex1_mf / 20; //实际频率值除20（整数）
	param.mid_freq[2] = hold_r->host_ex2_mf / 20; //实际频率值除20（整数）
	param.mid_freq[3] = hold_r->ex3_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[4] = hold_r->ex4_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[5] = hold_r->ex5_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[6] = hold_r->ex6_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[7] = hold_r->ex7_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[8] = hold_r->ex8_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[9] = hold_r->ex9_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[10] = hold_r->ex10_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[11] = hold_r->ex11_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[12] = hold_r->ex12_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[13] = hold_r->ex13_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[14] = hold_r->ex14_mf / 20;	  //实际频率值除20（整数）
	param.mid_freq[15] = hold_r->ex15_mf / 20;	  //实际频率值除20（整数）
#endif

	for (ixx = 0; ixx < 2; ixx++)
	{
		rt_thread_mdelay(1000); //启动前延时500ms，等待系统各个参数初始化完成
		LOG_D("scz_sample_thread_entry delay:%ds", ixx);
	}

	hold_r->s_mode = 0x01;		  //定时采样
	hold_r->is_use_ex = 0x00;	  //使用本机通道
	hold_r->is_hs = 0x00;		  //不使用高速采样
	hold_r->ch = 0x07;			  //配置
	hold_r->time_duration = 0x20; //配置

	// s_mode		=	0x03;	//单次采样
	// is_use_ex	=	0x00;	//使用本机通道
	// is_hs		=	0x00;	//不使用高速采样
	// ch 			=	0x01;	//配置

	while (1)
	{

		if (hold_r->s_mode == 0x00) //如果值为0，这说明当前不采样
		{
			rt_thread_mdelay(200); //延时200ms
		}
		else //采样方式为定时自动采样、定时按配置采样、单次采样（采完之后将S_MODE寄存器恢复成0x00，同时向SPVW发送停止采样命令）
		{
			if (hold_r->is_use_ex==0) //使用本机进行采样
			{
				LOG_D("1：scz use host");
				if (hold_r->is_hs) //如果高速采样,仅用通道1进行
				{
					LOG_D("2：scz hs sample");
					while (hold_r->is_hs) //连续发送连续指令，//循环接收数据，同时判断是否接收到停止采样命令
					{
						unsigned short scz_hs_delay =0;
						scz_hs_delay	=	60;
																																								 //采样前归0，采样后
						mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_HS_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
						err = Test_Send_Msg(&scz_spvw_dmgms);
						rt_sem_control(&scz_sam_spvw_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
						rt_sem_take(&scz_sam_spvw_complete_sem,6000);//等待数据收集信号量
					}
				}
				else
				{
					if (hold_r->s_mode == 0x01) //如果定时按配置采样
					{
						LOG_D("3：scz mun sample");

						//定时采样时先执行一次采样
						if (hold_r->ch != 0)
						{
							param.local_ch = hold_r->ch & 0x07;
							param.extend_ch[0] = 0;
							param.extend_ch[1] = 0;
							LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

							mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
							err = Test_Send_Msg(&scz_spvw_dmgms);

							if (err == RT_EOK)
							{
								//hold_r->s_mode = 0x00; //SPVW组件采样任务为单次采样，因此成功后，将保持寄存器值进行切换即可（停止采样），如果不成功，则300ms后再次采样
							}
						}
						while (hold_r->s_mode == 0x01) //如果模式为定时自动采样，则在此处循环
						{

							scz_sample_timing = scz0b1_timer1_count;

							while (((scz0b1_timer1_count - scz_sample_timing) < hold_r->time_duration) && (hold_r->is_use_ex==0) && (hold_r->s_mode == 0x01) &&( hold_r->is_hs == 0x00) ) //判断时间差小于且模式仍为单次模式
							{
								rt_thread_mdelay(100); //每个100ms检查一次模式和时间差
													   //LOG_D("scz0b1_timer1_count:%d",scz0b1_timer1_count);
							}
							if (hold_r->s_mode != 0x01)
							{
								break; //如果设置了新模式，则break，退出循环；
							}
								
							if (hold_r->is_hs != 0x00)
							{	
								break; //如果设置了高速采样，则break，退出循环；
							}

							if (hold_r->is_use_ex != 0x00)
							{	
								break; //如果设置了高速采样，则break，退出循环；
							}


							if (hold_r->ch != 0)
							{
								param.local_ch = hold_r->ch & 0x07;
								param.extend_ch[0] = 0;
								param.extend_ch[1] = 0;
								LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

								mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
								err = Test_Send_Msg(&scz_spvw_dmgms);

								if (err == RT_EOK)
								{
									//hold_r->s_mode = 0x00; //SPVW组件采样任务为单次采样，因此成功后，将保持寄存器值进行切换即可（停止采样），如果不成功，则300ms后再次采样
								}
							}
						}
					}
					else if (hold_r->s_mode == 0x02) //如果定时自动采样
					{
						LOG_D("3：scz auto sample");

						//定时采样时先执行一次采样
						if (hold_r->ch != 0)
						{
							param.local_ch = hold_r->ch & 0x07;
							param.extend_ch[0] = 0;
							param.extend_ch[1] = 0;
							LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

							mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_AUTO_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
							err = Test_Send_Msg(&scz_spvw_dmgms);

							if (err == RT_EOK)
							{
							}
						}

						while (hold_r->s_mode == 0x02) //如果模式为定时自动采样，则在此处循环
						{

							scz_sample_timing = scz0b1_timer1_count;

							while (((scz0b1_timer1_count - scz_sample_timing) < hold_r->time_duration) && (hold_r->is_use_ex==0) && (hold_r->s_mode == 0x02) &&( hold_r->is_hs == 0x00)) //判断时间差小于且模式仍为定时采样模式
							{
								rt_thread_mdelay(100); //每个100ms检查一次模式和时间差
													   //LOG_D("scz0b1_timer1_count:%d",scz0b1_timer1_count);								
							}
							
							if (hold_r->s_mode != 0x02)
							{
								break; //如果设置了新模式，则break，退出循环；
							}
								

							if (hold_r->is_hs != 0x00)
							{	
								break; //如果设置了高速采样，则break，退出循环；
							}

							if (hold_r->is_use_ex != 0x00)
							{	
								break; //如果设置了高速采样，则break，退出循环；
							}
								

							if (hold_r->ch != 0)
							{
								param.local_ch = hold_r->ch & 0x07;
								param.extend_ch[0] = 0;
								param.extend_ch[1] = 0;
								LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

								mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_AUTO_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
								err = Test_Send_Msg(&scz_spvw_dmgms);

								if (err == RT_EOK)
								{
									//hold_r->s_mode = 0x00; //SPVW组件采样任务为单次采样，因此成功后，将保持寄存器值进行切换即可（停止采样），如果不成功，则300ms后再次采样
								}
							}
						}
					}
					else if (hold_r->s_mode == 0x03)//如果单次采样
					{
						unsigned char singel_sample_err_count = 0;

						LOG_D("3：scz signel sample");
						if (hold_r->ch != 0)
						{
							param.local_ch = hold_r->ch & 0x07;
							param.extend_ch[0] = 0;
							param.extend_ch[1] = 0;
							LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

							mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
							err = Test_Send_Msg(&scz_spvw_dmgms);

							if (err == RT_EOK)
							{
								hold_r->s_mode = 0x00; //SPVW组件采样任务为单次采样，因此成功后，将保持寄存器值进行切换即可（停止采样），如果不成功，则300ms后再次采样
							}
							else
							{
								singel_sample_err_count++;
								if (singel_sample_err_count >= 3) //如果失败则重试3次
								{
									rt_thread_mdelay(500); //延时500ms
									singel_sample_err_count = 0;
									hold_r->s_mode = 0x00;
								}
							}
						}
					}
				}
			}
			else if (hold_r->is_use_ex == 0x01) //使用扩展器进行采样
			{
				LOG_D("scz use ex");
				if (hold_r->s_mode == 0x01) //如果定时按配置采样
				{
					LOG_D("3：scz mun sample");

					//定时采样时先执行一次采样
					if (hold_r->ch != 0)
					{
						param.local_ch = 0;
						param.extend_ch[0] = (hold_r->ch & 0xFF00) >> 8;
						param.extend_ch[1] = hold_r->ch & 0xFF;
						LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

						mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
						err = Test_Send_Msg(&scz_spvw_dmgms);

						if (err == RT_EOK)
						{
							//hold_r->s_mode = 0x00; //SPVW组件采样任务为单次采样，因此成功后，将保持寄存器值进行切换即可（停止采样），如果不成功，则300ms后再次采样
						}
					}
					while (hold_r->s_mode == 0x01) //如果模式为定时自动采样，则在此处循环
					{

						scz_sample_timing = scz0b1_timer1_count;

						while (((scz0b1_timer1_count - scz_sample_timing) < hold_r->time_duration) && (hold_r->is_use_ex== 0x01) && (hold_r->s_mode == 0x01)) //判断时间差小于且模式仍为单次模式
						{
							rt_thread_mdelay(100); //每个100ms检查一次模式和时间差
												   //LOG_D("scz0b1_timer1_count:%d",scz0b1_timer1_count);
						}
						if (hold_r->s_mode != 0x01)
						{
							break; //如果设置了新模式，则break，退出循环；
						}

						if (hold_r->is_use_ex != 0x01)
						{	
							break; //如果设置了高速采样，则break，退出循环；
						}

						if (hold_r->ch != 0)
						{
							param.local_ch = 0;
							param.extend_ch[0] = (hold_r->ch & 0xFF00) >> 8;
							param.extend_ch[1] = hold_r->ch & 0xFF;
							LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

							mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
							err = Test_Send_Msg(&scz_spvw_dmgms);

							if (err == RT_EOK)
							{
								//hold_r->s_mode = 0x00; //SPVW组件采样任务为单次采样，因此成功后，将保持寄存器值进行切换即可（停止采样），如果不成功，则300ms后再次采样
							}
						}
					}
				}
				else if (hold_r->s_mode == 0x02) //如果定时自动采样
				{
					LOG_D("3：scz auto sample");

					//定时采样时先执行一次采样
					if (hold_r->ch != 0)
					{
						param.local_ch = 0;
						param.extend_ch[0] = (hold_r->ch & 0xFF00) >> 8;
						param.extend_ch[1] = hold_r->ch & 0xFF;
						LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

						mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_AUTO_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
						err = Test_Send_Msg(&scz_spvw_dmgms);

						if (err == RT_EOK)
						{
						}
					}

					while (hold_r->s_mode == 0x02) //如果模式为定时自动采样，则在此处循环
					{
						scz_sample_timing = scz0b1_timer1_count;

						while (((scz0b1_timer1_count - scz_sample_timing) < hold_r->time_duration) && (hold_r->is_use_ex== 0x01) && (hold_r->s_mode == 0x02)) //判断时间差小于且模式仍为单次模式
						{
							rt_thread_mdelay(100); //每个100ms检查一次模式和时间差
												   //LOG_D("scz0b1_timer1_count:%d",scz0b1_timer1_count);
						}
						if (hold_r->s_mode != 0x02)
						{
							break; //如果设置了新模式，则break，退出循环；
						}

						if (hold_r->is_use_ex != 0x01)
						{	
							break; //如果设置了高速采样，则break，退出循环；
						}

						if (hold_r->ch != 0)
						{
							param.local_ch = 0;
							param.extend_ch[0] = (hold_r->ch & 0xFF00) >> 8;
							param.extend_ch[1] = hold_r->ch & 0xFF;
							LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

							mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_AUTO_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
							err = Test_Send_Msg(&scz_spvw_dmgms);

							if (err == RT_EOK)
							{
								//hold_r->s_mode = 0x00; //SPVW组件采样任务为单次采样，因此成功后，将保持寄存器值进行切换即可（停止采样），如果不成功，则300ms后再次采样
							}
						}
					}
				}
				else //如果单次采样
				{
					unsigned char singel_sample_err_count = 0;

					LOG_D("3：scz signel sample");
					if (hold_r->ch != 0)
					{
						param.local_ch = 0;
						param.extend_ch[0] = (hold_r->ch & 0xFF00) >> 8;
						param.extend_ch[1] = hold_r->ch & 0xFF;
						LOG_D("local=0x%x,ex0=0x%x,ex1=0x%x", param.local_ch, param.extend_ch[0], param.extend_ch[1]);

						mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, SCZ_BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
						err = Test_Send_Msg(&scz_spvw_dmgms);

						if (err == RT_EOK)
						{
							hold_r->s_mode = 0x00; //SPVW组件采样任务为单次采样，因此成功后，将保持寄存器值进行切换即可（停止采样），如果不成功，则300ms后再次采样
						}
						else
						{
							singel_sample_err_count++;
							if (singel_sample_err_count >= 3) //如果失败则重试3次
							{
								rt_thread_mdelay(500); //延时500ms
								singel_sample_err_count = 0;
								hold_r->s_mode = 0x00;
							}
						}
					}
				}
			}
			else
			{
				LOG_D("1：scz err s_mode=0x%x", hold_r->s_mode);
			}
		}

		rt_thread_mdelay(200); //延时200ms
	}
}