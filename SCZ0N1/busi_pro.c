#include "rtthread.h"

#define LOG_TAG "BUSI_PRO"
#define LOG_LVL LOG_LVL_DBG

#include "busi_cfg.h"
#include "common.h"
#include "busi_pro.h"
#include "busi_manager.h"
#include "seq_cdumb.h"
#include <ulog.h>
#include <rtthread.h>
#include "board.h"
#include "drv_common.h"
#include "self_test.h"
#include "rtdevice.h"
//此文件主要是组件的具体内部实现
//mapp_template中的消息数据处理尽可能简短(防止msgpro进程长时间阻塞)
//将消息对应的具体工作抛出到单独的进程中进行处理

#define IS_SEND_TEST 0

//命令接收线程
struct rt_thread thread_busi_cmd_pro;
unsigned char thread_busi_cmd_pro_stack[7 * THREAD_STACK_SIZE / 2];
rt_mq_t busi_mq_cmd_pro_pipe = RT_NULL;
rt_mq_t busi_mq_clk_pro_pipe = RT_NULL;

//时钟条目线程
struct rt_thread thread_busi_clock_pro;
unsigned char thread_busi_clock_pro_stack[2 * THREAD_STACK_SIZE];
unsigned char busi_cmd_pro_pool[BUSI_CMD_PRO_POOL_SIZE] = {0};

static rt_timer_t clr_busy_timer;
static unsigned char ret_count; //采样超时计数
struct rt_messagequeue busi_mq_sam_res_pipe;

//采样数据接收线程
unsigned char BUSI_cont_sam_pool[BUSI_CONT_MAX_MSG_SIZE] = {0}; //BUSI组件管道缓冲
struct rt_thread rthread_sam_msg_pro;
unsigned char thread_cont_sam_msg_pro_stack[THREAD_STACK_SIZE * 2];
unsigned char clk_num;
rt_mutex_t mtx_com_send = RT_NULL;

//数据发送线程，在完成一轮采样后，统一进行数据发送
struct rt_thread thread_vw_data_send_pro;
unsigned char thread_vw_data_send_pro_stack[2 * THREAD_STACK_SIZE];
//unsigned char busi_data_send_pro_pool[BUSI_CMD_PRO_POOL_SIZE] = {0};

//创建一个测试线程，用于测试数据采集
struct rt_thread thread_vw_sam_test_pro;
unsigned char thread_vw_sam_test_pro_stack[2 * THREAD_STACK_SIZE];

unsigned char current_clk_num = 1; //当前正在运行的时钟条目(1-2)

unsigned int busi_work_time = 0;
unsigned int busi_normal_time = 0;
unix_ms_t busi_unix;

char *ins_name[] = {
	"RebootSensorNode",
	"QueryAllInfoSensorNode",
	"ConfigClockTaskSCZ0F1",
	"ConfigVibraWireParameterSCZ0F1"}; //下行指令接口
bool is_sampling = RT_FALSE;

void *cdumb = NULL;
seq_cdumb_t *seq_cdumb;
unsigned short pkt_count;
SCZ_SAM_SPVW0_DAT_RES_STRU mq_sam_res;

//超时定时器，避免一直处于忙状态
void cont_sam_timeout(void *parameter)
{

	unsigned char res_msg = 0;
	LOG_E("sam_time_out%d", ret_count);
	ret_count++;
	clr_busy();
	if (ret_count > 3)
	{
		ret_count = 0;
		LOG_E("busy reset");
		adbs_set_bkpreg(0, (11 | 0x12340000));
		rt_hw_cpu_reset();
	}
}

