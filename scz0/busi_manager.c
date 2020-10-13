#include "rtthread.h"
#include "common.h"
#include "busi_manager.h"
#include "busi_pro.h"

#define LOG_TAG "SCZ_BUSI"
#define LOG_LVL LOG_LVL_DBG

#define IS_SEND_TEST 0
#include <ulog.h>
#include "board.h"

//===========================================================
static struct rt_messagequeue busi_pipe;           //BUSI组件管道
unsigned char busi_msg_pool[BUSI_POOL_SIZE] = {0}; //BUSI组件管道缓冲
//===========================================================
CP_CTL_STRU busi_ctl = {0, RT_NULL, RT_NULL, BUSI_CPID, 0}; //BUSI组件控制体
CP_REG_STRU busi_reg = {RT_NULL, BUSI_CPID};                //用于BUSI组件向主管道注册

struct rt_thread rthread_msg_pro; //BUSI消息处理进程结构体
unsigned char thread_msg_pro_stack[3072];

struct rt_thread rthread_register; //BUSI组件注册进程结构体
unsigned char thread_register_stack[THREAD_STACK_SIZE];

DL_FUNCS_STRU funcs;

DM_GMS_STRU scz_dmgms;
GMS_STRU scz_gms;

COMB0_CONFIG_MODBUS_STRU modbus_cfg = MODBUS_CFG_DEF;
static rt_sem_t sem_reg = RT_NULL;
static rt_sem_t sem_rscmd_pro = RT_NULL;
static rt_timer_t system_timer = RT_NULL;
unsigned int work_time;

void *pexchange;

RESP_STRU resp_reg;

/*******    数据采集线程    ******/
struct rt_thread rthread_sca_sample_spvw;
unsigned char rthread_sca_sample_spvw_stack[3072];
unsigned char rthread_sca_sample_spvw_THREAD_PRIORITY = 20;

struct rt_semaphore scz_sam_spvw_complete_sem;

void load_modbus_cfg_def(void *m_p)
{
    COMB0_CONFIG_MODBUS_STRU *p = (COMB0_CONFIG_MODBUS_STRU *)m_p;
    p->adu_max_len = modbus_cfg.adu_max_len;
    p->baud_rate = modbus_cfg.baud_rate;
    p->rs_mode = modbus_cfg.rs_mode;
    p->resp_timeout = modbus_cfg.resp_timeout;
    p->_0x_len = modbus_cfg._0x_len;
    p->_1x_len = modbus_cfg._1x_len;
    p->_4x_len = modbus_cfg._4x_len;
    p->_3x_len = modbus_cfg._3x_len;
    //   p->addr = modbus_cfg.addr;
    ("p->_4x_len =%d,p->_3x_len=%d", p->_4x_len, p->_3x_len);
}
void system_timer_timeout()
{
    work_time++;
    input_r->worktime_h = (work_time & 0xFFFF0000) >> 8;
    input_r->worktime_l = work_time & 0x0000FFFF;
    //if(pro_ini.IS_DEBUG)LOG_D("worktime= %d",work_time);
}


