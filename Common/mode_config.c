/***********************************************************************************************************************

   Filename: mode_config.c

   Global Designator: MODECFG_

   Contents: Configuration parameter I/O for MTU mode.

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2015 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author David McGraw

   $Log$

 ***********************************************************************************************************************
   Revision History:

   @version
   #since
 **********************************************************************************************************************/

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <bsp.h>

#include "compiler_types.h"
#include "file_io.h"

#define MODECFG_GLOBALS
#include "mode_config.h"
#undef MODECFG_GLOBALS

#if ( EP == 1 )
#include "vbat_reg.h"
#endif

#if (ACLARA_DA == 1)
#include "b2bmessage.h"
#endif

#include "DBG_SerialDebug.h"
#include "HEEP_util.h"

/* ****************************************************************************************************************** */
/* GLOBAL DEFINTIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint16_t fileVersion;
   uint8_t uShipMode;
   uint8_t uQuietMode;
#if ( EP == 1)
   uint8_t urfTestMode;
   uint8_t uDecommissionMode;
#endif
} modeConfigFileData_t;

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define MODECFG_FILE_UPDATE_RATE_CFG_SEC ((uint32_t) 30 * 24 * 3600)    /* File updates once per month. */
#define MODECFG_FILE_VERSION 1

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static FileHandle_t  modeConfigFileHandle = {0};
static OS_MUTEX_Obj modeConfigMutex;

static modeConfigFileData_t modeConfigFileData = {0};
static uint8_t fileDataValid = false;


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function name:

   Purpose:

   Arguments:

   Returns: void

   Re-entrant Code:

   Notes:

 **********************************************************************************************************************/

returnStatus_t MODECFG_init( void )
{
   FileStatus_t   fileStatus;
   returnStatus_t retVal = eFAILURE;

   if ( OS_MUTEX_Create( &modeConfigMutex ) )
   {
      if ( eSUCCESS == FIO_fopen(  &modeConfigFileHandle, ePART_SEARCH_BY_TIMING, ( uint16_t )eFN_MODECFG,
                                   ( lCnt )sizeof( modeConfigFileData ),
                                   FILE_IS_NOT_CHECKSUMED, MODECFG_FILE_UPDATE_RATE_CFG_SEC, &fileStatus ) )
      {
         if ( fileStatus.bFileCreated )
         {
            modeConfigFileData.fileVersion = MODECFG_FILE_VERSION;
            modeConfigFileData.uShipMode = 0;
            modeConfigFileData.uQuietMode = 0;
#if ( EP == 1)
            modeConfigFileData.urfTestMode = 0;
            modeConfigFileData.uDecommissionMode = 0;
#endif
            retVal = FIO_fwrite( &modeConfigFileHandle, 0, ( uint8_t* ) &modeConfigFileData,
                                 ( lCnt )sizeof( modeConfigFileData ) );
         }
         else
         {
            retVal = FIO_fread( &modeConfigFileHandle, ( uint8_t* ) &modeConfigFileData, 0,
                                ( lCnt )sizeof( modeConfigFileData ) );
         }
      }
   }

   (void)MODECFG_get_initialCFGMode();

#if ( EP == 1 )
   // Save ship mode and meter shop mode in VBAT before it is cleared so outage
   // restoration will if we started up in ship mode.
   VBATREG_SHIP_MODE = modeConfigFileData.uShipMode;
   VBATREG_METER_SHOP_MODE = modeConfigFileData.uDecommissionMode;

#endif

   if ( eSUCCESS == retVal )
   {
      fileDataValid = true;
   }

   return( retVal );

}

/***********************************************************************************************************************

   Function name: MODECFG_get_ship_mode

   Purpose: get system "ship mode" setting

   Arguments: none

   Returns: uint8_t system ship mode;

   Re-entrant Code: yes

   Notes:

 **********************************************************************************************************************/

uint8_t MODECFG_get_ship_mode( void )
{
   uint16_t uShipMode = 0;

   if ( fileDataValid )
   {
      uShipMode = modeConfigFileData.uShipMode;
   }

   return( ( uint8_t )uShipMode );
}

/***********************************************************************************************************************

   Function name: MODECFG_set_ship_mode

   Purpose: set system ship mode

   Arguments:  ship mode setting

   Returns: success/failure

   Re-entrant Code: yes

   Notes:

 **********************************************************************************************************************/

