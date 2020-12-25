#include "rtthread.h"
#include "common.h"
#include "busi_manager.h"
#include "busi_pro.h"

#include "busi_cfg.h"

#include "common.h"
#include "adbs.h"
#define LOG_TAG "MB_BUSI"
#define LOG_LVL LOG_LVL_DBG

#define IS_SEND_TEST 0
#include <ulog.h>

//===========================================================
static struct rt_messagequeue busi_pipe;           //BUSI组件管道
unsigned char busi_msg_pool[BUSI_POOL_SIZE] = {0}; //BUSI组件管道缓冲
//===========================================================
CP_CTL_STRU busi_ctl = {0, RT_NULL, RT_NULL, BUSI_CPID, 0}; //BUSI组件控制体
CP_REG_STRU busi_reg = {RT_NULL, BUSI_CPID};                //用于BUSI组件向主管道注册

struct rt_thread rthread_msg_pro; //BUSI消息处理进程结构体
unsigned char thread_msg_pro_stack[2 * THREAD_STACK_SIZE];

rt_thread_t rthread_register; //BUSI组件注册进程结构体

//DL_FUNCS_STRU funcs;
static DM_GMS_STRU tmp_dmgms;
static GMS_STRU tmp_gms;

static rt_sem_t sem_reg = RT_NULL;
static rt_timer_t system_timer = RT_NULL;

RESP_STRU resp_reg;

void system_timer_timeout()
{
    busi_work_time++;
    busi_normal_time++;

    if (busi_work_time == 5) //正常运行5分钟，去除看门狗重启计数
    {
        wdt_reboot_count_clr();
    }
}

