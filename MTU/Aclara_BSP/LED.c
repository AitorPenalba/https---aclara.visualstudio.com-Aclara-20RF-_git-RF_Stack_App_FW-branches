/**********************************************************************************************************************
 *
 * Filename: LED.c
 *
 * Global Designator: LED_
 *
 * Contents:
 *
 **********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2017-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
 * be used or copied only in accordance with the terms of such license.
 *********************************************************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#endif
#include "buffer.h"
#include "MAC.h"
#include "DBG_SerialDebug.h"
#include "mode_config.h"
#if ( EP == 1 )
#include "timer_util.h"
#include "time_sys.h"
#include "version.h"
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
#include "hmc_display.h"
#include "APP_MSG_Handler.h"
#endif
#endif

/* #DEFINE DEFINITIONS */
#define BLUE_LED_TIMEOUT 300           /* seconds until LED power up routine ends */
#define RED_LED_TIMEOUT 30             /* seconds until LED power up routine ends */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */
#if ( RTOS_SELECTION == MQX_RTOS )
typedef struct led_isr_struct_led
{
   void *         OLD_ISR_DATA;
   void           (_CODE_PTR_ OLD_ISR)(void *);
   unsigned long  TICK_COUNT;
} MY_ISR_STRUCT_LED, *MY_ISR_STRUCT_LED_PTR;
#endif

typedef enum
{
   BLINK_SLOW = ((uint8_t)0),
   BLINK_FAST,
   STEADY_ON,
   STEADY_OFF,
   MANUAL
}enum_LedControl_t;

#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
typedef enum
{
   QUIET_MODE = ((uint8_t)0),
   SHIP_MODE,
   SHOP_MODE,
   SECURE_MODE,
   RUN_MODE,
   HOT
}enum_LcdMode_t;
#endif

#if ( EP == 1 )
typedef union
{
   struct
   {
      bool     selftestFail  : 1;
      bool     radioDisabled : 1;
      uint8_t  rsvd          : 6;
   }bits;
   uint8_t flags;
}DiagnosticsType_t;
#endif

/* CONSTANTS */


/* FILE VARIABLE DEFINITIONS */

#if ( EP == 1 )
static uint16_t LedTimerId;
#if ( MCU_SELECTED == NXP_K24 )
static enum_LedControl_t redLedControl = BLINK_SLOW; // Assign initial state of red LED
static enum_LedControl_t blueLedControl = BLINK_SLOW; // Assign initial state of blue LED
static enum_LedControl_t greenLedControl = STEADY_OFF; // Assign initial state of green LED
#endif
static bool diagnosticLedTimeout = (bool)false; // Identifies when 30 seconds LED timeout has occurred
static DiagnosticsType_t diagnosticStatus;

#if ( MCU_SELECTED == NXP_K24 )
static const uint32_t led_pins[] =
{
   LED0_PIN ,
   LED1_PIN ,
   LED2_PIN ,
};
#endif
#endif

/* FUNCTION PROTOTYPES */
static returnStatus_t createLedTimer(void);
static void ledVisual_CB(uint8_t, void *);
#if ( RTOS_SELECTION == MQX_RTOS )
static void LED_vApplicationTickHook( void *user_isr_ptr );
#endif

/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: LED_init

  Purpose: This function initializes the data structures and drivers needed to
           run the LED's on the board

  Arguments:

  Returns: InitSuccessful - true if everything was successful, false if something failed

  Notes: By setting the port pin High, we should be turning the LED Off for an init state

