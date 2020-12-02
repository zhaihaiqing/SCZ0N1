#include "busi_cfg.h"
#include "busi_pro.h"
#include "minini.h"
#include "dfs_fs.h"
#define LOG_TAG "BUSI_CFG"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

UTC_CLOCK_CONFIG_STRU clock_cfg = {0};

unsigned char sn[SN_LENGTH + 1] = {0};
unsigned char nodeid;
busi_sam_cfg_t sam_cfg = SAM_CFG_DEF;
pro_ini_t pro_ini = PRO_INI_DEF;

vw_cfg_reg_t scz_cfg_par; //采集参数，采集模式、通道配置


//char * ini_key[]= {"PRO_LOG_LVL","PRODUCT_TYPE","VERSION"};
//Init_info_t init_info;
unsigned char get_com_cfg(char *filename, void *d_p,unsigned short d_l) //获取其他组件配置
{
    int fd;
    int rn;
    fd = open(filename, O_RDONLY);
    if (-1 == fd)
    {
        LOG_W("\"%s\"File open failed ", filename);
        close(fd);
        return RT_ERROR;
    }
    rn = read(fd,d_p, d_l);
    if (rn != d_l)
    {
        LOG_W("read %s len error",filename);
        close(fd);
        return RT_ERROR;
    }
    close(fd);
}

void set_clk1_sam_def_cfg()
{
    sam_cfg.running_mode = RUN_MODE;
    sam_cfg.odr = ODR;
    sam_cfg.range = RANGE;
    sam_cfg.axis = AXIS;
}
void set_clk2_sam_def_cfg()
{
    sam_cfg.running_mode_s = RUN_MODE_S;
    sam_cfg.odr_s = ODR_S;
    sam_cfg.range_s = RANGE_S;
    sam_cfg.axis_s = AXIS_S;
}

/**
 * 字符串中非c字符有几个
 */
char my_strchr(char const *p, char c)
{
    char n = 5; //sn后五位
    while (*p)
    {
        if (*p != c)
            return n;
        else
            n--;
        p++;
    }
    return n;
}


unsigned char get_scz0_cfg(void)
{
    struct stat stat_buf;
    int fd = 0, size = 0;
    int res = 0, status = 0;

    vw_cfg_reg_t scz_cfg_par_tmp;

    res = stat(BUSI_SCZ_CFG_FILENAME, &stat_buf); //获取文件信息，文件存在，则加载数据，文件不存在则创建文件并初始化为0

    if (res == 0) //获取文件信息成功，说明文件存在，加载全部文件
    {
        LOG_W("SCZ0.CFG read info OK,size:%d", stat_buf.st_size);
        fd = open(BUSI_SCZ_CFG_FILENAME, O_RDONLY); //只读打开文件
        if (fd > 0)                       //打开成功
        {
            size = read(fd, &scz_cfg_par, sizeof(vw_cfg_reg_t));
            close(fd);
            //LOG_E("l_ch:0x%x,e_ch:0x%x", scz_cfg_par.local_ch[0], scz_cfg_par.ex_ch[0]);
            status = 1;
        }
        else
        {
            LOG_E("open SCZ0 failure");
            status = 0;
        }

        if(stat_buf.st_size != sizeof(vw_cfg_reg_t))
        {
            res=-1;
        }
    }
    else if (res == -1) //获取文件信息失败，说明文件不存在
    {
        fd = open(BUSI_SCZ_CFG_FILENAME, O_CREAT | O_WRONLY); //以创建和读写的方式创建文件
        if (fd > 0)                                 //创建成功
        {
            memset(&scz_cfg_par_tmp, 0, sizeof(vw_cfg_reg_t));
            write(fd, &scz_cfg_par_tmp, sizeof(vw_cfg_reg_t));
            close(fd);
            LOG_D("Creat and init SCZ0.CFG Ok");
            status = 1;
        }
        else
        {
            LOG_E("Creat SCZ0.CFG failure!");
            status = 0;
        }
    }
    return status; //如果成功，返回0，如果不成功，返回对应的标志位
}

