/** ****************************************************************************
@file host_reset.c

Watches the host and will reset the board if it appears to be off in the woods.

A product of Aclara Technologies LLC
Confidential and Proprietary
Copyright 2018 Aclara.  All Rights Reserved.
*******************************************************************************/

/*******************************************************************************
#INCLUDES
*******************************************************************************/

#include <stdint.h>

#include "project.h" // required for a lot of random headers so be safe and include 1st
#include "b2bmessage.h"
#include "DBG_SerialDebug.h"

#include "host_reset.h"

/*******************************************************************************
Local #defines (Object-like macros)
*******************************************************************************/

/** Number of miliseconds between attempts to check on host */
#define HOST_CHECK_PERIOD (1000 * 120)

/** Max number of missed responses before performing hard reset */
#define HARD_RESET_LIMIT 5

/** Max number of missed responses before performing soft reset */
#define SOFT_RESET_LIMIT 4

#define HARD_RESET_TIMING 7000
#define SOFT_RESET_TIMING 3000

/*******************************************************************************
Local #defines (Function-like macros)
*******************************************************************************/

/*******************************************************************************
Local Struct, Typedef, and Enum Definitions (Private to this file)
*******************************************************************************/

/*******************************************************************************
Static Function Prototypes
*******************************************************************************/

/*******************************************************************************
Global Variable Definitions
*******************************************************************************/

/*******************************************************************************
Static Variable Definitions
*******************************************************************************/

static uint8_t missedRespCt = 0;

/*******************************************************************************
Function Definitions
*******************************************************************************/

/**
Queries the host to see if it is alive and attempts to reset the board if there
is no response.

@param arg0 Not used, but required here by MQX because this is a task
*/
void HostResetTask(uint32_t arg0)
{
   (void)arg0;

   while (true) /*lint !e716 */
   {
      OS_TASK_Sleep(HOST_CHECK_PERIOD);

      if (B2BSendEchoReq())
      {
         missedRespCt = 0;
      }
      else
      {
         missedRespCt++;

         if (missedRespCt >= HARD_RESET_LIMIT)
         {
            ERR_printf("No response from host.  Performing hard reset.");

            DA_HOLD_HOST_RESET();
            OS_TASK_Sleep(HARD_RESET_TIMING);
            DA_RELEASE_HOST_RESET();

            missedRespCt = 0;
         }
         else if (missedRespCt >= SOFT_RESET_LIMIT)
         {
            ERR_printf("No response from host.  Performing soft reset.");

            DA_HOLD_SOM_OFF();
            OS_TASK_Sleep(SOFT_RESET_TIMING);
            DA_RELEASE_SOM_OFF();
         }
      }
   }
}