*******************************************************************************/
returnStatus_t LED_init ( void )
{
  returnStatus_t retVal = eSUCCESS;
#if ( MCU_SELECTED == NXP_K24 )
#if ( TRACE_MODE == 0 )
   uint8_t  hwVerString[VER_HW_STR_LEN];
   ( void )VER_getHardwareVersion ( &hwVerString[0], sizeof(hwVerString) );

   if ( 'C' == hwVerString[0] )
   {
      /* If the hardware revision is C, disable the three LED pins and it's
       * application functionality will be replaced by the LCD module */
      GRN_LED_PIN_DISABLE();
      RED_LED_PIN_DISABLE();
      BLU_LED_PIN_DISABLE();
   }
   else
   {
      GRN_LED_PIN_TRIS();
      RED_LED_PIN_TRIS();
      BLU_LED_PIN_TRIS();
   }
#else
#if 0
   MCM_ETBCC   |= MCM_ETBCC_ETDIS_MASK | MCM_ETBCC_ITDIS_MASK;
   /* from Kxx.mac */
   /* Set up Trace clock and data lines   */
   SIM_SCGC5   |= (1<<13); // Enable internal clock to Port E
   SIM_SOPT2   |= (1<<12); // Debug trace clock select = Core/system clock
   ETF_FCR     |= 3;       // Enable trace funnel ports 0 and 1
#endif
   PORTE_PCR0   = 0x540;   // Enable trace clock with DSE
   PORTE_PCR1   = 0x540;   // Enable trace d3 with DSE
   PORTE_PCR2   = 0x540;   // Enable trace d2 with DSE
   PORTE_PCR3   = 0x540;   // Enable trace d1 with DSE
   PORTE_PCR4   = 0x540;   // Enable trace d0 with DSE
#endif // #if ( TRACE_MODE == 0 )
#endif // #if ( MCU_SELECTED == NXP_K24 )

   if ( 0 == MODECFG_get_quiet_mode() )
   {  // Let LED timer execute only if not in quiet mode (timer cannot be disabled in quiet mode see ledVisual_CB())
      /* The following code was taken from MQX example code for adding our timer code to the RTOS tick interrupt.  So, the */
      /* following code will replace the RTOS tick interrupt with our own and then our ISR will call the RTOS tick. */

#if ( RTOS_SELECTION == MQX_RTOS )
      MY_ISR_STRUCT_LED_PTR isr_ptr;
      isr_ptr = (MY_ISR_STRUCT_LED_PTR)_mem_alloc_zero(sizeof(MY_ISR_STRUCT_LED));
      isr_ptr->TICK_COUNT   = 0;
      isr_ptr->OLD_ISR_DATA = _int_get_isr_data(INT_SysTick);             /*lint !e641 */
      isr_ptr->OLD_ISR      = _int_get_isr(INT_SysTick);                  /*lint !e641 */
      _int_install_isr(INT_SysTick, LED_vApplicationTickHook, isr_ptr);   /*lint !e641 !e64 !e534 */
#elif( RTOS_SELECTION == FREE_RTOS )
      /* No need to install ISR */
#endif
      diagnosticStatus.flags = 0; // clear the diagnostic flags
#if ( TEST_TDMA == 0 )
      retVal = createLedTimer();
#endif
   }
   LED_checkModeStatus();

   return ( retVal );
}

/*******************************************************************************

  Function name: LED_on

  Purpose: This function is used to turn the Led on.

  Arguments: LedId - Id of the LED that is to be modified

  Returns:

  Notes:

*******************************************************************************/
void LED_on     ( enum_LED_ID LedId )
{
#if ( MCU_SELECTED == NXP_K24 )
#if ( TRACE_MODE == 0 )
   GPIOE_PSOR = led_pins[LedId];   /* LED On   */
#endif
#endif
}

/*******************************************************************************

  Function name: LED_off

  Purpose: This function is used to turn the Led off.

  Arguments: LedId - Id of the LED that is to be modified

  Returns:

  Notes:

*******************************************************************************/
void LED_off    ( enum_LED_ID LedId )
{
#if ( MCU_SELECTED == NXP_K24 )
#if ( TRACE_MODE == 0 )
   GPIOE_PCOR = led_pins[LedId];   /* LED Off   */
#endif
#endif
}

/*******************************************************************************

  Function name: LED_toggle

  Purpose: This function is used to toggle the Led

  Arguments: LedId - Id of the LED that is to be modified

  Returns:

  Notes:

*******************************************************************************/
void LED_toggle ( enum_LED_ID LedId )
{
#if ( MCU_SELECTED == NXP_K24 )
#if ( TRACE_MODE == 0 )
  GPIOE_PTOR = led_pins[LedId];   /* toggle   */
#endif
#endif
}

