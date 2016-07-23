////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (��MStar Confidential Information��) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
///
/// file    drvTuner_TDA18250A.c
/// @author MStar Semiconductor Inc.
//////////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------



#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmFrontEnd.h"
#include "tmbslFrontEndTypes.h"
#include "tda18250b.h"

//*--------------------------------------------------------------------------------------
//* Include Driver files
//*--------------------------------------------------------------------------------------
#include "tmbslTDA18250A.c"
#include "tmbslTDA18250A_Advanced.c"


#define TDA18250A_ADDR             0x60
#define RET_SUCCESS 0
#define RET_ERROR 1


//*--------------------------------------------------------------------------------------
//* Prototype of function to be provided by customer
//*--------------------------------------------------------------------------------------
static tmErrorCode_t     UserWrittenI2CRead(tmUnitSelect_t tUnit,UInt32 AddrSize, UInt8* pAddr,UInt32 ReadLen, UInt8* pData);
static tmErrorCode_t     UserWrittenI2CWrite (tmUnitSelect_t tUnit, UInt32 AddrSize, UInt8* pAddr,UInt32 WriteLen, UInt8* pData);
static tmErrorCode_t     UserWrittenWait(tmUnitSelect_t tUnit, UInt32 tms);

#define MAX_FRONTEND_NUM            1