returnStatus_t MODECFG_set_ship_mode( uint8_t uShipMode )
{
   returnStatus_t retVal;

   OS_MUTEX_Lock( &modeConfigMutex ); // Function will not return if it fails
   modeConfigFileData.uShipMode = uShipMode;
   retVal = FIO_fwrite( &modeConfigFileHandle, 0, ( uint8_t* ) &modeConfigFileData,
                        ( lCnt )sizeof( modeConfigFileData ) );
   OS_MUTEX_Unlock( &modeConfigMutex ); // Function will not return if it fails

   if ( eSUCCESS == retVal )
   {
#if ( EP == 1 )
      VBATREG_SHIP_MODE = modeConfigFileData.uShipMode;
      LED_checkModeStatus();
#endif
      DBG_logPrintf( 'I', "eFN_MODECFG Write Successful" );
   }
   else
   {
      DBG_logPrintf( 'E', "eFN_MODECFG Write Failed" );
   }

   return( retVal );
}

/***********************************************************************************************************************

   Function name: MODECFG_get_quiet_mode

   Purpose: get system "quiet mode" setting

   Arguments: none

   Returns: uint8_t system quiet mode;

   Re-entrant Code: yes

   Notes:

 **********************************************************************************************************************/

uint8_t MODECFG_get_quiet_mode( void )
{
   uint16_t uQuietMode = 0;

   if ( fileDataValid )
   {
      uQuietMode = modeConfigFileData.uQuietMode;
   }

   return( ( uint8_t )uQuietMode );
}

/***********************************************************************************************************************

   Function name: MODECFG_set_quiet_mode

   Purpose: set system quiet mode

   Arguments:  quiet mode setting

   Returns: success/failure

   Re-entrant Code: yes

   Notes:

 **********************************************************************************************************************/

returnStatus_t MODECFG_set_quiet_mode( uint8_t uQuietMode )
{
   returnStatus_t retVal = eFAILURE;

   if ( uQuietMode <= 1 )
   {
      OS_MUTEX_Lock( &modeConfigMutex ); // Function will not return if it fails
      modeConfigFileData.uQuietMode = uQuietMode;
      retVal = FIO_fwrite( &modeConfigFileHandle, 0, ( uint8_t* ) &modeConfigFileData,
                           ( lCnt )sizeof( modeConfigFileData ) );
      OS_MUTEX_Unlock( &modeConfigMutex ); // Function will not return if it fails

      if ( eSUCCESS == retVal )
      {
#if ( EP == 1 )
         LED_checkModeStatus();
#endif
         DBG_logPrintf( 'I', "eFN_MODECFG Write Successful" );
      }
      else
      {
         DBG_logPrintf( 'E', "eFN_MODECFG Write Failed" );
      }
   }
   else
   {
      DBG_logPrintf( 'E', "eFN_MODECFG Write Failed, Value Out of Range" );
   }

   return( retVal );
}
#if ( EP == 1 )
/***********************************************************************************************************************

   Function name: MODECFG_get_rfTest_mode

   Purpose: get system "rfTest mode" setting

   Arguments: none

   Returns: uint8_t system rfTest mode;

   Re-entrant Code: yes

   Notes:

 **********************************************************************************************************************/

uint8_t MODECFG_get_rfTest_mode( void )
{
   uint16_t urfTestMode = 0;

   if ( fileDataValid )
   {
      urfTestMode = modeConfigFileData.urfTestMode;
   }

   return( ( uint8_t )urfTestMode );
}

/***********************************************************************************************************************

   Function name: MODECFG_set_rfTest_mode

   Purpose: set system rfTest mode

   Arguments:  rfTest mode setting

   Returns: success/failure

   Re-entrant Code: yes

   Notes:

 **********************************************************************************************************************/

returnStatus_t MODECFG_set_rfTest_mode( uint8_t urfTestMode )
{
   returnStatus_t retVal = eFAILURE;

   if ( urfTestMode <= 1 )
   {
      OS_MUTEX_Lock( &modeConfigMutex ); // Function will not return if it fails
      modeConfigFileData.urfTestMode = urfTestMode;
      retVal = FIO_fwrite( &modeConfigFileHandle, 0, ( uint8_t* ) &modeConfigFileData,
                           ( lCnt )sizeof( modeConfigFileData ) );
      OS_MUTEX_Unlock( &modeConfigMutex ); // Function will not return if it fails

      if ( eSUCCESS == retVal )
      {
#if ( EP == 1 )
         LED_checkModeStatus();
#endif
         DBG_logPrintf( 'I', "eFN_MODECFG Write Successful" );
      }
      else
      {
         DBG_logPrintf( 'E', "eFN_MODECFG Write Failed" );
      }
   }
   else
   {
      DBG_logPrintf( 'E', "eFN_MODECFG Write Failed, Value Out of Range" );
   }

   return( retVal );
}
#endif

#if ( EP == 1 )
/***********************************************************************************************************************

   Function name: MODECFG_get_decommission_mode

   Purpose: get system "decommission mode" setting

   Arguments: none

   Returns: uint8_t system decomission mode;

   Re-entrant Code: yes

   Notes:

 **********************************************************************************************************************/