#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
/*!
 **************************************************************************************************************
**********************************************
 *
 *  \fn           LED_lcdModeDisplay
 *
 *  \brief        Update the mode or hot status to LCD display.
 *
 *  \param        enum_LcdMode_t
 *
 *  \return       None
 *
 *  \details      This function will update the LCD with the mode or hot status.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
****************************************************************************************************************
*********************************************/
static void LED_lcdModeDisplay( enum_LcdMode_t cmd )
{
   switch ( cmd )
   {
      case (uint8_t)QUIET_MODE:
      {
         HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_MODE_QUIET, HMC_DISP_POS_NIC_MODE);
         break;
      }
      case (uint8_t)SHIP_MODE:
      {
         HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_MODE_SHIP, HMC_DISP_POS_NIC_MODE);
         break;
      }
      case (uint8_t)SHOP_MODE:
      {
         HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_MODE_SHOP, HMC_DISP_POS_NIC_MODE);
         break;
      }
      case (uint8_t)SECURE_MODE:
      {
         HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_MODE_SECURE, HMC_DISP_POS_NIC_MODE);
         break;
      }
      case (uint8_t)RUN_MODE:
      {
         HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_MODE_RUN, HMC_DISP_POS_NIC_MODE);
         break;
      }
      case (uint8_t)HOT:
      {
         HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_HOT, HMC_DISP_POS_NIC_MODE);
         break;
      }
      default:
      {
         /* Do Nothing */
         break;
      }
   }
}
#endif

#if ( EP == 1 )
/*!
 **************************************************************************************************************
**********************************************
 *
 *  \fn           LED_checkModeStatus
 *
 *  \brief        This function when called updates the status of the Mode LED, based upon the current mode
 *                status of the Endpoint.
 *
 *  \param        None
 *
 *  \return       None
 *
 *  \details      This function will update the status of the Mode LED of the endpoint.  It first will check
 *                for quietmode, then shipmode, and then finally meter shopmode.  If none of the mode's are
 *                set, it sets the endpoint to normal operation mode.  This function can be used whenever a
 *                check of the current mode is required.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
****************************************************************************************************************
*********************************************/
void LED_checkModeStatus()
{
   /* Update Mode LED based upon current mode status */
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
   enum_LcdMode_t cmd;
   uint8_t  securityMode = 0;

   ( void )APP_MSG_SecurityHandler( method_get, appSecurityAuthMode, &securityMode, NULL );
#endif

   if ( MODECFG_get_quiet_mode() ) // if in quiet mode, set LED accordingly
   {
      LED_setGreenLedStatus(QUIETMODE);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
      cmd = QUIET_MODE;
#endif
   }
   else if ( MODECFG_get_ship_mode() ) // if in ship mode, set LED accordingly
   {
      LED_setGreenLedStatus(SHIPMODE);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
      cmd = SHIP_MODE;
#endif
   }
   else if ( MODECFG_get_decommission_mode() ) // if in decommission mode, set LED accordingly
   {
      LED_setGreenLedStatus(METERSHOPMODE);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
      cmd = SHOP_MODE;
#endif
   }
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
   else if ( securityMode )   // if in Secure mode
   {
      cmd = SECURE_MODE;
   }
#endif
   else // LED will be in normal operation
   {
      LED_setGreenLedStatus(NORMALMODE);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
      cmd = RUN_MODE;
#endif
   }

#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
   if ( MAC_IsTxPaused() ) //Check for Radio Disabled Condition
   {
      cmd = HOT;
   }
   /* Update the LCD based on the mode or hot condition */
   LED_lcdModeDisplay(cmd);
#endif
}

/*!
 **************************************************************************************************************
**********************************************
 *
 *  \fn           createLedTimer
 *
 *  \brief        This function creates a timer and assigns a callback function.
 *
 *  \param        None
 *
 *  \return       returnStatus_t
 *
 *  \details      This function creates a timer and assigns a call back function to be called every
 *                1000ms.  If unable to create timer, a message will be sent to the debug port.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
****************************************************************************************************************
*********************************************/
static returnStatus_t createLedTimer(void)
{
   timer_t tmrSettings;
   returnStatus_t retStatus;

   (void)memset(&tmrSettings, 0, sizeof(tmrSettings));
   tmrSettings.ulDuration_mS  = 1000; // call LED callback function every second
   tmrSettings.pFunctCallBack = ledVisual_CB;
   retStatus = TMR_AddTimer(&tmrSettings);
   if (eSUCCESS == retStatus)
   {
      LedTimerId = tmrSettings.usiTimerId;
   }
   return retStatus;
}