#define TRUE 0
#define FALSE 1
#define LOG_DISPLAY

 int i2c_write(struct regmap *rmap, u8 adr,u8 regaddr, u8 * data, u32 len,u8 contiguous)
 {

	if(!data){
	    	printk(KERN_ERR " addr %#02x reg %#02x failed data null\n",adr,regaddr);
	    	return RET_ERROR;
	    }
	if (rmap) {
		if (len > 1) {
			u32 i;

			if (contiguous == 1) {
				if (regmap_bulk_write(rmap, regaddr, data, len) == 0) {
#ifdef LOG_DISPLAY
					for (i = 0; i < len; i++) {
						printk(KERN_DEBUG ">%#02x[%#02x] %#08x\n", adr,
								(regaddr + i), data[i]);
					}
#endif
				} else {
					printk(
							KERN_ERR "failed to write i2c len %u bus %#02x reg %#02x contiguous\n",
							len, adr, regaddr);
					return RET_ERROR;
				}
			} else {

				u32 lentmp = len;
				u32 i = 0;
				while (lentmp--) {
					if (regmap_write(rmap, regaddr, data[i]) == 0) {

					} else {
						printk(
								KERN_ERR "failed to write i2c bus %#02x reg %#02x\n",
								adr, regaddr);
						return RET_ERROR;
					}
					i++;
				}
#ifdef LOG_DISPLAY
				for (i = 0; i < len; i++) {
					printk(KERN_DEBUG ">%#02x[%#02x] %#08x\n", adr, regaddr,
							data[i]);
				}
#endif
			}

			return RET_SUCCESS;
		}

		if (regmap_write(rmap, regaddr, *data) == 0) {
#ifdef LOG_DISPLAY
			printk(KERN_DEBUG ">%#02x[%#02x] %#08x\n", adr, regaddr, *data);
#endif
			return RET_SUCCESS;
		}

	}

	printk(KERN_ERR "failed to write i2c bus %#02x reg %#02x\n", adr, regaddr);
	return RET_ERROR;

}

  int i2c_read(struct regmap *rmap,
                     u8 adr,u8 regaddr, u8 *pdata, u32 len,u8 contiguous)
 {

	if (!pdata) {
		printk(KERN_ERR "Read addr %#02x reg %#02x data null\n", adr, regaddr);
		return RET_ERROR;
	}
	if (rmap) {
		if (len > 1) {

			u32 i;
			if (contiguous == 1) {
				if (regmap_bulk_read(rmap, regaddr, (int*) pdata, len) == 0) {
#ifdef LOG_DISPLAY
					for (i = 0; i < len; i++) {
						printk(KERN_DEBUG "<%#02x[%#02x] %#08x\n", adr,
								(regaddr + i), pdata[i]);
					}
#endif
				} else {
					printk(
							KERN_ERR "failed to read i2c len %u bus %#02x reg %#02x contiguous\n",
							len, adr, regaddr);

					return RET_ERROR;
				}
			} else {
				u32 lentmp = len;
				u32 i = 0;
				while (lentmp--) {
					if (regmap_read(rmap, regaddr, (int*) pdata + i) == 0) {

					} else {
						printk(
								KERN_ERR "failed to read i2c  len %#02x bus %#02x reg %#02x \n",
								len, adr, regaddr);

						return RET_ERROR;
					}
					i++;
				}
#ifdef LOG_DISPLAY
				for (i = 0; i < len; i++) {
					printk(KERN_DEBUG "<%#02x[%#02x] %#08x\n", adr, regaddr,
							pdata[i]);
				}
#endif

			}

			return RET_SUCCESS;
		}

		if (regmap_read(rmap, regaddr, (int*) pdata) == 0) {
#ifdef LOG_DISPLAY
			printk(KERN_DEBUG "<%#02x[%#02x] %#08x\n", adr, regaddr, *pdata);
#endif
			return RET_SUCCESS;
		}

	}

	printk(KERN_ERR "failed to read i2c len %u bus %#02x reg %#02x \n", len,
			adr, regaddr);
	return RET_ERROR;

}

  static Bool tda_write (UInt8 uAddress, UInt8 uSubAddress, UInt32 uNbData, UInt8 * pDataBuff)
  {

  	int s32Ret;
  	struct tda18250b_dev* mdev;

  	u8 *pu8Tmp = NULL;

  	if (NULL == pDataBuff)
  	{
  		printk( "pointer is null\n");
  		return -1;
  	}

  	pu8Tmp = pDataBuff;
  	mdev = tda18250b_getdev();
  	if(!mdev){
  		printk(KERN_ERR "tda_write mdev NULL\n ");
  		return -1;
  	}
  	s32Ret=i2c_write(mdev->regmap, uAddress,uSubAddress, pDataBuff, uNbData,1);
  	    	if(s32Ret==RET_ERROR){
  	    		s32Ret = 1;
  	    		printk(KERN_ERR "tda_write return error for add %#02x reg %#02x \n",uAddress,uSubAddress);
  	    	}else{
  	    		s32Ret = 0;
  	    	}
  	        if(0 != s32Ret)
  	        {
  	            return s32Ret;
  	        }

  	return 0;

  }

  static Bool tda_read (UInt8 uAddress, UInt8 uSubAddress, UInt32 uNbData, UInt8 * pDataBuff)
  {

  	int s32Ret;
  	struct tda18250b_dev* mdev;

  	mdev = tda18250b_getdev();
  	if(!mdev){
  			printk(KERN_ERR "tda_write mdev NULL\n ");
  			return -1;
  		}

  	    	s32Ret=i2c_read(mdev->regmap, uAddress,uSubAddress, pDataBuff, uNbData,0);
  	    	if(s32Ret==RET_ERROR){
  	    	    		s32Ret = 1;
  	    	    		printk(KERN_ERR "tda_read return error for add %#02x reg %#02x \n",uAddress,uSubAddress);
  	    	    	}else{
  	    	    		s32Ret = 0;
  	    	    	}
  	        if(0 != s32Ret)
  	        {
  	            return s32Ret;
  	        }


  	return 0;
  }