uint8_t MODECFG_get_decommission_mode( void )

{
   uint8_t uDecommissionMode = 0;

   if ( fileDataValid )
   {
      uDecommissionMode = modeConfigFileData.uDecommissionMode;
   }

   return( uDecommissionMode );
}


/***********************************************************************************************************************

   Function name: MODECFG_set_decommision_mode

   Purpose: set system decommision mode

   Arguments:  decommision mode setting

   Returns: success/failure

   Re-entrant Code: yes

   Notes:

 **********************************************************************************************************************/

returnStatus_t MODECFG_set_decommission_mode( uint8_t uDecommissionMode )
{
   returnStatus_t retVal;

   OS_MUTEX_Lock( &modeConfigMutex ); // Function will not return if it fails

   modeConfigFileData.uDecommissionMode = uDecommissionMode;
   retVal = FIO_fwrite( &modeConfigFileHandle, 0, ( uint8_t* ) &modeConfigFileData,
                        ( lCnt )sizeof( modeConfigFileData ) );
   OS_MUTEX_Unlock( &modeConfigMutex ); // Function will not return if it fails

   if ( eSUCCESS == retVal )
   {
#if ( EP == 1 )
      VBATREG_METER_SHOP_MODE = modeConfigFileData.uDecommissionMode;
      LED_checkModeStatus();
#if (ACLARA_DA == 1)
      (void)B2BSendDecommission();
#endif
#endif
      DBG_logPrintf( 'I', "eFN_MODECFG Write Successful" );
   }
   else
   {
      DBG_logPrintf( 'E', "eFN_MODECFG Write Failed" );
   }

   return( retVal );
}


/***********************************************************************************************************************

   Function Name: MODECFG_decommissionModeHandler

   Purpose: Get/Set decommissionMode

   Arguments:  action-> set or get
               id    -> HEEP enum associated with the value
               value -> pointer to new value to be placed in file
               attr  -> pointer to structure to return data attributes (only for get action). Ignore if NULL

   Returns: If parameter not handled by this routine, e_APP_NOT_HANDLED. Otherwise return success of get/put.

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/
/*lint -efunc( 715,MODECFG_decommissionModeHandler) symbol id not referenced. */
returnStatus_t MODECFG_decommissionModeHandler( enum_MessageMethod action, meterReadingType id, void *value, OR_PM_Attr_t *attr )
{
   returnStatus_t  retVal = eAPP_NOT_HANDLED; // Success/failure

   if ( method_get == action )   /* Used to "get" the variable. */
   {
      if ( sizeof(modeConfigFileData.uDecommissionMode) <= MAX_OR_PM_PAYLOAD_SIZE ) //lint !e506 !e774
      {  //The reading will fit in the buffer
         *(uint8_t *)value = MODECFG_get_decommission_mode();
         retVal = eSUCCESS;
         if (attr != NULL)
         {
            attr->rValLen = (uint16_t)sizeof(modeConfigFileData.uDecommissionMode);
            attr->rValTypecast = (uint8_t)uintValue;
         }
      }
   }
   else  /* method_put, method_post used to "set" the variable.   */
   {
      retVal = MODECFG_set_decommission_mode(*(uint8_t *)value);
   }
   return retVal;
} //lint !e715 symbol id not referenced. This funtion is only called when id is matched
#endif

/***********************************************************************************************************************

   Function Name: MODECFG_get_initialCFGMode

   Purpose: Get the initial configuration mode of the device on power up

   Arguments:  None

   Returns: the mode configuration during the MODECFG_Init task

   Side Effects: None

   Reentrant Code: Yes

 **********************************************************************************************************************/

modeConfig_e  MODECFG_get_initialCFGMode( void )
{
  static modeConfig_e initialModeConfig = e_MODECFG_INIT;

  if(initialModeConfig == e_MODECFG_INIT)
  {
    //mode exclusivity is not yet fully enforced in EP V1.70
    if( 0 != modeConfigFileData.uQuietMode )
    {
      initialModeConfig = e_MODECFG_QUIET_MODE;

    }
    else if ( 0 !=  modeConfigFileData.uShipMode )
    {
      initialModeConfig = e_MODECFG_SHIP_MODE;

    }
#if ( EP == 1 )
    else if( 0 != modeConfigFileData.urfTestMode )
    {
      initialModeConfig = e_MODECFG_RF_TEST_MODE ;

    }
    else if( 0 != modeConfigFileData.uDecommissionMode )
    {
      initialModeConfig = e_MODECFG_DECOMMISSION_MODE;
    }
#endif
    else
    {
      initialModeConfig = e_MODECFG_NORMAL_MODE;

    }
  }
  return initialModeConfig;


}