/*!
 **************************************************************************************************************
**********************************************
 *
 *  \fn           LED_setBlueLedStatus
 *
 *  \brief        Update the current status of the LED and change LED state pending current LED status.
 *
 *  \param        enum_RedLedStatus_t
 *
 *  \return       None
 *
 *  \details      Update the current status of the LED and change LED state pending current LED status.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
****************************************************************************************************************
*********************************************/
void LED_setBlueLedStatus(enum_BlueLedStatus_t status)
{
#if ( MCU_SELECTED == NXP_K24 )
   if (blueLedControl != MANUAL)
   {
      switch (status )
      {
         case TIME_IDLE : // Five minute timeout has occurred, turn LED off
         {
            blueLedControl = STEADY_OFF;
            LED_off(BLU_LED);
            break;
         }
         case VALID_TIME : // endpoint has acquire valid time, change LED state
         {
            blueLedControl = STEADY_ON;
            LED_on(BLU_LED);
            break;
         }
         case NO_VALID_TIME : // endpoint does not have a valid time, change LED to blinking state
         {
            blueLedControl = BLINK_SLOW;
            break;
         }
         case MANUAL_BLU : // change blue LED to manual operation
         {
            blueLedControl = MANUAL;
            LED_off(BLU_LED);
            break;
         }
         default :
         {
            blueLedControl = STEADY_OFF;
            LED_off(BLU_LED);
            break;
         }
      }
   }
#endif
}


/*!
 **************************************************************************************************************
**********************************************
 *
 *  \fn           LED_setGreenLedStatus
 *
 *  \brief        Update the current status of the LED and change LED state pending current LED status.
 *
 *  \param        enum_RedLedStatus_t
 *
 *  \return       None
 *
 *  \details      Update the current status of the LED and change LED state pending current LED status.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
****************************************************************************************************************
*********************************************/
void LED_setGreenLedStatus(enum_GreenLedStatus_t status)
{
#if ( MCU_SELECTED == NXP_K24 )
   if (greenLedControl != MANUAL)
   {
      switch ( status )
      {
         case NORMALMODE : // set green LED to normal operation mode
         {
            greenLedControl = STEADY_OFF;
            LED_off(GRN_LED);
            break;
         }
         case QUIETMODE : // set green LED to quiet mode
         {
            greenLedControl = STEADY_ON;
            LED_on(GRN_LED);
            break;
         }
         case SHIPMODE : // set green LED to ship mode
         {
            greenLedControl = BLINK_SLOW;
            break;
         }
         case METERSHOPMODE : // set green LED to decommission mode
         {
            greenLedControl = BLINK_FAST;
            break;
         }
         case MANUAL_GRN : // place green LED into manual operation condition
         {
            greenLedControl = MANUAL;
            LED_off(GRN_LED);
            break;
         }
         default :
         {
            greenLedControl = STEADY_OFF;
            LED_off(GRN_LED);
            break;
         }
      }
   }
#endif
}