bool tda18250b_set_tuner(u32 rFrequency,enum fe_delivery_system	delivery_system, enum fe_modulation modulation)
{
	TDA18250AStandardMode_t standard_mode;
	tmErrorCode_t err = TM_OK;
	tmbslFrontEndState_t PLLLockMaster = tmbslFrontEndStateUnknown;

	switch(delivery_system){
	case SYS_DVBT:
	case SYS_DVBT2:
		standard_mode = TDA18250A_DVBT_8MHz;break;

	case SYS_DVBC_ANNEX_A:
		standard_mode = TDA18250A_QAM_8MHz;
		break;
	default: standard_mode = TDA18250A_QAM_8MHz;
	break;
	}
	printk(KERN_DEBUG "stdMode %u\n",standard_mode);
	if(TM_OK != tmbslTDA18250A_SetStandardMode(0, 0, standard_mode))
		printk("!!!!! standard_mode error\n");


	err = tmbslTDA18250A_SetRF(0,0, rFrequency);
	if(err != TM_OK)
		return err;

	err = tmbslTDA18250A_GetPLLState(0,0, &PLLLockMaster);


	return err;
}


bool tda18250B_Init(u8 u8TunerIndex)
{
    tmErrorCode_t               err = TM_OK;
    tmbslFrontEndDependency_t   sSrvTunerFunc;
    UInt32                      TunerUnit = 0;
    TDA18250AStandardMode_t     TDA18250AStdMode = TDA18250A_QAM_8MHz;

    sSrvTunerFunc.sIo.Write = UserWrittenI2CWrite;
    sSrvTunerFunc.sIo.Read = UserWrittenI2CRead;

    sSrvTunerFunc.sTime.Get             = Null;
    sSrvTunerFunc.sTime.Wait            = UserWrittenWait;
    sSrvTunerFunc.sMutex.Init           = Null;
    sSrvTunerFunc.sMutex.DeInit         = Null;
    sSrvTunerFunc.sMutex.Acquire        = Null;
    sSrvTunerFunc.sMutex.Release        = Null;
    sSrvTunerFunc.dwAdditionalDataSize  = 0;
    sSrvTunerFunc.pAdditionalData       = Null;

    // Reset the XTAL CLK to 18250B Only
    tmbslTDA18250B_ResetConfig(u8TunerIndex);

    /* Open TDA18250A driver instance */
    err = tmbslTDA18250A_Open(u8TunerIndex,TunerUnit, &sSrvTunerFunc);
    printk("\n=== TDA18250B open, result[0x%lX]. \n", err);
    if(err == TM_OK)
    {
        /* TDA18250A Power On */
        err = tmbslTDA18250A_SetPowerState(u8TunerIndex,TunerUnit, tmPowerOn);
        printk("\n=== TDA18250B SetPowerState, result[0x%lX]. \n", err);

        /* TDA18250A Hardware initialization */
        err = tmbslTDA18250A_HwInit(u8TunerIndex,TunerUnit);
        printk("\n=== TDA18250B HW init status, result[0x%lX]. \n", err);

        /* TDA18250A Power On */
        err = tmbslTDA18250A_SetPowerState(u8TunerIndex,TunerUnit, tmPowerOn);
        printk("\n=== TDA18250B Set2ndPowerState, result[0x%lX]. \n", err);

        if(err == TM_OK)
        {
          /* TDA18250A standard mode if changed */
          err = tmbslTDA18250A_SetStandardMode(u8TunerIndex, TunerUnit, TDA18250AStdMode);
          printk("\n=== TDA18250B init SetStandardMode status[0x%lX]. \n", err);
        }

    }
    else
    {
    	if(err==TDA18250A_ERR_ALREADY_SETUP){
    		return TRUE;
    	}
        printk("\n=== TDA18250B open fail[%ld]. \n", err);
    }

    #if 0   //temp
    if(err == TM_OK)
    {
      /* Get TDA18250A PLL Lock status */
      err = tmbslTDA18250A_GetLockStatus(TunerUnit, &PLLLock);
    }
    #endif
    if(err == TM_OK)
    {
        return TRUE;
    }
    else
    {
        printk("\n=== TDA18250B init fail, status[%ld]. \n", err);
        return FALSE;
    }

}

