#ifndef _BUSI_MANAGER_H_
#define _BUSI_MANAGER_H_

//注:将BUSI替换为实际mapp名，形如UTCM STOM


#define BUSI_CPID (7)

#define BUSI_PIPE_NMSG (20)
#define BUSI_POOL_SIZE (sizeof(GMS_STRU)*BUSI_PIPE_NMSG)
#define BUSI_REG_SEM_WAIT_TIME (1000)

extern CP_CTL_STRU busi_ctl;


#endif