/*!
 **************************************************************************************************************
**********************************************
 *
 *  \fn           LED_setRedLedStatus
 *
 *  \brief        Update the current status of the LED and change LED state pending current LED status.
 *
 *  \param        enum_RedLedStatus_t
 *
 *  \return       None
 *
 *  \details      Update the current status of the LED and change LED state pending current LED status.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
****************************************************************************************************************
*********************************************/
void LED_setRedLedStatus(enum_RedLedStatus_t status)
{
#if ( MCU_SELECTED == NXP_K24 )
#if ( TEST_TDMA == 0 )
   if (redLedControl != MANUAL)
   {
      switch ( status )
      {
         case SELFTEST_RUNNING : // Self-Test has started, blink LED at fast rate
         {
            redLedControl = BLINK_FAST;
            break;
         }
         case SELFTEST_FAIL : // Self-Test Failed, turn LED ON
         {
            diagnosticStatus.bits.selftestFail = (bool)true;
            redLedControl = STEADY_ON;
            LED_on(RED_LED);
            break;
         }
         case SELFTEST_PASS :
         {
            diagnosticStatus.bits.selftestFail = (bool)false; // clear the self-test fail bit
            if ( diagnosticStatus.flags == 0 ) // if no other diagnostics are flagged update LED
            {
              if( diagnosticLedTimeout ) // if after 30 seconds since power up, turn OFF red LED
              {
               redLedControl = STEADY_OFF;
               LED_off(RED_LED);
              }
               else // else blink red LED
               {
                  redLedControl = BLINK_SLOW;
               }
            }
            break;
         }
         case RADIO_DISABLED : // radio temperature outside operating range (upper extreme)
         {
            diagnosticStatus.bits.radioDisabled = (bool)true;
            redLedControl = STEADY_ON;
            LED_on(RED_LED);
            break;
         }
         case RADIO_ENABLED : // radio temperature in operating range (upper extreme)
         {
            diagnosticStatus.bits.radioDisabled = (bool)false;
            if ( diagnosticLedTimeout && ( diagnosticStatus.flags == 0) ) // if after 30 seconds since power up and no diagnostic flags
            {
               redLedControl = STEADY_OFF;
               LED_off(RED_LED);

            }
            break;
         }
         case MICRO_RUNNING : // Current state at power up
         {
            redLedControl = BLINK_SLOW;
            break;
         }
         case MANUAL_RED : // Set LED to manual control.  This is done via debug port.
         {
            redLedControl = MANUAL;
            LED_off(RED_LED);
            break;
         }
         case DIAGNOSTIC_IDLE : // 30 seconds after power up, red LED will turn OFF
         default :
         {
            redLedControl = STEADY_OFF;
            LED_off(RED_LED);
            break;
         }
      }
   }
#endif
#endif
}


/*!
 *************************************************************************************************************
**********************************************
 *
 *  \fn           ledVisual_CB
 *
 *  \brief        Updates current status of LED's based upon events occurring within a period
 *                of time as required in product specific PDS.
 *
 *  \param        uint8_t, void *
 *
 *  \return       None
 *
 *  \details      This function will update the status of the EUT's LEDs by updating status
 *                enumerations that are part of the LED module.  Valid time in the EUT will be
 *                polled as well as monitoring timeout values of specific LED's.  After five minutes,
 *                the function will create a task to delete the timer since the timer is no longer
 *                needed for LED functionality.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
***************************************************************************************************************
*********************************************/
/*lint -esym(818,pData)*/
/*lint -esym(715,cmd,pData)*/
static void ledVisual_CB(uint8_t cmd, void *pData)
{
   static buffer_t buf;
   static uint32_t time = 0;  /* track time passed in function */

   ++time; /* increment time when we enter function */


   /* If the diagnostic LED timeout has not occurred, check on time passed so far  */
   if ( !diagnosticLedTimeout )
   {
      /* check to see if we crossed timeout threshold and no diagnostic errors have occurred */
      if ( time >= RED_LED_TIMEOUT ) // If we crossed threshold, set timeout to true
      {
        diagnosticLedTimeout = true;
        if( diagnosticStatus.flags == 0 ) // if we are presently not in a diagnostic condition, set red LED to idle condition
        {
            LED_setRedLedStatus(DIAGNOSTIC_IDLE);
        }
      }
   }


   /* If we crossed the valid time LED timeout threshold, change status of LED
      to IDLE and delete timer since it is no longer needed */
   if (time > BLUE_LED_TIMEOUT)
   {
      /* Delete timer and set LED status to Idle */
      LED_setBlueLedStatus(TIME_IDLE);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
      HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_NULL, HMC_DISP_POS_NETWORK_STATUS);
#endif
      /***
      * Use a static buffer instead of allocating one because only need to inform MAC and
      * cannot delete timer here since this is a callback function from the timer task.
      * Cannot do this in quiet mode since MAC task is not running.
      ***/
      BM_AllocStatic(&buf, eSYSFMT_VISUAL_INDICATION);
      MAC_Request(&buf);
   }
   /* If we have not crossed timeout threshold, update LED's based upon valid time */
   else
   {
      /* Check if the EUT has a valid time */
      if ( TIME_SYS_IsTimeValid() )
      {
         LED_setBlueLedStatus(VALID_TIME);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
         HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_VALID_TIME, HMC_DISP_POS_NETWORK_STATUS);
#endif
      }
      else
      {
         LED_setBlueLedStatus(NO_VALID_TIME);
#if ( END_DEVICE_PROGRAMMING_DISPLAY == 1 )
         HMC_DISP_UpdateDisplayBuffer(HMC_DISP_MSG_INVALID_TIME, HMC_DISP_POS_NETWORK_STATUS);
#endif
      }
   }

}
/*lint +esym(818,pData)*/
/*lint +esym(715,cmd,pData)*/