//指令接收线程
void BUSI_cmd_pro_entry(void *p)
{
	busi_cmd_buf_t cmd;
	pb_istream_t stream;
	UTC_CLOCK_ENTRY_STRU clk_tmp;
	static char msg_res;
	LOG_W("request%d", __LINE__);
	rt_pm_request(PM_SLEEP_MODE_NONE);
	SensorMessages_SensorDownMessage msg = SensorMessages_SensorDownMessage_init_zero;

	static unsigned char down_buf[SensorMessages_SensorDownMessage_size] = {0};

	ulog_tag_lvl_filter_set(LOG_TAG, pro_ini.PRO_LOG_LVL);
	clr_busy_timer = rt_timer_create("clr_busy_timer", cont_sam_timeout, RT_NULL, 1000 * 135, RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
	LOG_D("cmdprorun");
	static bu_sp_cmd_t bs_set_cfg;
	while (1)
	{
		if (rt_mq_recv(busi_mq_cmd_pro_pipe, &cmd, sizeof(busi_cmd_buf_t), RT_WAITING_FOREVER) == RT_EOK) //从OTA_PRO_RECV_PIECE管道获取消息
		{
			// pb_istream_t stream;
			// UTC_CLOCK_ENTRY_STRU clk_tmp;
			// static char msg_res;
			// LOG_W("request%d", __LINE__);
			// rt_pm_request(PM_SLEEP_MODE_NONE);
			// SensorMessages_SensorDownMessage msg = SensorMessages_SensorDownMessage_init_zero;

			// unsigned char down_buf[SensorMessages_SensorDownMessage_size] = {0};

			rt_memcpy(down_buf, cmd.data, cmd.d_len);
			stream = pb_istream_from_buffer(down_buf, cmd.d_len);

			//stream = pb_istream_from_buffer(cmd.data, cmd.d_len);

			LOG_E("cmd_len=%d", cmd.d_len);
			LOG_HEX("receive_cmd", 16, cmd.data, cmd.d_len);

			rt_enter_critical();
			if (!pb_decode(&stream, SensorMessages_SensorDownMessage_fields, &msg))
			{
				LOG_E("de fail%d", __LINE__);
				busi_com_send_ack(msg.instruction_name, CODE_ENOACK, msg.instruction_id);
				LOG_W("release=%d", __LINE__);
				rt_pm_release(PM_SLEEP_MODE_NONE);
				rt_exit_critical();
				continue;
			}
			rt_exit_critical();
			LOG_W("ins_name=%s", msg.instruction_name);
			LOG_W("gw_id=%s", msg.gateway_id);
			LOG_W("id=%d", msg.node_id);
			LOG_W("instruction_id=%s", msg.instruction_id);

			if (strstr(ins_name[CFG_CLK], msg.instruction_name)) //如果是配置时钟条目
			{
				LOG_W("sample_pattern=%d", msg.msg.configclocktask.sample_pattern);
				LOG_D("local_channel_bit=%d", msg.msg.configclocktask.local_channel_bit);
				LOG_W("ext_channel_bit=%d", msg.msg.configclocktask.ext_channel_bit);
				LOG_D("is_on=%d", msg.msg.configclocktask.is_on);
				LOG_D("clock_num=%d", msg.msg.configclocktask.clock_num);
				LOG_D("hour=%d", msg.msg.configclocktask.hour);
				LOG_D("minute_t=%d", msg.msg.configclocktask.minute);
				LOG_D("second=%d", msg.msg.configclocktask.second);
				LOG_D("interval=%d", msg.msg.configclocktask.interval);
				LOG_D("number=%d", msg.msg.configclocktask.number);

				if (data_check(CFG_CLK, &msg.msg.configclocktask)) ////时钟条目参数检测，不符合直接退出。
				{
					busi_com_send_ack(msg.instruction_name, CODE_EINVAL, msg.instruction_id);
					LOG_W("release=%d", __LINE__);
					rt_pm_release(PM_SLEEP_MODE_NONE);
					continue;
				}
				// if (init_info.incli_marked == 0 && msg.msg.configclocktask.is_on == 1)
				// {
				// 	busi_com_send_ack(msg.instruction_name, CODE_FAIL, msg.instruction_id);
				// 	LOG_W("release=%d", __LINE__);
				// 	rt_pm_release(PM_SLEEP_MODE_NONE);
				// 	continue;
				// }

				//更新对应的采样参数，同时将其保存在CFG文件中
				clk_tmp.is_on = msg.msg.configclocktask.is_on; //更新时钟条目
				clk_tmp.clock = msg.msg.configclocktask.clock_num;
				clk_tmp.hour = msg.msg.configclocktask.hour;
				clk_tmp.minute = msg.msg.configclocktask.minute;
				clk_tmp.second = msg.msg.configclocktask.second;
				clk_tmp.interval = msg.msg.configclocktask.interval;
				if (clk_tmp.interval < 2)
					clk_tmp.interval = 2;
				clk_tmp.number = msg.msg.configclocktask.number;

				if (msg.msg.configclocktask.clock_num == 1) //时钟条目1
				{
					get_scz0_cfg(); //获取SCZ的配置参数

					scz_cfg_par.sam_mode[0] = msg.msg.configclocktask.sample_pattern; //更新参数
					scz_cfg_par.local_ch[0] = msg.msg.configclocktask.local_channel_bit;
					scz_cfg_par.ex_ch[0] = msg.msg.configclocktask.ext_channel_bit;

					save_scz0_cfg(&scz_cfg_par);

					rt_memcpy(&clock_cfg.clockEntry1, &clk_tmp, sizeof(clk_tmp));

					busi_com_send_msg(CP_CMD_DST(UTC_CMD_CFG_CE), MB_STATN_UTCM, &clock_cfg.clockEntry1, sizeof(UTC_CLOCK_ENTRY_STRU));
				}
				else if (msg.msg.configclocktask.clock_num == 2) //时钟条目2
				{
					get_scz0_cfg(); //获取SCZ的配置参数

					scz_cfg_par.sam_mode[1] = msg.msg.configclocktask.sample_pattern; //更新参数
					scz_cfg_par.local_ch[1] = msg.msg.configclocktask.local_channel_bit;
					scz_cfg_par.ex_ch[1] = msg.msg.configclocktask.ext_channel_bit;
					save_scz0_cfg(&scz_cfg_par);

					rt_memcpy(&clock_cfg.clockEntry2, &clk_tmp, sizeof(clk_tmp));
					busi_com_send_msg(CP_CMD_DST(UTC_CMD_CFG_CE), MB_STATN_UTCM, &clock_cfg.clockEntry2, sizeof(UTC_CLOCK_ENTRY_STRU));
				}
				else if (msg.msg.configclocktask.clock_num == 0) //时钟条目0
				{
					rt_memcpy(&clock_cfg.clockEntry0, &clk_tmp, sizeof(clk_tmp));
					busi_com_send_msg(CP_CMD_DST(UTC_CMD_CFG_CE), MB_STATN_UTCM, &clock_cfg.clockEntry0, sizeof(UTC_CLOCK_ENTRY_STRU));
				}
				else
				{
					busi_com_send_ack(msg.instruction_name, CODE_EINVAL, msg.instruction_id);
					rt_pm_release(PM_SLEEP_MODE_NONE);
					continue;
				}
				busi_com_send_ack(msg.instruction_name, RT_EOK, msg.instruction_id);
			}
			else if (strstr(ins_name[REBOOT], msg.instruction_name)) //如果是重启指令
			{
				unsigned int tmp;
				busi_com_send_ack(msg.instruction_name, RT_EOK, msg.instruction_id);
				adbs_set_bkpreg(0, (10 | 0x12340000));
				rt_thread_mdelay(1);
				tmp = adbs_read_bkpreg(0);
				LOG_D("reboot=%x,%d", (tmp & 0xFFFF0000), (tmp & 0x0000FFFF));
				rt_thread_mdelay(30 * 1000); //等待ack传完
				rt_hw_cpu_reset();
			}
			else if (strstr(ins_name[Q_ALL], msg.instruction_name)) //如果是查询所有信息
			{
				rt_thread_mdelay(3);

				busi_com_send_data(SensorMessages_SensorUpMessage_topinfonodedata_tag, NULL, UP); //发送top信息
				LOG_D("SensorMessages_SensorUpMessage_topinfonodedata_tag");
				rt_thread_mdelay(3);

				busi_com_send_data(SensorMessages_SensorUpMessage_clocktasktimeinfosensornode_tag, NULL, UP); //发送时钟任务信息
				LOG_D("SensorMessages_SensorUpMessage_clocktasktimeinfosensornode_tag");
				rt_thread_mdelay(3);

				busi_com_send_data(SensorMessages_SensorUpMessage_clocktasksampleinfo_tag, NULL, UP);
				LOG_D("SensorMessages_SensorUpMessage_clocktasksampleinfo_tag");
				rt_thread_mdelay(3);

				busi_com_send_data(SensorMessages_SensorUpMessage_vibrawireinfo_tag, NULL, UP);
				LOG_D("SensorMessages_SensorUpMessage_vibrawireinfo_tag");
			}
			else if (strstr(ins_name[CFG_PAR], msg.instruction_name)) //如果是配置传感器参数
			{
				// LOG_E("recv configvibrawireparameter");
				// LOG_E("msg.msg.configvibrawireparameter.ch0_midfreq：%d", msg.msg.configvibrawireparameter.ch0_midfreq);
				// LOG_E("msg.msg.configvibrawireparameter.ch1_midfreq：%d", msg.msg.configvibrawireparameter.ch1_midfreq);
				// LOG_E("msg.msg.configvibrawireparameter.ch2_midfreq：%d", msg.msg.configvibrawireparameter.ch2_midfreq);
				// LOG_E("msg.msg.configvibrawireparameter.ch3_midfreq：%d", msg.msg.configvibrawireparameter.ch3_midfreq);
				// LOG_E("msg.msg.configvibrawireparameter.ch4_midfreq：%d", msg.msg.configvibrawireparameter.ch4_midfreq);

				if (data_check(CFG_PAR, &msg.msg.configvibrawireparameter)) //检查传感器参数，如果合法，则存储在CFG文件中
				{
					LOG_E("data_check err 2");
					busi_com_send_ack(msg.instruction_name, CODE_EINVAL, msg.instruction_id);
					rt_pm_release(PM_SLEEP_MODE_NONE);
					continue;
				}

				//get_scz0_cfg(); //获取SCZ的配置参数

				scz_cfg_par.midfreq[0] = msg.msg.configvibrawireparameter.ch0_midfreq; //更新参数
				scz_cfg_par.midfreq[1] = msg.msg.configvibrawireparameter.ch1_midfreq;
				scz_cfg_par.midfreq[2] = msg.msg.configvibrawireparameter.ch2_midfreq;
				scz_cfg_par.midfreq[3] = msg.msg.configvibrawireparameter.ch3_midfreq;
				scz_cfg_par.midfreq[4] = msg.msg.configvibrawireparameter.ch4_midfreq;
				scz_cfg_par.midfreq[5] = msg.msg.configvibrawireparameter.ch5_midfreq;
				scz_cfg_par.midfreq[6] = msg.msg.configvibrawireparameter.ch6_midfreq;
				scz_cfg_par.midfreq[7] = msg.msg.configvibrawireparameter.ch7_midfreq;
				scz_cfg_par.midfreq[8] = msg.msg.configvibrawireparameter.ch8_midfreq;
				scz_cfg_par.midfreq[9] = msg.msg.configvibrawireparameter.ch9_midfreq;
				scz_cfg_par.midfreq[10] = msg.msg.configvibrawireparameter.ch10_midfreq;
				scz_cfg_par.midfreq[11] = msg.msg.configvibrawireparameter.ch11_midfreq;
				scz_cfg_par.midfreq[12] = msg.msg.configvibrawireparameter.ch12_midfreq;
				scz_cfg_par.midfreq[13] = msg.msg.configvibrawireparameter.ch13_midfreq;
				scz_cfg_par.midfreq[14] = msg.msg.configvibrawireparameter.ch14_midfreq;
				scz_cfg_par.midfreq[15] = msg.msg.configvibrawireparameter.ch15_midfreq;
				scz_cfg_par.midfreq[16] = msg.msg.configvibrawireparameter.ch16_midfreq;

				LOG_E("main_direction=%d", msg.msg.configvibrawireparameter.ch0_midfreq);
				
				save_scz0_cfg(&scz_cfg_par);

				if (msg_res == 0)
				{
					busi_com_send_ack(msg.instruction_name, RT_EOK, msg.instruction_id);
				}
				else
				{
					LOG_D("send msg err=%d", msg_res);
					busi_com_send_ack(msg.instruction_name, CODE_FAIL, msg.instruction_id);
				}
			}
			else
			{
				LOG_E("cmd err");
				busi_com_send_ack(msg.instruction_name, CODE_EINVAL, msg.instruction_id);
			}
			LOG_W("release=%d", __LINE__);
		}
		rt_pm_release(PM_SLEEP_MODE_NONE);
	}
}

/**
 * 时钟条目处理函数，根据时钟条目及配置进行采样
 */

void BUSI_clock_pro_entry(void *p)
{
	LOG_D("clkprorun");

	// bu_sp_cmd_t sam_cmd;
	bu_sp_sam_cfg_t bu_sp_sam_cfg;
	// sam_cmd.cmd = SPCA_CMD_CABLE_SAMPLE;
	// sam_cmd.cmd_done = RT_NULL;
	// sam_cmd.r_p = RT_NULL;
	//static bu_sp_cmd_t bs_get_sam;
	static unsigned char clk_num_tmp;
	while (1)
	{
		if (rt_mq_recv(busi_mq_clk_pro_pipe, &clk_num_tmp, sizeof(clk_num_tmp), RT_WAITING_FOREVER) == RT_EOK)
		{
			switch (clk_num_tmp)
			{
			case 0: //时钟条目0
				LOG_W("clk_num:%d", clk_num_tmp);
				// if (is_busy())
				// {
				// 	LOG_W("release=%d", __LINE__);
				// 	rt_pm_release(PM_SLEEP_MODE_NONE);
				// 	break;
				// }
				busi_com_send_data(SensorMessages_SensorUpMessage_topinfonodedata_tag, NULL, UP);
				LOG_W("release=%d", __LINE__);
				rt_pm_release(PM_SLEEP_MODE_NONE);
				break;
			case 1:
			{
				LOG_W("clk_num:%d", clk_num_tmp);
				if (is_busy())
				{
					LOG_W("release=%d", __LINE__);
					rt_pm_release(PM_SLEEP_MODE_NONE);
					break;
				}
				else
				{
					set_busy();
				}
				current_clk_num = 1; //将正在运行的条目改为1

				// scz_cfg_par.sam_mode[0] = 0; //更新参数
				// scz_cfg_par.local_ch[0] = 0;
				// scz_cfg_par.ex_ch[0] = 0x8001;
				// save_scz0_cfg(&scz_cfg_par);
				// memset(&scz_cfg_par,0,sizeof(scz_cfg_par));
				// get_scz0_cfg();

				scz_sam_mode = scz_cfg_par.sam_mode[0];
				scz_sam_l_ch = scz_cfg_par.local_ch[0];
				scz_sam_e_ch = scz_cfg_par.ex_ch[0];

				rt_memcpy(&scz_sam_midfreq, &scz_cfg_par.midfreq[0], sizeof(scz_sam_midfreq));
				// {
				// 	unsigned char i=0;
				// 	for(i=0;i<17;i++)
				// 	{
				// 		LOG_W("scz_sam_midfreq[%d]:%d",i,scz_sam_midfreq[i]);
				// 	}
				// }
				
				//LOG_E("clk1 send start_sample_sem,l_ch:0x%x,e_ch:0x%x", scz_sam_l_ch, scz_sam_e_ch);

				rt_sem_release(&start_sample_sem); //处理完成后发送信号量
				rt_timer_start(clr_busy_timer);
			}
			break;
			case 2:
				LOG_W("clk_num:%d", clk_num_tmp);
				if (is_busy())
				{
					LOG_W("release=%d", __LINE__);
					rt_pm_release(PM_SLEEP_MODE_NONE);
					break;
				}
				else
				{
					//set_busy();
				}

				//rt_sem_release(&start_sample_sem); //处理完成后发送信号量
				/*
				bu_sp_sam_cfg.sample_axis = sam_cfg.axis_s;
				bu_sp_sam_cfg.sample_odr = sam_cfg.odr_s;
				bu_sp_sam_cfg.sample_range = sam_cfg.range_s;
				bu_sp_sam_cfg.save_mode = 1;
				bu_sp_sam_cfg.sample_res_mq = &busi_mq_sam_res_pipe;
				sam_cmd.p = &bu_sp_sam_cfg;
				*/
				//
				//rt_timer_start(clr_busy_timer);

				break;
			default:
				break;
			}
		}
	}
}

/*************************************************************************
 * 
 *  振弦数据采集部分
 * 
 * ***********************************************************************
*/

//vw_sam_reg_t scz_sam_par; //采集参数，采集模式、通道配置

//vw_par_reg_t scz_sen_par;				  //传感器参数，中心频率等
struct rt_semaphore one_ch_complete_sem;  //创建一个信号量，当数据接收线程收到一个通道的数据时，就释放一下信号量，同步消息至采集线程
struct rt_semaphore one_sam_complete_sem; //创建一个信号量，当一轮采集结束后，发送该信号量，以通知数据发送线程进行数据发送
struct rt_semaphore start_sample_sem;	  //创建一个信号量，当时钟条目触发后，发送信号量，以开始采样。

vw_data_reg_t vw_data = {0}; //振弦最终采样数据,缓存数据
//vw_data_reg_t vw_data1 = {0}; //振弦最终采样数据,时钟条目1
//vw_data_reg_t vm_data2 = {0}; //振弦最终采样数据,时钟条目2

static struct rt_semaphore scz_spvw_sem; //信号量，用于和SPVW进行同步
static RESP_STRU scz_spvw_resp;			 //返回值指针
uint8_t scz_spvw_cmd_dat;				 //SPVW命令类型

//向主管道发送消息
#define TESTCOMP_REG_SEM_WAIT_TIME (3000)
static int Test_Send_Msg(DM_GMS_STRU *dat)
{
	rt_sem_init(&scz_spvw_sem, "scz_spvw_sem", 0, RT_IPC_FLAG_FIFO);
	rt_mq_send(busi_ctl.p_mainpipe, dat, sizeof(DM_GMS_STRU));			  //向主管道发送数据
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

//数据收集线程，该线程用于收集SPVW发送过来的数据
void BUSI_sam_pro_entry(void *p)
{
	rt_err_t res = 0;
	uint32_t tmp;
	//sp_sam_data_t d_res;
	// bu_vib_data_t d_vib = {0};
	// unsigned int l_unix_tmp = 0;
	// unsigned int l_subtime = 0;
	// double d_tmp = 0;

	LOG_D("sam_pro_start");

	rt_sem_init(&one_ch_complete_sem, "one_ch_complete_sem", 0, RT_IPC_FLAG_FIFO); //创建信号量

	res = rt_mq_init(&busi_mq_sam_res_pipe, "BUSI_mq_res", BUSI_cont_sam_pool, sizeof(SCZ_SAM_SPVW0_DAT_RES_STRU), sizeof(BUSI_cont_sam_pool), RT_IPC_FLAG_FIFO); //建立BUSI管道
	if (res != RT_EOK)
	{
		LOG_E("BUSImqinifail!");
		return;
	}
	while (1)
	{
		if (rt_mq_recv(&busi_mq_sam_res_pipe, &mq_sam_res, sizeof(mq_sam_res), RT_WAITING_FOREVER) == RT_EOK) //如果收到时钟条目给出的采集信息，则执行一次采集任务
		//if (rt_mq_recv(&scz_rx_spvw_dat_mq, &mq_sam_res, sizeof(mq_sam_res), RT_WAITING_FOREVER) == RT_EOK)//如果收到时钟条目给出的采集信息，则执行一次采集任务
		{
			static char status;
			static unsigned char retry_count;
			busi_normal_time = 0; //接收到采样数据，异常处理清零。

			//LOG_D("BUSI_sam_pro_entry rx dat");

			get_rtc_ms_timestamp(&busi_unix);
			mq_sam_res.unix = busi_unix.unix_time;

			ret_count = 0;
			LOG_W("mq_sam_res.channel!!!!!!!!!!!!!!!!:%d", mq_sam_res.channel);

			if (mq_sam_res.channel <= 16)
			{
				vw_data.vw_u_time[mq_sam_res.channel] = mq_sam_res.unix;
				if ((mq_sam_res.freq >= 400) && (mq_sam_res.freq <= 3600))
				{
					vw_data.vw_f_data[mq_sam_res.channel] = mq_sam_res.freq;
				}
				else
				{
					vw_data.vw_f_data[mq_sam_res.channel] = 0;
				}
				if ((mq_sam_res.temp >= -40) && (mq_sam_res.temp <= 80))
				{
					vw_data.vw_t_data[mq_sam_res.channel] = mq_sam_res.temp;
				}
				else
				{
					vw_data.vw_t_data[mq_sam_res.channel] = -85;
				}
				vw_data.vw_e_vlotage[mq_sam_res.channel] = mq_sam_res.e_vol;
			}

			//LOG_D("mq_sam_res unix:%d, channel:%d, freq:%f, temp:%f, e_vol:%f", mq_sam_res.unix, mq_sam_res.channel, mq_sam_res.freq, mq_sam_res.temp, mq_sam_res.e_vol);
			LOG_D("vw_data unix:%d, channel:%d, freq:%f, temp:%f, e_vol:%f", vw_data.vw_u_time[mq_sam_res.channel], mq_sam_res.channel, vw_data.vw_f_data[mq_sam_res.channel], vw_data.vw_t_data[mq_sam_res.channel], vw_data.vw_e_vlotage[mq_sam_res.channel]);

			//busi_com_send_data(SensorMessages_SensorUpMessage_inclinationdata_tag, &mq_sam_res, UP);
			//clr_busy();

			LOG_W("release=%d", __LINE__);
			rt_pm_release(PM_SLEEP_MODE_NONE);
		}
	}
}

//数据发送线程，完成一轮采集后，集中进行数据发送
void BUSI_vw_data_send_pro_entry(void *p)
{
	rt_err_t res = 0;
	uint32_t tmp = 0x01;
	unsigned char i = 1;
	vw_sam_data_t vm_dat = {0};

	LOG_D("BUSI_vw_data_send_pro_entry");

	rt_sem_init(&one_sam_complete_sem, "one_sam_complete_sem", 0, RT_IPC_FLAG_FIFO); //创建信号量

	while (1)
	{
		//发送时根据配置的通道信息进行数据发送

		rt_sem_control(&one_sam_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
		rt_sem_take(&one_sam_complete_sem, RT_WAITING_FOREVER);			  //等待数据收集信号量

		{
			LOG_D("rx_one_sam_complete_sem,clk_num:%d", current_clk_num);

			if (scz_sam_l_ch >= 1) //主机通道采样
			{
				vm_dat.ret_ch = 0;
				vm_dat.ret_voltage = vw_data.vw_e_vlotage[0];
				vm_dat.ret_freq = vw_data.vw_f_data[0];
				vm_dat.ret_temp = vw_data.vw_t_data[0];

				LOG_D("vw_data_send unix:%d, channel:%d, freq:%f, temp:%f, e_vol:%f", vw_data.vw_u_time[0], 0, vw_data.vw_f_data[0], vw_data.vw_t_data[0], vw_data.vw_e_vlotage[0]);
				busi_com_send_data(SensorMessages_SensorUpMessage_vibrawiredata_tag, &vm_dat, UP);
				rt_thread_mdelay(3);
			}
			else
			{
				if (scz_sam_e_ch != 0)
				{
					tmp = 0x01;
					for (i = 1; i <= 16; i++)
					{
						if ((scz_sam_e_ch & tmp) != 0)
						{
							vm_dat.ret_ch = i;

							vm_dat.ret_voltage = vw_data.vw_e_vlotage[i];
							vm_dat.ret_freq = vw_data.vw_f_data[i];
							vm_dat.ret_temp = vw_data.vw_t_data[i];

							LOG_D("vw_data_send unix:%d, channel:%d, freq:%f, temp:%f, e_vol:%f", vw_data.vw_u_time[i], i, vw_data.vw_f_data[i], vw_data.vw_t_data[i], vw_data.vw_e_vlotage[i]);
							busi_com_send_data(SensorMessages_SensorUpMessage_vibrawiredata_tag, &vm_dat, UP);
							rt_thread_mdelay(3);
						}
						tmp = tmp << 1;
					}
				}
			}
			rt_timer_stop(clr_busy_timer); //发送完成就停止忙检测定时器

			clr_busy(); //清除忙标志

			rt_thread_mdelay(10);
		}
	}
}

#define scz_wait_vw_time 8100

unsigned char scz_sam_mode = 0;
unsigned char scz_sam_l_ch = 0;	 //本机通道
unsigned short scz_sam_e_ch = 0; //扩展通道
unsigned short scz_sam_midfreq[17] = {0};

//振弦数据采集测试线程，封装为一个函数，在时钟条目线程中进行调用
void BUSI_vw_sam_test_pro_entry(void *p)
{
	static DM_GMS_STRU scz_spvw_dmgms = {0};
	static SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU param; //SPVW输入参数e

	rt_err_t res = 0;
	uint32_t tmp;
	int err;
	int ix = 0;

	rt_sem_init(&start_sample_sem, "start_sample_sem", 0, RT_IPC_FLAG_FIFO); //创建信号量

	rt_thread_mdelay(50);
	LOG_D("BUSI_vw_sam_test_pro_entry");

	while (1)
	{
		rt_sem_control(&start_sample_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
		rt_sem_take(&start_sample_sem, RT_WAITING_FOREVER);			  //等待数据收集信号量
		LOG_W("busi recv start_sample_sem,l_ch:0x%x.e_ch:0x%x", scz_sam_l_ch, scz_sam_e_ch);

		if (scz_sam_l_ch >= 1) //主机通道采样
		{
			for (ix = 0; ix < 19; ix++)
				param.mid_freq[ix] = 40; //先统一将中心频率赋值为40*20，防止SPVW报错。
			param.local_ch = 0x01;
			param.extend_ch[0] = 0;
			param.extend_ch[1] = 0;

			if (scz_sam_midfreq[0] == 0) //自动模式
			{
				LOG_D("111111_1111");
				param.mid_freq[0] = 40;																																						 //先使用400-1200进行扫频，判断频率值是否在该范围内
				mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
				err = Test_Send_Msg(&scz_spvw_dmgms);

				rt_sem_control(&one_ch_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
				rt_sem_take(&one_ch_complete_sem, scz_wait_vw_time);			 //等待数据收集信号量

				if ((vw_data.vw_f_data[0] < 400) || (vw_data.vw_f_data[0] > 1200)) //第一段采样失败，进行第二段采样，1000-2000
				{
					LOG_D("111111_2222");
					param.mid_freq[0] = 75;
					mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
					err = Test_Send_Msg(&scz_spvw_dmgms);

					rt_sem_control(&one_ch_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
					rt_sem_take(&one_ch_complete_sem, scz_wait_vw_time);			 //等待数据收集信号量

					if ((vw_data.vw_f_data[0] < 1000) || (vw_data.vw_f_data[0] > 2000)) //第二段采样失败，进行第三段采样，1800-3600
					{
						LOG_D("111111_3333");
						param.mid_freq[0] = 135;
						mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
						err = Test_Send_Msg(&scz_spvw_dmgms);

						rt_sem_control(&one_ch_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
						rt_sem_take(&one_ch_complete_sem, scz_wait_vw_time);			 //等待数据收集信号量

						if ((vw_data.vw_f_data[0] < 1800) || (vw_data.vw_f_data[0] > 3600)) //第三段采样失败
						{
							vw_data.vw_f_data[0] = 0;
						}
						rt_thread_mdelay(200);
					}
				}
			}
			else //已经配置中心频率
			{
				LOG_D("2222222");
				param.mid_freq[0] = scz_sam_midfreq[0] / 20;
				if (param.mid_freq[0] < 40)
					param.mid_freq[0] = 40;
				if (param.mid_freq[0] > 160)
					param.mid_freq[0] = 160;

				mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
				err = Test_Send_Msg(&scz_spvw_dmgms);

				LOG_D("5555 Wait one_ch_complete_sem");
				rt_sem_control(&one_ch_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
				rt_sem_take(&one_ch_complete_sem, scz_wait_vw_time);			 //等待数据收集信号量
			}
			rt_sem_release(&one_sam_complete_sem); //一轮数据采集完成后，发送采样完成采样信号量
		}
		else //扩展器采样
		{
			if (scz_sam_e_ch != 0)
			{
				unsigned short tem = 0x01;
				unsigned char i = 1, j = 0;

				for (j = 0; j < 19; j++)
					param.mid_freq[j] = 40; //先统一将中心频率赋值为40*20，防止SPVW报错。
				param.local_ch = 0x00;
				param.extend_ch[0] = 0;
				param.extend_ch[1] = 0;

				for (i = 1; i <= 16; i++)
				{
					if ((scz_sam_e_ch & tem) != 0)
					{
						//ix即为通道号，tem即为对应的bit位
						if (i <= 8)
						{
							param.extend_ch[0] = 0;
							param.extend_ch[1] = tem; //特别说明，SPVW组件此处有坑，指令用的大端模式
						}
						else
						{
							param.extend_ch[0] = tem >> 8;
							param.extend_ch[1] = 0;
						}

						if (scz_sam_midfreq[i + 2] == 0) //未配置中心频率，自动模式
						{
							LOG_D("111111_1111");
							param.mid_freq[i + 2] = 40;																																					 //先使用400-1200进行扫频，判断频率值是否在该范围内
							mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
							err = Test_Send_Msg(&scz_spvw_dmgms);

							rt_sem_control(&one_ch_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
							rt_sem_take(&one_ch_complete_sem, scz_wait_vw_time);			 //等待数据收集信号量

							if ((vw_data.vw_f_data[i] < 400) || (vw_data.vw_f_data[i] > 1200)) //第一段采样失败，进行第二段采样，1000-2000
							{
								LOG_D("111111_2222");
								param.mid_freq[i + 2] = 75;
								mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
								err = Test_Send_Msg(&scz_spvw_dmgms);

								rt_sem_control(&one_ch_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
								rt_sem_take(&one_ch_complete_sem, scz_wait_vw_time);			 //等待数据收集信号量

								if ((vw_data.vw_f_data[i] < 1000) || (vw_data.vw_f_data[i] > 2000)) //第二段采样失败，进行第三段采样，1800-3600
								{
									LOG_D("111111_3333");
									param.mid_freq[i + 2] = 135;
									mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
									err = Test_Send_Msg(&scz_spvw_dmgms);

									rt_sem_control(&one_ch_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
									rt_sem_take(&one_ch_complete_sem, scz_wait_vw_time);			 //等待数据收集信号量

									if ((vw_data.vw_f_data[i] < 1800) || (vw_data.vw_f_data[i] > 3600)) //第三段采样失败
									{
										vw_data.vw_f_data[i] = 0;
									}
									rt_thread_mdelay(200);
								}
							}
						}
						else //已经配置中心频率
						{
							param.mid_freq[i + 2] = scz_sam_midfreq[i] / 20;
							if (param.mid_freq[i + 2] < 40)
							{
								param.mid_freq[i + 2] = 40;
							}

							if (param.mid_freq[i + 2] > 160)
							{
								param.mid_freq[i + 2] = 160;
							}

							mb_make_dmgms(&scz_spvw_dmgms, 0, &scz_spvw_sem, SPVW_CMD_MANUAL_SAMPLE, MB_STATN_SPVW0, BUSI_CPID, &param, sizeof(SCZ_SAM_SPVW0_MANUAL_SAMPLE_PARAM_STRU), &scz_spvw_resp); //向多维消息体中装入消息
							err = Test_Send_Msg(&scz_spvw_dmgms);

							LOG_D("5555 Wait one_ch_complete_sem");
							rt_sem_control(&one_ch_complete_sem, RT_IPC_CMD_RESET, RT_NULL); //等待前清零信号量，防止误操作
							rt_sem_take(&one_ch_complete_sem, scz_wait_vw_time);			 //等待数据收集信号量
						}
					}
					//LOG_D("ix:%d tem:0x%x", i, tem);
					tem = tem << 1;
				}

				rt_sem_release(&one_sam_complete_sem); //一轮数据采集完成后，发送采样完成采样信号量
			}
			else
			{
				//采样参数错误
				LOG_D("sam_par err:sam_mode:%d,local_ch:0x%x,ex_ch:0x%x", scz_sam_mode, scz_sam_l_ch, scz_sam_e_ch);
			}
		}

		rt_thread_mdelay(10);
	}
}

//通过4G发送消息体
unsigned char busi_com_send_msg(unsigned char cmd, unsigned char des, void *pd, unsigned short len)
{
	rt_sem_t sem_busend_msg;
	char res = 0;
	RESP_STRU resp_busend_msg = {0};
	DM_GMS_STRU gms_sp_cfg = {0};
	sem_busend_msg = rt_sem_create("busend_msg", 0, RT_IPC_FLAG_FIFO);
	mb_make_dmgms(&gms_sp_cfg, 0, sem_busend_msg, cmd, des, MB_STATN_BUSI, pd, len, &resp_busend_msg);
	rt_mq_send(busi_ctl.p_mainpipe, &gms_sp_cfg, sizeof(DM_GMS_STRU));
	if (RT_EOK != rt_sem_take(sem_busend_msg, BUSI_COM_MSG_SEM_WAIT_TIME))
	{
		LOG_E("setsp_cfg timeout!");
		res = RT_ETIMEOUT;
	}
	else
	{
		if (resp_busend_msg.r_src == 0) //无结果返回
		{
			res = RT_EOK;
		}
		else
		{
			res = resp_busend_msg.r_res;
		}
	}
	rt_sem_delete(sem_busend_msg);
	return res;
}

//发送上行数据
unsigned char busi_com_send_data(unsigned short tag, void *d_p, unsigned char topic)
{
	//rt_mutex_take(mtx_com_send, BUSI_COM_MSG_MTX_WAIT_TIME);
	unsigned char busi_res;
	SensorMessages_SensorUpMessage msg = SensorMessages_SensorUpMessage_init_zero;
	pb_ostream_t stream;
	unsigned char bu_up_pbuf[SensorMessages_SensorUpMessage_size] = {0};
	stream = pb_ostream_from_buffer(bu_up_pbuf, SensorMessages_SensorUpMessage_size);
	strcpy(msg.gateway_id, sn);
	strcpy(msg.node_id, sn + (SN_LENGTH - nodeid));
	get_rtc_ms_timestamp(&busi_unix);
	msg.unix_time = busi_unix.unix_time;

	switch (tag)
	{
	case SensorMessages_SensorUpMessage_clocktasktimeinfosensornode_tag: //上传时钟信息
		strcpy(msg.packet_name, "ClockTaskTimeInfoSensorNode");
		msg.which_msg = SensorMessages_SensorUpMessage_clocktasktimeinfosensornode_tag;
		msg.msg.clocktasktimeinfosensornode.clock0_is_on = clock_cfg.clockEntry0.is_on;
		msg.msg.clocktasktimeinfosensornode.clock0_hour = clock_cfg.clockEntry0.hour;
		msg.msg.clocktasktimeinfosensornode.clock0_minute = clock_cfg.clockEntry0.minute;
		msg.msg.clocktasktimeinfosensornode.clock0_second = clock_cfg.clockEntry0.second;
		msg.msg.clocktasktimeinfosensornode.clock0_interval = clock_cfg.clockEntry0.interval;
		msg.msg.clocktasktimeinfosensornode.clock0_number = clock_cfg.clockEntry0.number;
		msg.msg.clocktasktimeinfosensornode.clock1_is_on = clock_cfg.clockEntry1.is_on;
		msg.msg.clocktasktimeinfosensornode.clock1_hour = clock_cfg.clockEntry1.hour;
		msg.msg.clocktasktimeinfosensornode.clock1_minute = clock_cfg.clockEntry1.minute;
		msg.msg.clocktasktimeinfosensornode.clock1_second = clock_cfg.clockEntry1.second;
		msg.msg.clocktasktimeinfosensornode.clock1_interval = clock_cfg.clockEntry1.interval;
		msg.msg.clocktasktimeinfosensornode.clock1_number = clock_cfg.clockEntry1.number;
		msg.msg.clocktasktimeinfosensornode.clock2_is_on = clock_cfg.clockEntry2.is_on;
		msg.msg.clocktasktimeinfosensornode.clock2_hour = clock_cfg.clockEntry2.hour;
		msg.msg.clocktasktimeinfosensornode.clock2_minute = clock_cfg.clockEntry2.minute;
		msg.msg.clocktasktimeinfosensornode.clock2_second = clock_cfg.clockEntry2.second;
		msg.msg.clocktasktimeinfosensornode.clock2_interval = clock_cfg.clockEntry2.interval;
		msg.msg.clocktasktimeinfosensornode.clock2_number = clock_cfg.clockEntry2.number;
		break;
	case SensorMessages_SensorUpMessage_topinfonodedata_tag: //上传状态信息

		strcpy(msg.packet_name, "TopInfoSensorNode");

		msg.which_msg = SensorMessages_SensorUpMessage_topinfonodedata_tag;
		msg.msg.topinfonodedata.battery_voltage = getbat();
		LOG_D("bat=%f", msg.msg.topinfonodedata.battery_voltage);
		msg.msg.topinfonodedata.sampling = is_sampling;
		msg.msg.topinfonodedata.version = pro_ini.VERSION;
		msg.msg.topinfonodedata.worktime = busi_work_time;
		//msg.msg.topinfonodedata.worktime = 120;
		//	msg.msg.topinfonodedata.interval = clock_cfg.clockEntry0.interval;
		msg.msg.topinfonodedata.battery_num = pro_ini.BATTERY_NUM;
		msg.msg.topinfonodedata.battery_type = pro_ini.BATTERY_TYPE;
		strcpy(msg.msg.topinfonodedata.product_type, "SCZ0F1");
		break;
	case SensorMessages_SensorUpMessage_clocktasksampleinfo_tag: //上传采样参数
		strcpy(msg.packet_name, "ClockTaskSampleInfoSCZ0F1");
		msg.which_msg = SensorMessages_SensorUpMessage_clocktasksampleinfo_tag;
		{
			msg.msg.clocktasksampleinfo.sample_pattern = scz_cfg_par.sam_mode[0];
			msg.msg.clocktasksampleinfo.local_channel_bit = scz_cfg_par.local_ch[0];
			msg.msg.clocktasksampleinfo.ext_channel_bit = scz_cfg_par.ex_ch[0];
		}

		break;
	case SensorMessages_SensorUpMessage_vibrawireinfo_tag: //上传传感器参数
		strcpy(msg.packet_name, "VibraWireInfoSCZ0F1");
		msg.which_msg = SensorMessages_SensorUpMessage_vibrawireinfo_tag;
		{
			msg.msg.vibrawireinfo.ch0_midfreq = scz_cfg_par.midfreq[0];
			msg.msg.vibrawireinfo.ch1_midfreq = scz_cfg_par.midfreq[1];
			msg.msg.vibrawireinfo.ch2_midfreq = scz_cfg_par.midfreq[2];
			msg.msg.vibrawireinfo.ch3_midfreq = scz_cfg_par.midfreq[3];
			msg.msg.vibrawireinfo.ch4_midfreq = scz_cfg_par.midfreq[4];
			msg.msg.vibrawireinfo.ch5_midfreq = scz_cfg_par.midfreq[5];
			msg.msg.vibrawireinfo.ch6_midfreq = scz_cfg_par.midfreq[6];
			msg.msg.vibrawireinfo.ch7_midfreq = scz_cfg_par.midfreq[7];
			msg.msg.vibrawireinfo.ch8_midfreq = scz_cfg_par.midfreq[8];
			msg.msg.vibrawireinfo.ch9_midfreq = scz_cfg_par.midfreq[9];
			msg.msg.vibrawireinfo.ch10_midfreq = scz_cfg_par.midfreq[10];
			msg.msg.vibrawireinfo.ch11_midfreq = scz_cfg_par.midfreq[11];
			msg.msg.vibrawireinfo.ch12_midfreq = scz_cfg_par.midfreq[12];
			msg.msg.vibrawireinfo.ch13_midfreq = scz_cfg_par.midfreq[13];
			msg.msg.vibrawireinfo.ch14_midfreq = scz_cfg_par.midfreq[14];
			msg.msg.vibrawireinfo.ch15_midfreq = scz_cfg_par.midfreq[15];
			msg.msg.vibrawireinfo.ch16_midfreq = scz_cfg_par.midfreq[16];
		}

		break;
	case SensorMessages_SensorUpMessage_vibrawiredata_tag: //上传采样数据
		strcpy(msg.packet_name, "VibraWireSCZ0F1");

		msg.which_msg = SensorMessages_SensorUpMessage_vibrawiredata_tag;
		{
			char str[3];
			vw_sam_data_t *d_sam = (vw_sam_data_t *)d_p;
			LOG_D("0x%x %fv %fHz %f", d_sam->ret_ch, d_sam->ret_voltage, d_sam->ret_freq, d_sam->ret_temp);

			sprintf(msg.channel, "%d", d_sam->ret_ch);

			msg.msg.vibrawiredata.voltage = d_sam->ret_voltage;
			msg.msg.vibrawiredata.freq = d_sam->ret_freq;
			msg.msg.vibrawiredata.temp = d_sam->ret_temp;
		}
		break;

	default:
		LOG_E("no tag %d ", __LINE__);
		break;
	}

	busi_res = pb_encode(&stream, SensorMessages_SensorUpMessage_fields, &msg);

	if (busi_res)
	{
		//LOG_HEX("busiencode", 16, bu_up_pbuf, stream.bytes_written);
		switch (topic)
		{
		case UP:
			busi_res = busi_com_send_msg(COMN0_CMD_SEND_DATA, MB_STATN_COMX, bu_up_pbuf, stream.bytes_written);
			break;
		case BATCH:
			busi_res = busi_com_send_msg(COMN0_CMD_SEND_DATA, MB_STATN_COMX, bu_up_pbuf, stream.bytes_written);
			break;
		default:
			busi_res = RT_EINVAL;
			break;
		}
		return busi_res;
	}
	else
	{

		LOG_E("encode fail 111111111 %d", __LINE__);
		return RT_ERROR;
	}
	//	rt_mutex_release(mtx_com_send);
}

//发送ACK
void busi_com_send_ack(unsigned char *cmd_name, unsigned char res, char *cmd_id)
{
	unsigned char busi_res;
	SensorMessages_SensorAckMessage msg = SensorMessages_SensorAckMessage_init_zero;
	pb_ostream_t stream;
	unsigned char ack_pbuf[SensorMessages_SensorAckMessage_size] = {0};
	stream = pb_ostream_from_buffer(ack_pbuf, SensorMessages_SensorAckMessage_size);
	get_rtc_ms_timestamp(&busi_unix);
	msg.unix_time = busi_unix.unix_time;
	strcpy(msg.instruction_id, cmd_id);
	strcpy(msg.instruction_name, cmd_name);
	strcpy(msg.packet_name, strcat(cmd_name, "Ack"));
	strcpy(msg.gateway_id, sn);
	msg.err_code = res;
	if (pb_encode(&stream, SensorMessages_SensorAckMessage_fields, &msg))
	{
		//LOG_HEX("busiencode", 10, ack_pbuf, stream.bytes_written);
		busi_com_send_msg(COMN0_CMD_SEND_ACK, MB_STATN_COMX, ack_pbuf, stream.bytes_written);
	}
	else
	{
		LOG_E("encode fail 2222222222 %d", __LINE__);
	}
}

unsigned char data_check(unsigned char type, void *d_p)
{
	switch (type)
	{
	case CFG_CLK:
	{
		ConfigClockTask *d_tmp = (ConfigClockTask *)d_p;
		if (d_tmp->clock_num > 2)
		{
			return RT_EINVAL;
		}

		if (d_tmp->hour > 23)
		{
			return RT_EINVAL;
		}
		if ((d_tmp->hour == 0) && (d_tmp->minute < 3))
		{
			return RT_EINVAL;
		}
		if (d_tmp->minute > 59)
		{
			return RT_EINVAL;
		}
		if (d_tmp->second > 59)
		{
			return RT_EINVAL;
		}
		// if (d_tmp->clock_num == 0)//如果是时钟条目0
		// {

		// }
		// else
		// {

		// }

		if (d_tmp->interval < 30) //间隔必须大于30s
		{
			return RT_EINVAL;
		}
		if (d_tmp->interval > 0xffff)
		{
			return RT_EINVAL;
		}
		if (d_tmp->is_on > 1)
		{
			return RT_EINVAL;
		}
		if (d_tmp->number > 0xffff)
		{
			return RT_EINVAL;
		}
		// if(d_tmp->sample_pattern > 1)//检查本机通道
		//  	{
		// 	return RT_EINVAL;
		// }
		if (d_tmp->local_channel_bit > 1) //检查本机通道
		{
			return RT_EINVAL;
		}
		if((d_tmp->local_channel_bit != 0) && (d_tmp->ext_channel_bit !=0))
		{
			return RT_EINVAL;
		}
		// if(d_tmp->ext_channel_bit > 1)
		//  	{
		// 	return RT_EINVAL;
		// }
	}
	break;
	case CFG_PAR:
	{
		ConfigVibraWireParameter *d_tmp = (ConfigVibraWireParameter *)d_p;
		if (!((d_tmp->ch0_midfreq == 0) || ((d_tmp->ch0_midfreq >= 400) && (d_tmp->ch0_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch1_midfreq == 0) || ((d_tmp->ch1_midfreq >= 400) && (d_tmp->ch1_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch2_midfreq == 0) || ((d_tmp->ch2_midfreq >= 400) && (d_tmp->ch2_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch3_midfreq == 0) || ((d_tmp->ch3_midfreq >= 400) && (d_tmp->ch3_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch4_midfreq == 0) || ((d_tmp->ch4_midfreq >= 400) && (d_tmp->ch4_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch5_midfreq == 0) || ((d_tmp->ch5_midfreq >= 400) && (d_tmp->ch5_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch6_midfreq == 0) || ((d_tmp->ch6_midfreq >= 400) && (d_tmp->ch6_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch7_midfreq == 0) || ((d_tmp->ch7_midfreq >= 400) && (d_tmp->ch7_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch8_midfreq == 0) || ((d_tmp->ch8_midfreq >= 400) && (d_tmp->ch8_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch9_midfreq == 0) || ((d_tmp->ch9_midfreq >= 400) && (d_tmp->ch9_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch10_midfreq == 0) || ((d_tmp->ch10_midfreq >= 400) && (d_tmp->ch10_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch11_midfreq == 0) || ((d_tmp->ch11_midfreq >= 400) && (d_tmp->ch11_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch12_midfreq == 0) || ((d_tmp->ch12_midfreq >= 400) && (d_tmp->ch12_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch13_midfreq == 0) || ((d_tmp->ch13_midfreq >= 400) && (d_tmp->ch13_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch14_midfreq == 0) || ((d_tmp->ch14_midfreq >= 400) && (d_tmp->ch14_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch15_midfreq == 0) || ((d_tmp->ch15_midfreq >= 400) && (d_tmp->ch15_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
		if (!((d_tmp->ch16_midfreq == 0) || ((d_tmp->ch16_midfreq >= 400) && (d_tmp->ch16_midfreq <= 3600))))
		{
			return RT_EINVAL;
		}
	}
	break;
	default:
		return RT_EINVAL;
		break;
	}
	return RT_EOK;
}

unsigned char is_busy()
{
	return is_sampling;
}
void set_busy()
{
	is_sampling = RT_TRUE;
}
void clr_busy()
{
	is_sampling = RT_FALSE;
}