void BUSI_msg_pro_entry(void *p) //BUSI组件消息处理进程，在该进程中，BUSI处理所有的消息接口
{
    int fd;
    pexchange = rt_malloc(sizeof(COMB0_CONFIG_MODBUS_STRU) * 2);
    get_sam_cfg();
    get_sn();

    load_input_reg_def();
    load_modbus_cfg_def(pexchange);
    LOG_D("BUSI_msg_pro thread start");
    {
        unsigned char res = 0;
        busi_modbus_cmd_pro_pipe = rt_mq_create("busi_modbus_cmd_pro_pipe", sizeof(modbus_cmd_t), BUSI_MODBUS_CMD_PRO_NMSG, RT_IPC_FLAG_FIFO); //建立用于缓冲分片数据的管道
        if (busi_modbus_cmd_pro_pipe == RT_NULL)
        {
            LOG_E("busi_modbus_cmd_pro_pipe init fail!");
        }
        //modbus命令处理线程
        rt_thread_init(&thread_busi_modbus_cmd_pro, "busi_modbus_cmd_pro", modbus_cmd_pro_entry, RT_NULL, thread_busi_modbus_cmd_pro_stack, sizeof(thread_busi_modbus_cmd_pro_stack), THREAD_PRIORITY - 2, THREAD_TIMESLICE);
        rt_thread_startup(&thread_busi_modbus_cmd_pro);

        //创建数据采集线程
        rt_thread_init(&rthread_sca_sample_spvw,
                       "rthread_sca_sample_spvw",
                       scz_sample_thread_entry,
                       RT_NULL,
                       rthread_sca_sample_spvw_stack,
                       sizeof(rthread_sca_sample_spvw_stack),
                       rthread_sca_sample_spvw_THREAD_PRIORITY,
                       THREAD_TIMESLICE);
        rt_thread_startup(&rthread_sca_sample_spvw);

        //连续数据采集进程
        // rt_thread_init(&rthread_cont_sam_msg_pro, "BUSI_cont_sam_msg_pro", BUSI_cont_sam_pro_entry, RT_NULL, thread_cont_sam_msg_pro_stack, sizeof(thread_cont_sam_msg_pro_stack), THREAD_PRIORITY, THREAD_TIMESLICE);
        // rt_thread_startup(&rthread_cont_sam_msg_pro);
    }
    sem_rscmd_pro = rt_sem_create("busi_rscmd_pro", 0, RT_IPC_FLAG_FIFO);
    rt_memset(&scz_dmgms, 0, sizeof(DM_GMS_STRU)); //清空多维消息体

#if IS_SEND_TEST
    mb_make_dmgms(&scz_dmgms, 0, sem_rscmd_pro, COMB0_CMD_CONFIG_MODBUS, MB_STATN_TESTCOMP, MB_STATN_BUSI, pexchange, sizeof(COMB0_CONFIG_MODBUS_STRU), NULL); //向多维消息体中装入消息

#else
    mb_make_dmgms(&scz_dmgms, 0, sem_rscmd_pro, COMB0_CMD_CONFIG_MODBUS, MB_STATN_COMB0, MB_STATN_BUSI, pexchange, sizeof(COMB0_CONFIG_MODBUS_STRU), NULL); //向多维消息体中装入消息

#endif
    rt_mq_send(busi_ctl.p_mainpipe, &scz_dmgms, sizeof(DM_GMS_STRU));
    if (RT_EOK != rt_sem_take(sem_rscmd_pro, BUSI_REG_SEM_WAIT_TIME))
    {
        LOG_E("cp-reg ot!");
        return;
    }
    {
        LOG_D("hold_r run");
        COMB0_CONFIG_MODBUS_RESP_STRU *modbus_mapping = (COMB0_CONFIG_MODBUS_RESP_STRU *)pexchange;
        LOG_D("p_0x:%08X", modbus_mapping->p_0x);
        LOG_D("p_1x:%08X", modbus_mapping->p_1x);
        LOG_D("p_4x:%08X", modbus_mapping->p_4x);
        LOG_D("p_3x:%08X", modbus_mapping->p_3x);
        input_r = (input_reg_t *)(modbus_mapping->p_4x);
        hold_r = (hold_reg_t *)(modbus_mapping->p_3x);
        //LOG_D("hold_r %d %d", hold_r->d_addr, hold_r->d_group);
    }
    rt_memcpy(input_r, &input_reg, sizeof(input_reg));
    rt_memcpy(hold_r, &hold_reg, sizeof(hold_reg));

    system_timer = rt_timer_create("system_timer", system_timer_timeout, RT_NULL, 1000 * 60, RT_TIMER_FLAG_PERIODIC);
    if (RT_NULL == system_timer)
    {
        LOG_E("s_time err");
    }
    rt_timer_start(system_timer);
    scz0b1_timer_sample();

    rt_sem_init(&scz_sam_spvw_complete_sem, "scz_sam_spvw_complete_sem", 0, RT_IPC_FLAG_FIFO); //创建信号量



    LOG_D("hold_r addr=%d group=%d", hold_r->d_addr, hold_r->d_group);
    LOG_D("hold_r is_hs=%d is_use_ex=%d", hold_r->is_hs, hold_r->is_use_ex);
    LOG_D("hold_r ch=0x%x", hold_r->ch);
    LOG_D("hold_r s_mode=%d td=%d", hold_r->s_mode, hold_r->time_duration);


    while (1)
    {
        if (rt_mq_recv(&busi_pipe, &scz_gms, sizeof(GMS_STRU), RT_WAITING_FOREVER) == RT_EOK) //从busi_pipe获取消息
        {
            //LOG_D("SCZ Receive data,drc=%d",scz_gms.d_src);
            if (scz_gms.d_cmd.is_src_cmd == 1) //使用源的指令解析
            {
                switch (scz_gms.d_src)
                {
                case MB_STATN_COMB0: //消息是从comb发送过来的
                {
                    switch (scz_gms.d_cmd.cmd)
                    {
                    case COMB0_CMD_HREGS_CONFIGGED: //收到保持寄存器改变
                    {
                        unsigned char res = 0;
                        modbus_cmd_t tmp;
                        unsigned char len;
                        len = sizeof(modbus_cmd_t);
                        rt_memcpy(&tmp, scz_gms.d_p, len);
                        if (tmp.func != 0x06) //写单个寄存器
                        {
                            mb_resp_msg(&scz_gms, MB_STATN_BUSI, MODBUS_EINVAL); //回应消息,释放信号量
                            break;
                        }
                        if (tmp.data_addr >= HOLD_REG_LAST)
                        {
                            mb_resp_msg(&scz_gms, MB_STATN_BUSI, CP_RESP_SUC); //回应消息,释放信号量
                            break;
                        }
                        switch (tmp.data_addr) //要修改的寄存器序号
                        {
                        case HOLD_REG_0: //修改地址
                            if (tmp.data[0] > MAX_DEVICE_ADDR)
                                res = MODBUS_EINVAL;
                            else
                                res = CP_RESP_SUC;
                            break;
                        case HOLD_REG_1: //向该位写入0xaaaa时，设备执行软重启
                            if (tmp.data[0] == 0xAAAA)
                                rt_hw_cpu_reset();
                            break;
                        case HOLD_REG_2: //器件分组
                            res = CP_RESP_SUC;
                            break;
                        case HOLD_REG_3:            //是否高速采样
                            if (tmp.data[0] > 0x01) //nom=0x00,HS=0x01
                                res = MODBUS_EINVAL;
                            else
                                res = CP_RESP_SUC;
                            break;
                        case HOLD_REG_4:            //是否使用扩展器
                            if (tmp.data[0] > 0x01) //不使用=0x00,使用=0x01
                                res = MODBUS_EINVAL;
                            else
                                res = CP_RESP_SUC;
                            break;
                        case HOLD_REG_5: //通道选择
                            res = CP_RESP_SUC;
                            break;
                        case HOLD_REG_6: //采样模式
                            if (tmp.data[0] > 0x03)
                                res = MODBUS_EINVAL;
                            else
                                res = CP_RESP_SUC;
                            break;
                        case HOLD_REG_7:                                        //定时间隔
                            if (tmp.data[0] < (6 * scz_calc_bit_1(hold_r->ch))) //定时采样最时间=通道数*6s
                                res = MODBUS_EINVAL;
                            else
                                res = CP_RESP_SUC;
                            break;
                        case HOLD_REG_8: //中心频率
                        case HOLD_REG_9:
                        case HOLD_REG_10:
                        case HOLD_REG_11:
                        case HOLD_REG_12:
                        case HOLD_REG_13:
                        case HOLD_REG_14:
                        case HOLD_REG_15:
                        case HOLD_REG_16:
                        case HOLD_REG_17:
                        case HOLD_REG_18:
                        case HOLD_REG_19:
                        case HOLD_REG_20:
                        case HOLD_REG_21:
                        case HOLD_REG_22:
                        case HOLD_REG_LAST:
                            if ((tmp.data[0] < 800) || (tmp.data[0] > 3200))
                                res = MODBUS_EINVAL;
                            else
                                res = CP_RESP_SUC;
                            break;
                        default:
                            break;
                        }

                        if (res == CP_RESP_SUC)
                        {
                            mb_resp_msg(&scz_gms, MB_STATN_BUSI, res);             //回应消息,释放信号量
                            res = rt_mq_send(busi_modbus_cmd_pro_pipe, &tmp, len); //将数据发送给modbus命令处理线程（modbus_cmd_pro_entry）
                            LOG_D("res=%d,%d", res, __LINE__);
                        }
                        else
                        {
                            mb_resp_msg(&scz_gms, MB_STATN_BUSI, res); //回应消息,释放信号量
                            LOG_D("res=%d,%d", res, __LINE__);
                        }
                        break;
                    }
                    }
                    break;
                }
                case MB_STATN_SPVW0: //消息是从SPVW0发送过来的
                {
                    switch (scz_gms.d_cmd.cmd)
                    {
                    case SPVW_RET_SAMPLE: //返回的是采样数据
                    {
                        SCZ_SAM_SPVW0_DAT_RES_STRU *d = (SCZ_SAM_SPVW0_DAT_RES_STRU *)scz_gms.d_p; //获取返回的数据指针，0:host_ch0  1-16:ex0-15;    20:host_ch1;    30:host_ch2
                        //LOG_D("SCZ Receive SPVW data");

                        switch (d->channel)
                        {
                        case SCZ_HOST_CH0:
                            input_r->HOST_EX_CH0_F = (unsigned short)(d->freq * 10);
                            input_r->HOST_EX_CH0_T = (short)(d->temp * 100);
                            break;
                        case SCZ_HOST_CH1:
                            input_r->HOST_EX_CH1_F = (unsigned short)(d->freq * 10);
                            if (hold_r->is_use_ex)
                            {
                                input_r->EX_CH1_T = (short)(d->temp * 100);
                            }
                            break;
                        case SCZ_HOST_CH2:
                            input_r->HOST_EX_CH2_F = (unsigned short)(d->freq * 10);
                            if (hold_r->is_use_ex)
                            {
                                input_r->EX_CH2_T = (short)(d->temp * 100);
                            }
                            break;
                        case SCZ_EX_CH0:
                            input_r->HOST_EX_CH0_F = (unsigned short)(d->freq * 10);
                            input_r->HOST_EX_CH0_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH1:
                            input_r->HOST_EX_CH1_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH1_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH2:
                            input_r->HOST_EX_CH2_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH2_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH3:
                            input_r->EX_CH3_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH3_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH4:
                            input_r->EX_CH4_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH4_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH5:
                            input_r->EX_CH5_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH5_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH6:
                            input_r->EX_CH6_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH6_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH7:
                            input_r->EX_CH7_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH7_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH8:
                            input_r->EX_CH8_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH8_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH9:
                            input_r->EX_CH9_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH9_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH10:
                            input_r->EX_CH10_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH10_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH11:
                            input_r->EX_CH11_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH11_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH12:
                            input_r->EX_CH12_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH12_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH13:
                            input_r->EX_CH13_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH13_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH14:
                            input_r->EX_CH14_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH14_T = (short)(d->temp * 100);
                            break;
                        case SCZ_EX_CH15:
                            input_r->EX_CH15_F = (unsigned short)(d->freq * 10);
                            input_r->EX_CH15_T = (short)(d->temp * 100);
                            break;
                        default:
                            break;
                        }
                        if (hold_r->is_use_ex) //如果使用了扩展器
                        {
                            input_r->EX_V = (unsigned short)(d->e_vol * 100);
                            LOG_D("SCZ rx channel：%d,db:%d,err_cnt：%d,freq：%.1f,temp：%.2f,e_vol：%.2f", d->channel, d->db, d->err_cnt, d->freq, d->temp, d->e_vol);
                        }
                        else
                        {
                            if (d->channel == 0)
                            {
                                LOG_D("SCZ rx channel：%d,db:%d,err_cnt：%d,freq：%.1f,temp：%.2f", d->channel, d->db, d->err_cnt, d->freq, d->temp);
                            }
                            else
                            {
                                LOG_D("SCZ rx channel：%d,db:%d,err_cnt：%d,freq：%.1f", d->channel, d->db, d->err_cnt, d->freq);
                            }
                        }

                        break;
                    }

                    case SPVW_RET_ACK: //返回的是ACK信息
                    {
                        uint8_t ack_d = *(uint8_t *)scz_gms.d_p;
                        if(ack_d==254)
                        {
                            rt_thread_mdelay(100);
                            rt_sem_release(&scz_sam_spvw_complete_sem); //处理完成后发送信号量
                        }
                        
                        LOG_D("SCZ Receive SPVW ack status=%d", ack_d);
                        break;
                    }
                    default:
                        break;
                    }

                    break;
                }

                default:
                    break;
                }
                mb_resp_msg(&scz_gms, BUSI_CPID, 0); //释放信号量并返回结果
            }
            else //使用目标的指令解析
            {
                mb_resp_msg(&scz_gms, BUSI_CPID, 0); //释放信号量并返回结果
            }
        }
    }
}