bool tda18250A_Init(u8 u8TunerIndex)
{
    tmErrorCode_t               err = TM_OK;
    tmbslFrontEndDependency_t   sSrvTunerFunc;
    UInt32                      TunerUnit = 0;
    TDA18250AStandardMode_t     TDA18250AStdMode = TDA18250A_QAM_8MHz;


    sSrvTunerFunc.sIo.Write = UserWrittenI2CWrite;
            sSrvTunerFunc.sIo.Read = UserWrittenI2CRead;
    sSrvTunerFunc.sTime.Get             = Null;
    sSrvTunerFunc.sTime.Wait            = UserWrittenWait;
    sSrvTunerFunc.sMutex.Init           = Null;
    sSrvTunerFunc.sMutex.DeInit         = Null;
    sSrvTunerFunc.sMutex.Acquire        = Null;
    sSrvTunerFunc.sMutex.Release        = Null;
    sSrvTunerFunc.dwAdditionalDataSize  = 0;
    sSrvTunerFunc.pAdditionalData       = Null;

    /* Open TDA18250A driver instance */
    err = tmbslTDA18250A_Open(u8TunerIndex,TunerUnit, &sSrvTunerFunc);
    printk("\n=== TDA18250A open, result[0x%lX]. \n", err);
    if(err == TM_OK)
    {
        /* TDA18250A Power On */
        err = tmbslTDA18250A_SetPowerState(u8TunerIndex,TunerUnit, tmPowerOn);
        printk("\n=== TDA18250A SetPowerState, result[0x%lX]. \n", err);

        /* TDA18250A Hardware initialization */
        err = tmbslTDA18250A_HwInit(u8TunerIndex,TunerUnit);
        printk("\n=== TDA18250A HW init status, result[0x%lX]. \n", err);

        /* TDA18250A Power On */
        err = tmbslTDA18250A_SetPowerState(u8TunerIndex,TunerUnit, tmPowerOn);
        printk("\n=== TDA18250A Set2ndPowerState, result[0x%lX]. \n", err);

        if(err == TM_OK)
        {
          /* TDA18250A standard mode if changed */
          err = tmbslTDA18250A_SetStandardMode(u8TunerIndex, TunerUnit, TDA18250AStdMode);
          printk("\n=== TDA18250A init SetStandardMode status[0x%lX]. \n", err);
        }

    }
    else
    {
        printk("\n=== TDA18250A open fail[%ld]. \n", err);
    }

    #if 0   //temp
    if(err == TM_OK)
    {
      /* Get TDA18250A PLL Lock status */
      err = tmbslTDA18250A_GetLockStatus(TunerUnit, &PLLLock);
    }
    #endif
    if(err == TM_OK)
    {
        return TRUE;
    }
    else
    {
        printk("\n=== TDA18250A init fail, status[%ld]. \n", err);
        return FALSE;
    }

}

tmErrorCode_t UserWrittenI2CRead(tmUnitSelect_t tUnit, UInt32 AddrSize, UInt8* pAddr,
UInt32 ReadLen, UInt8* pData)
{

    tmErrorCode_t err = TM_OK;
    err = tda_read(TDA18250A_ADDR, *pAddr, (u16)ReadLen, pData );
    return err;
}

tmErrorCode_t UserWrittenI2CWrite (tmUnitSelect_t tUnit,   UInt32 AddrSize, UInt8* pAddr,
UInt32 WriteLen, UInt8* pData)
{

    tmErrorCode_t err = TM_OK;
    err=tda_write(TDA18250A_ADDR, *pAddr, (u16)WriteLen,pData );

    return err;
}


tmErrorCode_t UserWrittenWait(tmUnitSelect_t tUnit, UInt32 tms)
{

    tmErrorCode_t err = TM_OK;
    msleep_interruptible(tms);
    return err;
}



#define TDA18250A_CHIP_ID_1 0x4a
#define TDA18250A_CHIP_ID_0 0xc7
#define TDA18250A_REVISION_0 0x20
#define TDA18250B_REVISION_0 0x21