void BUSI_msg_pro_entry(void *p) //BUSI组件消息处理进程
{
    LOG_D("msgprorun ");
    get_pro_ini();
    //get_com_cfg(BSCFG_CFG_FILENAME, &init_info, sizeof(init_info));
    get_com_cfg(UTCM_CFG_FILENAME, &clock_cfg, sizeof(clock_cfg));
    get_com_cfg(SN_FILENAME, sn, SN_LENGTH);
    get_scz0_cfg();     //获取SCZ的配置参数
    nodeid = my_strchr(sn + 10, '0');
    ulog_tag_lvl_filter_set(LOG_TAG, pro_ini.PRO_LOG_LVL);
    {
        unsigned char res = 0;
        busi_mq_cmd_pro_pipe = rt_mq_create("busi_mq_cmd_pro_pipe", sizeof(busi_cmd_buf_t), BUSI_CMD_PRO_NMSG, RT_IPC_FLAG_FIFO);

        if (busi_mq_cmd_pro_pipe == RT_NULL)
        {
            LOG_E("cmdpropipefail");
        }

        busi_mq_clk_pro_pipe = rt_mq_create("busi_mq_clk_pro_pipe", sizeof(clk_num), BUSI_CLK_PRO_NMSG, RT_IPC_FLAG_FIFO);
        if (busi_mq_clk_pro_pipe == RT_NULL)
        {
            LOG_E("clkpropipefail");
        }

        //创建命令解析线程
        rt_thread_init(&thread_busi_cmd_pro, 
                        "busi_cmd_pro", 
                        BUSI_cmd_pro_entry, 
                        RT_NULL, 
                        thread_busi_cmd_pro_stack, 
                        sizeof(thread_busi_cmd_pro_stack), 
                        THREAD_PRIORITY - 2, 
                        THREAD_TIMESLICE);
        rt_thread_startup(&thread_busi_cmd_pro);

        //创建采样线程
        rt_thread_init(&rthread_sam_msg_pro, 
                        "BUSI_sam_msg_pro", 
                        BUSI_sam_pro_entry, 
                        RT_NULL, 
                        thread_cont_sam_msg_pro_stack, 
                        sizeof(thread_cont_sam_msg_pro_stack), 
                        THREAD_PRIORITY - 2, 
                        THREAD_TIMESLICE);
        rt_thread_startup(&rthread_sam_msg_pro);

        //创建时钟条目线程
        rt_thread_init(&thread_busi_clock_pro, 
                        "BUSI_clk_pro", 
                        BUSI_clock_pro_entry, 
                        RT_NULL, 
                        thread_busi_clock_pro_stack, 
                        sizeof(thread_busi_clock_pro_stack), 
                        THREAD_PRIORITY - 2, 
                        THREAD_TIMESLICE);
        rt_thread_startup(&thread_busi_clock_pro);

        //创建数据发送线程
        rt_thread_init(&thread_vw_data_send_pro, 
                        "BUSI_data_send_pro", 
                        BUSI_vw_data_send_pro_entry, 
                        RT_NULL, 
                        thread_vw_data_send_pro_stack, 
                        sizeof(thread_vw_data_send_pro_stack), 
                        THREAD_PRIORITY - 2, 
                        THREAD_TIMESLICE);
        rt_thread_startup(&thread_vw_data_send_pro);

        //创建振弦数据采集测试线程
        rt_thread_init(&thread_vw_sam_test_pro, 
                        "BUSI_test_pro", 
                        BUSI_vw_sam_test_pro_entry, 
                        RT_NULL, 
                        thread_vw_sam_test_pro_stack, 
                        sizeof(thread_vw_sam_test_pro_stack), 
                        THREAD_PRIORITY - 2, 
                        THREAD_TIMESLICE);
        rt_thread_startup(&thread_vw_sam_test_pro);


        mtx_com_send = rt_mutex_create("BUSI_com_send", RT_IPC_FLAG_FIFO);
    }
    {
        if (clock_cfg.clockEntry0.interval != 900) //900是utcm组件默认值。
        {
            ;
        }
        else //无存储值
        {
            clock_cfg.clockEntry0.is_on = pro_ini.CLK_IS_ON;
            clock_cfg.clockEntry0.clock = pro_ini.CLK_NUM;
            clock_cfg.clockEntry0.hour = 0;
            clock_cfg.clockEntry0.minute = 3;
            clock_cfg.clockEntry0.second = 16;
            clock_cfg.clockEntry0.interval = pro_ini.INTERVAL;
            clock_cfg.clockEntry0.number = 65530;
        }

        busi_com_send_msg(CP_CMD_DST(UTC_CMD_CFG_CE), 
                            MB_STATN_UTCM, 
                            &clock_cfg.clockEntry0, 
                            sizeof(UTC_CLOCK_ENTRY_STRU));
    }
    system_timer = rt_timer_create("BUSI_sys_timer", 
                                    system_timer_timeout, 
                                    RT_NULL, 1000 * 60, 
                                    RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if (RT_NULL == system_timer)
    {
        LOG_E("timerr");
    }
    rt_timer_start(system_timer);
    while (1)
    {
        if (rt_mq_recv(&busi_pipe, &tmp_gms, sizeof(GMS_STRU), RT_WAITING_FOREVER) == RT_EOK) //从busi_pipe获取消息
        {
            LOG_W("request%d", __LINE__);
            rt_pm_request(PM_SLEEP_MODE_NONE);

            if (tmp_gms.d_cmd.is_src_cmd == 1) //使用源的指令解析
            {
                switch (tmp_gms.d_src) //判断消息源
                {
                case MB_STATN_COMX:
                    switch (tmp_gms.d_cmd.cmd) //使用源指令
                    {
                    case COMN0_CMD_RECV_DATA: //通信组件收到下行指令，转发到此
                    {
                        unsigned char res;
                        busi_cmd_buf_t tmp;
                        tmp.d_len = tmp_gms.d_dl;
                        LOG_E("busi recv_len：%d", tmp.d_len);
                        LOG_HEX("comreceive", 16, tmp_gms.d_p, tmp_gms.d_dl);
                        if (tmp_gms.d_dl < 256)
                        {
                            rt_memcpy(tmp.data, tmp_gms.d_p, tmp.d_len);
                            res = rt_mq_send(busi_mq_cmd_pro_pipe, &tmp, sizeof(busi_cmd_buf_t));
                            LOG_D("mq_cmd %d", res);
                        }
                        else
                        {
                            res = CP_RESP_FAIL;
                            LOG_E("cmd fail");
                        }
                        mb_resp_msg(&tmp_gms, MB_STATN_BUSI, res); //回应消息,释放信号量
                    }
                    break;
                    default:
                        break;
                    }
                    break;
                case MB_STATN_SPVW0: //如果消息来自振弦组件，将数据转发至采集线程

                    mb_resp_msg(&tmp_gms, BUSI_CPID, 0); //释放信号量并返回结果

                    switch (tmp_gms.d_cmd.cmd)
                    {
                    case SPVW_RET_SAMPLE: //收到的是振弦组件的数据，
                    {
                        unsigned char res;
                        SCZ_SAM_SPVW0_DAT_RES_STRU scz_dat;

                        rt_memcpy(&scz_dat.channel, (SCZ_SAM_SPVW0_DAT_RES_STRU *)tmp_gms.d_p, sizeof(SCZ_SAM_SPVW0_DAT_RES_STRU));
                        res = rt_mq_send(&busi_mq_sam_res_pipe, &scz_dat, sizeof(SCZ_SAM_SPVW0_DAT_RES_STRU));//将接收到的数据发送至采集线程
                        LOG_D("mq_cmd %d", res);
                        
                        break;
                    }
                    case SPVW_RET_ACK:
                    {
                        uint8_t ack_d = *(uint8_t *)tmp_gms.d_p; 

                        //mb_resp_msg(&tmp_gms, BUSI_CPID, 0); //释放信号量并返回结果

                        if ((ack_d == 1) || (ack_d == 2) || (ack_d == 26) || (ack_d == 254))
                        {
                            rt_thread_mdelay(50);
                            rt_sem_release(&one_ch_complete_sem); //处理完成后发送信号量
                        }
                        //mb_resp_msg(&tmp_gms, BUSI_CPID, 0); //释放信号量并返回结果
                        LOG_D("SCZ Receive SPVW ack status=%d", ack_d);
                        break;
                    }
                    default:
                        break;
                    }

                    break;

                case MB_STATN_UTCM:
                {
                    LOG_W("request%d", __LINE__);
                    rt_pm_request(PM_SLEEP_MODE_NONE);

                    // rt_thread_mdelay(2000);
                }
                    switch (tmp_gms.d_cmd.cmd) //使用源指令
                    {
                    case UTC_ENT_CE0_TIMEUP: //收到时钟条目0触发
                                             //  LOG_D("0run");
                        clk_num = 0;
                        if (rt_mq_send(busi_mq_clk_pro_pipe, &clk_num, sizeof(clk_num))) //发送失败
                        {
                            LOG_W("release=%d", __LINE__);
                            rt_pm_release(PM_SLEEP_MODE_NONE);
                        }
                        break;
                    case UTC_ENT_CE1_TIMEUP:
                        LOG_D("1run %d", __LINE__);
                        clk_num = 1;
                        if (rt_mq_send(busi_mq_clk_pro_pipe, &clk_num, sizeof(clk_num))) //发送失败
                        {
                            LOG_W("release=%d", __LINE__);
                            rt_pm_release(PM_SLEEP_MODE_NONE);
                        }
                        break;
                    case UTC_ENT_CE2_TIMEUP:
                        clk_num = 2;
                        LOG_D("2run %d", __LINE__);
                        if (rt_mq_send(busi_mq_clk_pro_pipe, &clk_num, sizeof(clk_num))) //发送失败
                        {
                            LOG_W("release=%d", __LINE__);
                            rt_pm_release(PM_SLEEP_MODE_NONE);
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
            }
            else //使用目的地址的命令解析
            {
                LOG_E("cmderr%d", __LINE__);
            }

            LOG_W("release=%d", __LINE__);
            rt_pm_release(PM_SLEEP_MODE_NONE);
        }
    }
}
void BUSI_register_entry(void *p)
{
    {
        busi_reg.p_pipe = &busi_pipe;
        {
            sem_reg = rt_sem_create("busi_reg", 0, RT_IPC_FLAG_FIFO);                                                                                       //初始化用于消息同步的信号量，接受者在完成消息处理后，必要时需要对信号量进行释放
            rt_memset(&tmp_dmgms, 0, sizeof(DM_GMS_STRU));                                                                                                  //清空多维消息体
            mb_make_dmgms(&tmp_dmgms, 0, sem_reg, CP_CMD_DST(MAINPIPE_CMD_CPREG), MB_STATN_MAIN, MB_STATN_BUSI, &busi_reg, sizeof(CP_REG_STRU), &resp_reg); //向多维消息体中装入消息
            rt_mq_send(busi_ctl.p_mainpipe, &tmp_dmgms, sizeof(DM_GMS_STRU));                                                                               //向主管道发送注册命令
            if (RT_EOK != rt_sem_take(sem_reg, BUSI_REG_SEM_WAIT_TIME))                                                                                     //等待接收者释放信号量，超时将造成组件注册失败
            {
                LOG_E("cp-reg ot!");
            }
            rt_sem_delete(sem_reg);
            if (MB_STATN_MAIN == resp_reg.r_src && MB_RESP_SUC == resp_reg.r_res)
            {
                busi_ctl.is_registered = 1;
                LOG_D("regsuc!");
            }
            else
            {
                LOG_E("regfail!");
            }
        }
    }
    {
        if (rt_mq_recv(&busi_pipe, &tmp_gms, sizeof(GMS_STRU), RT_WAITING_FOREVER) == RT_EOK) //从BUSI组件管道获取消息
        {
            if (tmp_gms.d_cmd.is_src_cmd) //是源头命令
            {
                if (MB_STATN_MAIN == tmp_gms.d_src) //来自主管道
                {
                    if (MAINPIPE_CMD_STARTUP == tmp_gms.d_cmd.cmd) //业务启动命令
                    {
                        rt_thread_init(&rthread_msg_pro, "busi_msg_pro", BUSI_msg_pro_entry, RT_NULL, thread_msg_pro_stack, sizeof(thread_msg_pro_stack), THREAD_PRIORITY - 3, THREAD_TIMESLICE);
                        rt_thread_startup(&rthread_msg_pro);
                    }
                    else
                        return;
                }
                else
                    return;
            }
            else
                return;
        }
    }
    return;
}

void main(void *p) //主入口
{
    rt_err_t res = 0;

    LOG_D("main thread start");

    res = rt_mq_init(&busi_pipe, "busi_pipe", busi_msg_pool, sizeof(GMS_STRU), sizeof(busi_msg_pool), RT_IPC_FLAG_FIFO); //建立BUSI管道
    if (res != RT_EOK)
    {
        LOG_E("mainpipefail!");
        return;
    }
    {
        busi_ctl.p_mainpipe = ((CP_ENTRY_ARGS_STRU *)p)->p_mq;
        busi_ctl.p_lib = ((CP_ENTRY_ARGS_STRU *)p)->p_lib;
        //将动态库中的函数导出
        //这样的作法是为了在函数层面解耦
        {
            //funcs.pfun_=dlsym(comnb_ctl.p_lib,"xxx"); //从动态库(即so中找到名为xxx的符号，即函数名)
        }
    }
    rthread_register = rt_thread_create("busi_reg", BUSI_register_entry, RT_NULL, 2048, THREAD_PRIORITY - 7, THREAD_TIMESLICE);
    rt_thread_startup(rthread_register);

    return;
}