void BUSI_register_entry(void *p)
{
    unsigned char i = 0;
    get_pro_ini();
    ulog_tag_lvl_filter_set(LOG_TAG, pro_ini.PRO_LOG_LVL);
    {

        busi_reg.p_pipe = &busi_pipe;
        {
            sem_reg = rt_sem_create("busi_reg", 0, RT_IPC_FLAG_FIFO); //初始化用于消息同步的信号量，接受者在完成消息处理后，必要时需要对信号量进行释放

            rt_memset(&scz_dmgms, 0, sizeof(DM_GMS_STRU));                                                                                                  //清空多维消息体
            mb_make_dmgms(&scz_dmgms, 0, sem_reg, CP_CMD_DST(MAINPIPE_CMD_CPREG), MB_STATN_MAIN, MB_STATN_BUSI, &busi_reg, sizeof(CP_REG_STRU), &resp_reg); //向多维消息体中装入消息

            rt_mq_send(busi_ctl.p_mainpipe, &scz_dmgms, sizeof(DM_GMS_STRU)); //向主管道发送注册命令

            if (RT_EOK != rt_sem_take(sem_reg, BUSI_REG_SEM_WAIT_TIME)) //等待接收者释放信号量，超时将造成组件注册失败
            {
                LOG_E("cp-reg ot!");
                return;
            }

            if (MB_STATN_MAIN == resp_reg.r_src && MB_RESP_SUC == resp_reg.r_res) //注册成功
            {
                busi_ctl.is_registered = 1;

                LOG_D("BUSI_-reg suc!");
            }
            else
            {
                LOG_E("cp-reg fail!");
                return;
            }
        }
    }

    {
        if (rt_mq_recv(&busi_pipe, &scz_gms, sizeof(GMS_STRU), RT_WAITING_FOREVER) == RT_EOK) //从BUSI组件管道获取消息
        {
            LOG_D("BUSI_scz_gms22222222222222!");
            if (scz_gms.d_cmd.is_src_cmd) //是源头命令
            {
                if (MB_STATN_MAIN == scz_gms.d_src) //来自主管道
                {
                    if (MAINPIPE_CMD_STARTUP == scz_gms.d_cmd.cmd) //业务启动命令
                    {
                        LOG_D("BUSI_scz_gms333333333333333!");
                        rt_thread_init(&rthread_msg_pro, "busi_msg_pro", BUSI_msg_pro_entry, RT_NULL, thread_msg_pro_stack, sizeof(thread_msg_pro_stack), THREAD_PRIORITY, THREAD_TIMESLICE);
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
        LOG_E("pipe init fail!");
        return;
    }

    {
        busi_ctl.p_mainpipe = ((CP_ENTRY_ARGS_STRU *)p)->p_mq;
        busi_ctl.p_lib = ((CP_ENTRY_ARGS_STRU *)p)->p_lib;

        //将动态库中的函数导出
        //这样的作法是为了在函数层面解耦
        //{
        //funcs.pfun_=dlsym(comnb_ctl.p_lib,"xxx"); //从动态库(即so中找到名为xxx的符号，即函数名)
        //}
    }

    rt_thread_init(&rthread_register, "busi_reg", BUSI_register_entry, RT_NULL, thread_register_stack, sizeof(thread_register_stack), THREAD_PRIORITY - 7, THREAD_TIMESLICE);
    rt_thread_startup(&rthread_register);

    return;
}
