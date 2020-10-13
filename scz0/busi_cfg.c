#include "busi_cfg.h"
#include "minini.h"
#include "dfs_fs.h"
#define LOG_TAG "SCZ_CFG"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

UTC_CLOCK_CONFIG_STRU clock_cfg = {0};

unsigned char sn[SN_LENGTH + 1] = {0};

hold_reg_t hold_reg = HOLD_REG_DEF;
input_reg_t input_reg;
input_reg_t *input_r = NULL;

hold_reg_t *hold_r = NULL;
unsigned char get_sam_cfg() //获取采样相关参数
{
    int fd;
    int rn;
    fd = open(BSAM_CFG_FILENAME, O_RDONLY);
    if (-1 == fd)
    {
        LOG_W("\"%s\"openfail", BSAM_CFG_FILENAME);
        return RT_ERROR;
    }
    rn = read(fd, &hold_reg, sizeof(hold_reg));
    if (rn != sizeof(hold_reg))
    {
        LOG_E("readerr");
        close(fd);
        return RT_ERROR;
    }
    close(fd);
}

unsigned char get_sn() //获取sn号
{
    int fd;
    int rn;
    fd = open(SN_FILENAME, O_RDWR);
    if (-1 == fd)
    {
        LOG_W("\"%s\"open fail", SN_FILENAME);
        close(fd);
        return RT_ERROR;
    }
    rn = read(fd, sn, SN_LENGTH);
    if (rn != SN_LENGTH)
    {
        LOG_E("read err");
        close(fd);
        return RT_ERROR;
    }
    close(fd);
    return RT_EOK;
}




unsigned char BUSI_sam_cfg_save(hold_reg_t *s_cfg)
{
    struct stat stat_buf;
    int fd;
    unsigned char wn = 0;
    rt_err_t error = RT_EOK;
    LOG_D("SaveCfg:%s", BSAM_CFG_FILENAME);
    fd = open(BSAM_CFG_FILENAME, O_CREAT |O_WRONLY);
    if (-1 == fd)
    {
        LOG_E("\"%s\"File open failed", BSAM_CFG_FILENAME);
        return RT_ERROR;
    }
    wn = write(fd, s_cfg, sizeof(hold_reg_t));
    if (wn != sizeof(hold_reg_t))
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
        LOG_E("read ini LOG_LVL fail");
    ret = ini_getl("product_info", "PRODUCT_TYPE", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.PRODUCT_TYPE = ret;
    else
        LOG_E("read ini PRODUCT_TYPE fail");
    ret = ini_getl("product_info", "VERSION", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.VERSION = ret;
    else
        LOG_E("read ini VERSION fail");
    ret = ini_getl("modbus_cfg", "RS_BAUD_RATE", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.RS_BAUD_RATE = ret;
    else
        LOG_E("read ini RS_BAUD_RATE fail");
    ret = ini_getl("modbus_cfg", "RS_4X_LEN", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.RS_4X_LEN = ret;
    else
        LOG_E("read ini RS_4X_LEN fail");
    ret = ini_getl("modbus_cfg", "RS_3X_LEN", -1, BUSI_INI_FILENAME);
    if (ret != -1)
        pro_ini.RS_3X_LEN = ret;
    else
        LOG_E("read ini RS_3X_LEN fail");
    LOG_D("read ini suc");
    return ret;
}