// unsigned char save_scz0_cfg(void)
// {
//     int fd = 0, res = 0, size = 0;

//     fd = open(BUSI_SCZ_CFG_FILENAME, O_WRONLY); //以读写方式打开设备表

//     if (fd > 0)                     //如果成功
//     {
//          LOG_E("l_ch:0x%x,e_ch:0x%x", scz_cfg_par.local_ch[0], scz_cfg_par.ex_ch[0]);

//         size=write(fd, &scz_cfg_par, sizeof(vw_cfg_reg_t));

//         if(size!=sizeof(vw_cfg_reg_t))
//         {
//             LOG_E("Write SCZ0.CFG failure!");
//             res=1;
//         }
//         else
//         {
//             res=0;
//         }
//     }
//     else
//     {
//         LOG_E("Open SCZ0.CFG failure!");
//         res=1;
//     }
    
//     return res;
// }


unsigned char save_scz0_cfg(vw_cfg_reg_t *s_cfg)
{
    struct stat stat_buf;
    int fd;
    unsigned char wn = 0;
    rt_err_t error = RT_EOK;
    LOG_D("SaveCfg:%s", BUSI_SCZ_CFG_FILENAME);
    fd = open(BUSI_SCZ_CFG_FILENAME, O_CREAT | O_WRONLY);
    if (-1 == fd)
    {
        LOG_E("\"%s\"File open failed", BUSI_SCZ_CFG_FILENAME);
        close(fd);
        return RT_ERROR;
    }
    wn = write(fd, s_cfg, sizeof(vw_cfg_reg_t));
    if (wn != sizeof(vw_cfg_reg_t))
    {
        LOG_E("wcfgerr");
        close(fd);
        return RT_ERROR;
    }
    close(fd);

    return 0;
}


unsigned char BUSI_sam_cfg_save(busi_sam_cfg_t *s_cfg)
{
    struct stat stat_buf;
    int fd;
    unsigned char wn = 0;
    rt_err_t error = RT_EOK;
    LOG_D("SaveCfg:%s", BSAM_CFG_FILENAME);
    fd = open(BSAM_CFG_FILENAME, O_CREAT | O_WRONLY);
    if (-1 == fd)
    {
        LOG_E("\"%s\"File open failed", BSAM_CFG_FILENAME);
        close(fd);
        return RT_ERROR;
    }
    wn = write(fd, s_cfg, sizeof(busi_sam_cfg_t));
    if (wn != sizeof(busi_sam_cfg_t))
    {
        LOG_E("wcfgerr");
        close(fd);
        return RT_ERROR;
    }
    close(fd);

    return 0;
}

int get_pro_ini()
{
    int ret = 0;
    ret = ini_getl("product_info", "PRO_LOG_LVL", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.PRO_LOG_LVL = ret;
    else
        LOG_E("ini%d", __LINE__);
    ret = ini_getl("product_info", "PRODUCT_TYPE", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.PRODUCT_TYPE = ret;
    else
        LOG_E("ini%d", __LINE__);
    ret = ini_getl("product_info", "VERSION", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.VERSION = ret;
    else
        LOG_E("ini%d", __LINE__);
    ret = ini_getl("product_info", "INTERVAL", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.INTERVAL = ret;
    else
    {
        pro_ini.INTERVAL = 900;
        LOG_E("ini%d", __LINE__);
    }

    ret = ini_getl("product_info", "CLK_IS", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.CLK_IS_ON = ret;
    else
    {
        pro_ini.CLK_IS_ON = 1;
        LOG_E("ini%d", __LINE__);
    }

    ret = ini_getl("product_info", "CLK_NUM", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.CLK_NUM = ret;
    else
    {
        pro_ini.CLK_NUM = 0;
        LOG_E("ini%d", __LINE__);
    }
    ret = ini_getl("product_info", "BATTERY_TYPE", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.BATTERY_TYPE = ret;
    else
        LOG_E("ini%d", __LINE__);
    ret = ini_getl("product_info", "BATTERY_NUM", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.BATTERY_NUM = ret;
    else
        LOG_E("ini%d", __LINE__);
    return ret;
}