#if ( RTOS_SELECTION == MQX_RTOS ) && ( MCU_SELECTED == NXP_K24 )
/*!
 *************************************************************************************************************
**********************************************
 *
 *  \fn           LED_vApplicationTickHook
 *
 *  \brief        This routine controls the behavior of the LED's for blinking conditions.
 *
 *  \param        void *user_isr_ptr
 *
 *  \return       None
 *
 *  \details      This routine controls the behavior of the LED's for blinking conditions.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
***************************************************************************************************************
*********************************************/
STATIC void LED_vApplicationTickHook( void *user_isr_ptr )
{

   MY_ISR_STRUCT_LED_PTR  isr_ptr;   /* */

   /* This code is taken from the MQX example isr.c code to use the system tick to tick our own module. */
   isr_ptr = (MY_ISR_STRUCT_LED_PTR)user_isr_ptr;
   isr_ptr->TICK_COUNT++;
#if ( TEST_TDMA == 0 )
   /* Update the blue LED if needed */
   if (blueLedControl == BLINK_SLOW && isr_ptr->TICK_COUNT % 100 == 0 ) // each second
   {
      LED_toggle(BLU_LED);
   }

   /*  Update the red LED if needed */
   if ( redLedControl == BLINK_SLOW && isr_ptr->TICK_COUNT % 100 == 0 ) // each second
   {
      LED_toggle(RED_LED);
   }
   else if ( redLedControl == BLINK_FAST && isr_ptr->TICK_COUNT % 25 == 0 ) //at quarter second
   {
      LED_toggle(RED_LED);
   }

   /* Update the green LED if needed */
   if (greenLedControl == BLINK_SLOW && isr_ptr->TICK_COUNT % 100 == 0) // each second
   {
      LED_toggle(GRN_LED);
   }
   else if (greenLedControl == BLINK_FAST && isr_ptr->TICK_COUNT % 25 == 0 ) //at quarter second
   {
      LED_toggle(GRN_LED);
   }
#endif
   (*isr_ptr->OLD_ISR)(isr_ptr->OLD_ISR_DATA);
}
#endif


/*!
 *************************************************************************************************************
**********************************************
 *
 *  \fn           LED_getLedTimerId
 *
 *  \brief        Returns the ID number of the timer associated with the timeout threshold of the LED's.
 *
 *  \param        None
 *
 *  \return       uint16_t
 *
 *  \details      Returns the ID number of the timer associated with the timeout threshold of the LED's.
 *                This function can be used elsewhere in the program to acquire the timer ID to perform
 *                needed actions with this ID such as timer deletion when it is no longer needed.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
***************************************************************************************************************
*********************************************/
uint16_t LED_getLedTimerId()
{
   return LedTimerId;
}

/*!
 *************************************************************************************************************
**********************************************
 *
 *  \fn           LED_enabledManualControl
 *
 *  \brief        Updates a static variable in the LED module to indicate manual LED control is wanted.
 *
 *  \param        None
 *
 *  \return       None
 *
 *  \details      This function will set a static variable used by the LED module to indicate that the LED
 *                should be controlled manually via the debug port rather than by the firmware.  Currently,
 *                rebooting the endpoint will reset LED back to firmware control.
 *
 *  \sideeffect   None
 *
 *  \reentrant    No
***************************************************************************************************************
*********************************************/
void LED_enableManualControl()
{
   LED_setBlueLedStatus(MANUAL_BLU);
   LED_setRedLedStatus(MANUAL_RED);
   LED_setGreenLedStatus(MANUAL_GRN);
}

#if 0
/*************************************************************************
 This function may be used in the future to add capability to return LED's
 control back to firmware.  Currently, firmware will take over control at
 reboot and power up.  Manual control is initiated once the LED command
 is utilized through the Debug Command Line.
*************************************************************************/
void LED_disableManualControl()
{
   LED_setRedLedStatus(DIAGNOSTIC_IDLE);
   LED_setGreenLedStatus(NORMAL);
   LED_setBlueLedStatus(TIME_IDLE);
}
#endif

#endif // end of if (EP == 1)
/* end LED_Set () */