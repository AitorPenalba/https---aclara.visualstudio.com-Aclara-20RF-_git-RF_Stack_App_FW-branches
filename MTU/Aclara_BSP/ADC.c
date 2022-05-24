/******************************************************************************
 *
 * Filename: ADC.c
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2012-2022 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include "DBG_SerialDebug.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#include <bsp.h>
#endif

/* #DEFINE DEFINITIONS */
#define USE_LWADC          1
/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */
#if USE_LWADC
#else
static const ADC_INIT_STRUCT Adc_Init_Param =
{
   ADC_RESOLUTION_DEFAULT /* resolution */
};
#endif

/*lint -esym(750,ADC0_NO_CH,ADC_LSTS_12,ADC_LSTS_6,ADC_LSTS_2,RDIV_12VA_MONITOR)  not referenced */
#if USE_LWADC

/* ADC 0 */
#if ( MCU_SELECTED == NXP_K24 )
#define ADC0_NO_CH                 (ADC_SOURCE_MODULE(1) | ADC_SOURCE_MUXSEL_X | ADC_SOURCE_CHANNEL(31))
#define ADC0_PROC_TEMPERATURE_CH    ADC0_SOURCE_AD26 // See section 3.7.1.3.1.1 for ADC0 channel assignement
#define ADC0_SC_VOLTAGE_CH          ADC0_SOURCE_AD19
#define ADC0_4V0_VOLTAGE_CH         ADC0_SOURCE_AD3
#define ADC0_HW_REV_CH              ADC0_SOURCE_AD1
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
#define ADC0_BOARD_TEMPERATURE_CH   ADC0_SOURCE_AD0
#endif
#elif ( MCU_SELECTED == RA6E1 )
#define ADC0_HW_REV_CH              ADC_CHANNEL_5
#define ADC0_SC_VOLTAGE_CH          ADC_CHANNEL_7
#define ADC0_4V0_VOLTAGE_CH         ADC_CHANNEL_8
#endif


/* ADC 1 */
#if ENABLE_ADC1
#define ADC1_NO_CH                  (ADC_SOURCE_MODULE(2) | ADC_SOURCE_MUXSEL_X | ADC_SOURCE_CHANNEL(31))
#define ADC1_PROC_TEMPERATURE_CH    ADC1_SOURCE_AD26
#define ADC1_4V0_VOLTAGE_CH         ADC1_SOURCE_AD0
//      ADC1_SC_VOLTAGE_CH          not available
//      ADC1_HW_REV_CH              not available
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
#define ADC1_BOARD_TEMPERATURE_CH   ADC1_SOURCE_AD3
#endif
#endif

// ADC Long Sample Time settings
#define ADC_LSTS_20                 (0)
#define ADC_LSTS_12                 (1)
#define ADC_LSTS_6                  (2)
#define ADC_LSTS_2                  (3)

#else // Not using LWADC

/* ADC 0 */
#define ADC0_PROC_TEMPERATURE_CH    26
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
#define ADC0_BOARD_TEMPERATURE_CH    0
#endif
#define ADC0_SC_VOLTAGE_CH          19
#define ADC0_4V0_VOLTAGE_CH          3
#define ADC0_HW_REV_CH               1

/* ADC 1 */
#if ENABLE_ADC1
#define ADC1_PROC_TEMPERATURE_CH    26
#define ADC1_4V0_VOLTAGE_CH          0
//      ADC1_SC_VOLTAGE_CH             not available
//      ADC1_HW_REV_CH                 not available
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
#define ADC1_BOARD_TEMPERATURE_CH    3
#endif
#endif

// ADC Long Sample Time settings
#define ADC_LSTS_20                 (20)
#define ADC_LSTS_12                 (12)
#define ADC_LSTS_6                  (6)
#define ADC_LSTS_2                  (2)

#endif

/* The ADC Channel setting which disables the ADC */
#define ADC0_DISABLED_CH            0x1F
#if ENABLE_ADC1
#define ADC1_DISABLED_CH            0x1F
#endif

#define RHIGH                       ((float)2700)
#define RLOW                        ((float)7500)
/*lint -esym(750,RDIV_4V0)   */
#define RDIV_4V0                    (RLOW / (RLOW + RHIGH))
#define UN_RDIV_4V0                 ((RLOW + RHIGH) / RLOW)

/* FILE VARIABLE DEFINITIONS */
static OS_MUTEX_Obj  intAdcMutex_         = {0};      /* Serialize access to the ADC */
static bool          bIntAdcMutexCreated_ = (bool)false;

/* FUNCTION PROTOTYPES */
static bool setup_ADC0 ( void );
static float ADC_Get_Ch_Voltage ( uint32_t adc_source_adx );
#if ( MCU_SELECTED == NXP_K24 )
#if USE_LWADC
static uint32_t adc_calibrate(ADC_MemMapPtr adc_ptr);
#endif   //#if USE_LWADC
#if ENABLE_ADC1
static bool setup_ADC1 ( void );
#endif   //#if ENABLE_ADC1
#endif
#if ( TM_ADC_UNIT_TEST == 1 )
bool ADC_UnitTest(void);
#endif
/* FUNCTION DEFINITIONS */

/*******************************************************************************

  Function name: ADC_init

  Purpose: This function will initialize the variables and data structures required
           for the ADC modules

  Arguments: None

  Returns: InitSuccessful - true if everything was successful, false if something failed

  Notes: This opens the ADC modules and runs a calibration routine on them

*******************************************************************************/
returnStatus_t ADC_init ( void )
{
   returnStatus_t retVal = eSUCCESS; /* Start with pass status, and latch on any failure */

   if ( !bIntAdcMutexCreated_ ) /* If the mutex has not been created, create it. */
   {  /* Install ISRs and create sem & mutex to protect the banked driver modules critical section */
      if (OS_MUTEX_Create(&intAdcMutex_))
      {
         bIntAdcMutexCreated_ = (bool)true;
      }
      else
      {
         retVal = eFAILURE;
      }
   }

   if ( ! setup_ADC0() )
   {
      retVal = eFAILURE;
   }
#if ENABLE_ADC1
   if ( ! setup_ADC1() )
   {
      retVal = eFAILURE;
   }
#endif
   return ( retVal );
} /* end ADC_init () */

/*******************************************************************************

  Function name: setup_ADC0

  Purpose: This function will initialize the variables and data structures required
           for the ADC0

  Arguments:

  Returns: InitSuccessful - true if everything was successful, false if something failed

  Notes: This opens the ADC module (adc0 in this case) and runs a calibration routine on it

*******************************************************************************/
static bool setup_ADC0 ( void )
{
   bool         InitSuccessful = (bool)true; /* Start with pass status, and latch on any failure */
#if ( MCU_SELECTED == NXP_K24 )
#if USE_LWADC
   LWADC_STRUCT   adc = {0}; // Appease lint

   if (ADC_OK != adc_calibrate(_bsp_get_adc_base_address(0)))
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_init_input(&adc, ADC0_PROC_TEMPERATURE_CH)) //Default to the super cap voltage channel
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_set_attribute(&adc, LWADC_DEFAULT_NUMERATOR, 1))
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_set_attribute(&adc, LWADC_DEFAULT_DENOMINATOR, 1))
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_set_attribute(&adc, LWADC_NUMERATOR, 1))
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_set_attribute(&adc, LWADC_DENOMINATOR, 1))
   {
      InitSuccessful = (bool)false;
   }
   if ( InitSuccessful )   //lint -e{644} if InitSuccessful, adc has been initialized
   {
      //Set long sample time to 20 extra clocks
      adc.context_ptr->adc_ptr->CFG1 |= ADC_CFG1_ADLSMP_MASK;
      adc.context_ptr->adc_ptr->CFG2 &= ~ADC_CFG2_ADLSTS_MASK;
      adc.context_ptr->adc_ptr->CFG2 |= ADC_CFG2_ADLSTS(ADC_LSTS_20);
   }
#else
   int32_t      ErrorValue;
   MQX_FILE_PTR Adc0_File;
   int32_t      param;

   do
   {
      /* Setup Analog 0 - for Board Temperature reading */
      Adc0_File = fopen ( "adc0:", (const char*)&Adc_Init_Param );
      if ( Adc0_File == NULL )
      {
         InitSuccessful = (bool)false;
         ErrorValue = ferror ( Adc0_File );
         (void)printf ( "ERROR - ADC fopen adc0: %d\n", ErrorValue );
         break;
      } /* end if() */

      ErrorValue = ioctl ( Adc0_File, ADC_IOCTL_SET_HIGH_SPEED, NULL ); /*lint !e835 !e845*/
      if ( ErrorValue != MQX_OK )
      {
         InitSuccessful = (bool)false;
         (void)printf ( "ERROR - ADC Set High Speed adc0: %d\n", ErrorValue );
         break;
      } /* end if() */

      param = ADC_LSTS_20;
      ErrorValue = ioctl ( Adc0_File, ADC_IOCTL_SET_LONG_SAMPLE, &param ); /*lint !e835 !e845*/
      if ( ErrorValue != MQX_OK )
      {
         InitSuccessful = (bool)false;
         (void)printf ( "ERROR - ADC Set Long Sample time adc0: %d\n", ErrorValue );
         break;
      } /* end if() */

      param = 32;
      ErrorValue = ioctl ( Adc0_File, ADC_IOCTL_SET_HW_AVERAGING, &param ); /*lint !e835 !e845*/
      if ( ErrorValue != MQX_OK )
      {
         InitSuccessful = (bool)false;
         (void)printf ( "ERROR - ADC Set HW Averaging adc0: %d\n", ErrorValue );
         break;
      } /* end if() */

      ErrorValue = ioctl ( Adc0_File, ADC_IOCTL_CALIBRATE, NULL ); /*lint !e835 !e845*/
      if ( ErrorValue != MQX_OK )
      {
         InitSuccessful = (bool)false;
         (void)printf ( "ERROR - ADC Calibrate adc0: %d\n", ErrorValue );
         break;
      } /* end if() */

      /* Disable the ADC interrupts (that occur after every conversion)
         the fopen calls above automatically enable this interrupt that we don't need*/
      ADC0_SC1A &= ~(ADC_SC1_AIEN_MASK);
      /* Disable the PDB triggering for this ADC channel - Since the PDB isn't used to trigger this ADC module */
      PDB0_SC  &= ~(PDB_SC_PDBEN_MASK);   /* This will disable all ADC modules that use the PDB */
      PDB0_CH0C1 = 0;
      PDB0_CH0DLY0 = 0xFFFF;
      PDB0_CH0DLY1 = 0xFFFF;
      /* Change ADC to SW triggered */
      ADC0_SC2 &= ~(ADC_SC2_ADTRG_MASK);
   }while (0);
   if ( ! InitSuccessful)
   {
      fclose(Adc0_File);
   }
#endif
#elif ( MCU_SELECTED == RA6E1 )
   /* Initializes the module. */
   (void)R_ADC_Open( &g_adc0_ctrl, &g_adc0_cfg );  /* Renesas's FSP function returns are removed */
   /* Enable channels. */
   (void)R_ADC_ScanCfg( &g_adc0_ctrl, &g_adc0_channel_cfg );
#endif
   return ( InitSuccessful );
} /* end ADC_Setup_ADC0 () */

#if ENABLE_ADC1
/*******************************************************************************

  Function name: setup_ADC1

  Purpose: This function will initialize the variables and data structures required
           for the ADC1

  Arguments:

  Returns: InitSuccessful - true if everything was successful, false if something failed

  Notes: This opens the ADC module (adc1 in this case) and runs a calibration routine on it

*******************************************************************************/
static bool setup_ADC1 ( void )
{
   bool InitSuccessful = (bool)true; /* Start with pass status, and latch on any failure */
#if USE_LWADC
   LWADC_STRUCT adc = {0}; // Appease lint;

   if (ADC_OK != adc_calibrate(_bsp_get_adc_base_address(1)))
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_init_input(&adc, ADC1_PROC_TEMPERATURE_CH)) //Default to the super cap voltage channel
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_set_attribute(&adc, LWADC_DEFAULT_NUMERATOR, 1))
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_set_attribute(&adc, LWADC_DEFAULT_DENOMINATOR, 1))
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_set_attribute(&adc, LWADC_NUMERATOR, 1))
   {
      InitSuccessful = (bool)false;
   }
   else if(!_lwadc_set_attribute(&adc, LWADC_DENOMINATOR, 1))
   {
      InitSuccessful = (bool)false;
   }
   //Set long sample time to 20 extra clocks
   adc.context_ptr->adc_ptr->CFG1 |= ADC_CFG1_ADLSMP_MASK;
   adc.context_ptr->adc_ptr->CFG2 &= ~ADC_CFG2_ADLSTS_MASK;
   adc.context_ptr->adc_ptr->CFG2 |= ADC_CFG2_ADLSTS(ADC_LSTS_20);
#else
   int32_t      ErrorValue;
   MQX_FILE_PTR Adc1_File;
   int32_t      param;

   do
   {
      /* Setup Analog 1 - for Board Temperature reading */
      Adc1_File = fopen ( "adc1:", (const char*)&Adc_Init_Param );
      if ( Adc1_File == NULL )
      {
         InitSuccessful = (bool)false;
         ErrorValue = ferror ( Adc1_File );
         (void)printf ( "ERROR - ADC fopen adc1: %d\n", ErrorValue );
      } /* end if() */

      ErrorValue = ioctl ( Adc1_File, ADC_IOCTL_SET_HIGH_SPEED, NULL ); /*lint !e835 !e845*/
      if ( ErrorValue != MQX_OK )
      {
         InitSuccessful = (bool)false;
         (void)printf ( "ERROR - ADC Set High Speed adc1: %d\n", ErrorValue );
      } /* end if() */

      param = ADC_LSTS_20;
      ErrorValue = ioctl ( Adc1_File, ADC_IOCTL_SET_LONG_SAMPLE, &param ); /*lint !e835 !e845*/
      if ( ErrorValue != MQX_OK )
      {
         InitSuccessful = (bool)false;
         (void)printf ( "ERROR - ADC Set Long Sample time adc1: %d\n", ErrorValue );
         break;
      } /* end if() */

      param = 32;
      ErrorValue = ioctl ( Adc1_File, ADC_IOCTL_SET_HW_AVERAGING, &param ); /*lint !e835 !e845*/
      if ( ErrorValue != MQX_OK )
      {
         InitSuccessful = (bool)false;
         (void)printf ( "ERROR - ADC Set HW Averaging adc1: %d\n", ErrorValue );
         break;
      } /* end if() */

      ErrorValue = ioctl ( Adc1_File, ADC_IOCTL_CALIBRATE, NULL ); /*lint !e835 !e845*/
      if ( ErrorValue != MQX_OK )
      {
         InitSuccessful = (bool)false;
         (void)printf ( "ERROR - ADC Calibrate adc1: %d\n", ErrorValue );
      } /* end if() */

      /* Disable the ADC interrupts (that occur after every conversion)
         the fopen calls above automatically enable this interrupt that we don't need*/
      ADC1_SC1A &= ~(ADC_SC1_AIEN_MASK);
      /* Disable the PDB triggering for this ADC channel - Since the PDB isn't used to trigger this ADC module */
      PDB0_SC  &= ~(PDB_SC_PDBEN_MASK);   /* This will disable all ADC modules that use the PDB */
      PDB0_CH1C1 = 0;
      PDB0_CH1DLY0 = 0xFFFF;
      PDB0_CH1DLY1 = 0xFFFF;
      /* Change ADC to SW triggered */
      ADC1_SC2 &= ~(ADC_SC2_ADTRG_MASK);
   }while (0);
   if ( ! InitSuccessful)
   {
      fclose(Adc1_File);
   }
#endif   //#if/#else USE_LWADC

   return ( InitSuccessful );
} /* end ADC_Setup_ADC1 () */
#endif   //#if ENABLE_ADC1

#if ( MCU_SELECTED == NXP_K24 )
#if USE_LWADC
/*FUNCTION**********************************************************************
*
* Function Name    : adc_calibrate
* Returned Value   : ADC_OK or ADC_ERROR
* Comments         : Calibrates the ADC module (from MQX  adc_kadc.c)
*
*END***************************************************************************/
static uint32_t adc_calibrate(ADC_MemMapPtr adc_ptr)
{
   uint16_t cal_var;
   struct
   {
      uint8_t  SC2;
      uint8_t  SC3;
   } saved_regs;

   if (adc_ptr == NULL)
   {
      return ADC_ERROR_BAD_PARAM; /* no such ADC peripheral exists */
   }

   saved_regs.SC2 = (uint8_t)adc_ptr->SC2;
   saved_regs.SC3 = (uint8_t)adc_ptr->SC3;

   /* Enable Software Conversion Trigger for Calibration Process */
   adc_ptr->SC2 &= ~ADC_SC2_ADTRG_MASK;

   /* Initiate calibration */
   adc_ptr->SC3 |= ADC_SC3_CAL_MASK;

   /* Wait for conversion to complete */
   while (adc_ptr->SC2 & ADC_SC2_ADACT_MASK)
   {
      ;
   }

   /* Check if calibration failed */
   if (adc_ptr->SC3 & ADC_SC3_CALF_MASK)
   {
      /* Clear calibration failed flag */
      adc_ptr->SC3 |= ADC_SC3_CALF_MASK;

      /* calibration failed */
      return (uint32_t)-1;
   }

   /* Calculate plus-side calibration values according
    * to Calibration function described in reference manual.   */
   /* 1. Initialize (clear) a 16b variable in RAM */
   cal_var  = 0x0000;
   /* 2. Add the following plus-side calibration results CLP0, CLP1,
    *    CLP2, CLP3, CLP4 and CLPS to the variable. */
   cal_var +=  (uint16_t)adc_ptr->CLP0;
   cal_var +=  (uint16_t)adc_ptr->CLP1;
   cal_var +=  (uint16_t)adc_ptr->CLP2;
   cal_var +=  (uint16_t)adc_ptr->CLP3;
   cal_var +=  (uint16_t)adc_ptr->CLP4;
   cal_var +=  (uint16_t)adc_ptr->CLPS;

   /* 3. Divide the variable by two. */
   cal_var = cal_var / 2;

   /* 4. Set the MSB of the variable. */
   cal_var += 0x8000;

   /* 5. Store the value in the plus-side gain calibration registers ADCPGH and ADCPGL. */
   adc_ptr->PG = cal_var;
#if ADC_HAS_DIFFERENTIAL_CHANNELS
   /* Repeat procedure for the minus-side gain calibration value. */
   /* 1. Initialize (clear) a 16b variable in RAM */
   cal_var  = 0x0000;

   /* 2. Add the following minus-side calibration results CLM0, CLM1,
    *    CLM2, CLM3, CLM4 and CLMS to the variable. */
   cal_var += (uint16_t)adc_ptr->CLM0;
   cal_var += (uint16_t)adc_ptr->CLM1;
   cal_var += (uint16_t)adc_ptr->CLM2;
   cal_var += (uint16_t)adc_ptr->CLM3;
   cal_var += (uint16_t)adc_ptr->CLM4;
   cal_var += (uint16_t)adc_ptr->CLMS;

   /* 3. Divide the variable by two. */
   cal_var = cal_var / 2;

   /* 4. Set the MSB of the variable. */
   cal_var += 0x8000;

   /* 5. Store the value in the minus-side gain calibration registers ADCMGH and ADCMGL. */
   adc_ptr->MG = cal_var;
#endif /* ADC_HAS_DIFFERENTIAL_CHANNELS */
   /* Clear CAL bit */
   adc_ptr->SC3 = saved_regs.SC3;
   adc_ptr->SC2 = saved_regs.SC2;

   return ADC_OK;
}
#endif   //#if USE_LWADC
#endif

/*******************************************************************************

  Function name: ADC_ShutDown

  Purpose: This function is used if anyone wants to shutdown the ADC module
           this can be used when the unit is being powered down, etc.

  Arguments:

  Returns: returnStatus_t

  Notes: This function will disable the PDB (which disables the trigger for the
         ADC1, ADC2, ADC3 modules so they won't run anymore
         This function changes the ADC0 trigger from hardware to software, which
         will cause it to stop all future conversions.
         Because all ADC modules won't convert, the DMA is essentially also stopped

*******************************************************************************/
returnStatus_t ADC_ShutDown ( void )
{
#if ( MCU_SELECTED == NXP_K24 )
   //Disables the trigger for ADC channel
   ADC0_SC1A = (ADC0_SC1A & ~ADC_SC1_ADCH_MASK) | ADC_SC1_ADCH(ADC0_DISABLED_CH);
#if ENABLE_ADC1
   ADC1_SC1A = (ADC1_SC1A & ~ADC_SC1_ADCH_MASK) | ADC_SC1_ADCH(ADC1_DISABLED_CH);
#endif   //#if ENABLE_ADC1
#elif ( MCU_SELECTED == RA6E1 )
   //Closes the ADC driver, disables the interrupts and Stops the ADC
   //   (void)R_ADC_Close( &g_adc0_ctrl );  // TODO: RA6E1: Do We need this?
#endif
   return ( eSUCCESS );
} /* end ADC_ShutDown () */

/*******************************************************************************

  Function name: ADC0_Get_Ch_Voltage

  Purpose: This function is used to get the current voltage of the specified channel

  Arguments: adc_source_adx - The ADC channel 'x' to be read

  Returns: The converted result from the ADC capture in volts

  Notes:

*******************************************************************************/
static float ADC_Get_Ch_Voltage ( uint32_t adc_source_adx )
{
#if ( MCU_SELECTED == NXP_K24 )
   uint32_t result;
#elif ( MCU_SELECTED == RA6E1 )
   /* In RA6E1, ADC's value is returned in uint16_t  */
   uint16_t result;
#endif
   float    voltage;

   /* Wait for mutex so multiple tasks can share the ADC */
   OS_MUTEX_Lock(&intAdcMutex_);
#if ( MCU_SELECTED == NXP_K24 )
#if (USE_LWADC == 1)
   LWADC_STRUCT adc;

   (void)_lwadc_init_input(&adc, adc_source_adx);
   (void)_lwadc_read_average(&adc, 1, &result);
   //Turn ADC Continuous conversions off
   adc.context_ptr->adc_ptr->SC3 &= ~ADC_SC3_ADCO_MASK;
#else
   if ( ADC_GET_MODULE( adc_source_adx ) == ADC_SOURCE_MODULE(1) )
   {
      /* Initiate a SW start conversion (need to clear channel bits first) */
      ADC0_SC1A = (ADC0_SC1A & ~ADC_SC1_ADCH_MASK) | ADC_SC1_ADCH(adc_source_adx);
      /* Wait for conversion to complete */
      while ( !(ADC0_SC1A & ADC_SC1_COCO_MASK) )
      {
         ;
      }
      result = ADC0_RA;
   }
   else // ==ADC_SOURCE_MODULE(2)
   {
#warning "ADC1 functionality with LWADC==0 remains untested"
      /* Initiate a SW start conversion (need to clear channel bits first) */
      ADC1_SC1A = (ADC1_SC1A & ~ADC_SC1_ADCH_MASK) | ADC_SC1_ADCH(adc_source_adx);
      /* Wait for conversion to complete */
      while ( !(ADC1_SC1A & ADC_SC1_COCO_MASK) )
      {
         ;
      }
      result = ADC1_RA;
   }
#endif
#elif ( MCU_SELECTED == RA6E1 )
   (void)R_ADC_ScanStart( &g_adc0_ctrl );
   /* Wait for conversion to complete. */
   adc_status_t status;
   status.state = ADC_STATE_SCAN_IN_PROGRESS;
   while ( ADC_STATE_SCAN_IN_PROGRESS == status.state )
   {
       (void)R_ADC_StatusGet( &g_adc0_ctrl, &status );
   }
   /* Read converted data. */
   (void)R_ADC_Read( &g_adc0_ctrl, adc_source_adx, &result );
#endif
   OS_MUTEX_Unlock(&intAdcMutex_);

   /* Convert the raw value to voltage (voltage = (Value / ADC_ValueMax) * Vref) */
#if ( MCU_SELECTED == NXP_K24 )
   voltage = ( ( ( float )result / 65535.0f ) * 3.3f );
#elif ( MCU_SELECTED == RA6E1 )
   /* ADC's resolution is 12bit */
   voltage = ( ( ( float )result / 4095.0f ) * 3.3f );
#endif
   return ( voltage );
}/* end ADC_Get_Ch_Voltage () */

#if ( OVERRIDE_TEMPERATURE == 1 )
/*******************************************************************************

  Function name: ADC_Set_Man_Temperature

  Purpose: This function is used to set the override temperature of the processor

  Arguments: newTemperature - Override temperature in Celsius(0)

  Returns: None

  Notes:

*******************************************************************************/
float ManualTemperature_ = 1000.0;
void ADC_Set_Man_Temperature  (float newTemperature)
{
   ManualTemperature_ = newTemperature;
}

/*******************************************************************************

  Function name: ADC_Get_Man_Temperature

  Purpose: This function is used to get the override temperature of the processor

  Arguments: None

  Returns: Override temperature in Celcius(0)

  Notes:

*******************************************************************************/
float ADC_Get_Man_Temperature  ( void )
{
   return ManualTemperature_;
}
#endif

/*******************************************************************************

  Function name: ADC_Get_uP_Temperature

  Purpose: This function is used to get the current temperature of the processor

  Arguments: bFahrenheit - Boolean to return the temperature in Celcius(0) or Farenheit(1)

  Returns: The converted result from the ADC capture in the desired unit (Celcius or Farenheit)

  Notes:

*******************************************************************************/
float ADC_Get_uP_Temperature  (bool bFahrenheit)
{
#if ( MCU_SELECTED == NXP_K24 )
   float voltage;
   float Temperature;

   voltage = ADC_Get_Ch_Voltage( ADC0_PROC_TEMPERATURE_CH );

   /* The conversion formula came from Freescale Ref. Manual K64P144M120SF5RM, section 35.4.8,
    *   and application note "Temperature Sensor for the HCS08 Microcontroller Family", AN3031
    * Convert voltage to degC: 25 - ((Voltage - Vtemp25) / m)
    *  VTEMP   - voltage of the temperature sensor channel at the ambient temperature (value read from ADC).
    *  VTEMP25 - voltage of the temperature sensor channel at 25 °C.
    *  m       - is referred as temperature sensor slope in the device data sheet. It is the hot or cold
    *            voltage versus temperature slope in V/°C.
    * According to both K644 and K66 datasheets, Vtemp25=0.716 and m=0.00162
    */
   Temperature = 25.0f - ((voltage - 0.716f) / 0.00162f);
#if ( OVERRIDE_TEMPERATURE == 1 )
   if ( ManualTemperature_ < 250.0)
   {
      Temperature = ManualTemperature_;
   }
#endif
   if (bFahrenheit)
   {
      /* Convert from Celcius to Farenheit */
      Temperature = (((Temperature * 9.0f) / 5.0f) + 32.0f);
   }

   return ( Temperature );
#else                     //TODO: RA6E1 for renesas MCU
    float Temperature=0.0;
    return ( Temperature );
#endif
}

//K22 is the only one that supports the Board temp. sensor
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
/*******************************************************************************

  Function name: ADC_Get_Board_Temperature

  Purpose: This function is used to get the current temperature of the board

  Arguments: bFahrenheit - Boolean to return the temperature in Celcius(0) or Farenheit(1)

  Returns: ADC_Temperature - The converted result from the ADC capture in the desired unit

  Notes:

*******************************************************************************/
float ADC_Get_Board_Temperature (bool bFahrenheit)
{
   float voltage;
   float Temperature;

   voltage = ADC_Get_Ch_Voltage( ADC0_BOARD_TEMPERATURE_CH );

   /* The conversion to temperature came from the MCP9700_-E/LT temp sensor documentation */
   /* Convert voltage to degC (Voltage - Vnom at 0C)/(V per degC) */
   Temperature = (voltage - 0.5f) / 0.01f;

   if (bFahrenheit)
   {
      /* Convert from Celcius to Farenheit */
      Temperature = (((Temperature * 9.0f) / 5.0f) + 32.0f);
   }

   return ( Temperature );
}
#endif

/*******************************************************************************

  Function name: ADC_Get_SC_Voltage

  Purpose: This function is used to get the current Supercap Voltage

  Arguments:

  Returns: ADC_Voltage - The converted result from the ADC capture in volts

  Notes:

*******************************************************************************/
float ADC_Get_SC_Voltage ( void )
{
   return ( ADC_Get_Ch_Voltage( ADC0_SC_VOLTAGE_CH ) );
}/* end ADC_Get_SC_Voltage () */

/*******************************************************************************

  Function name: ADC_Get_4V0_Voltage

  Purpose: This function is used to get the current Supercap Voltage

  Arguments:

  Returns: ADC_Voltage - The converted result from the ADC capture in volts

  Notes:

*******************************************************************************/
float ADC_Get_4V0_Voltage ( void )
{
   float voltage;
#if ( ( USE_LWADC == 1 ) || ( ENABLE_ADC1 == 0 ) || ( MCU_SELECTED == RA6E1 ) )
   voltage = ADC_Get_Ch_Voltage( ADC0_4V0_VOLTAGE_CH ) ;
#else // LWDAC==0 && USE_ADC1==1
   voltage =  ADC_Get_Ch_Voltage( ADC1_4V0_VOLTAGE_CH ) ;
#endif
   /* Adjust for circuit voltage divider */
   voltage *= UN_RDIV_4V0;

   return ( voltage );
}/* end ADC_Get_4V0_Voltage () */

/*******************************************************************************

  Function name: ADC_GetVswr

  Purpose: Gets the VSWR for the antenna

  Arguments:

  Returns: VSWR ratio computed for the antenna.

  Notes:

*******************************************************************************/
#if ( VSWR_MEASUREMENT == 1 )
float ADC_Get_VSWR( void )
{
   uint32_t forwardResult, reflectedResult;

   /* Wait for mutex so multiple tasks can share the ADC */
   OS_MUTEX_Lock(&intAdcMutex_);

   LWADC_STRUCT adc;

   (void)_lwadc_init_input(&adc, ADC1_SOURCE_AD1);
   (void)_lwadc_read_average(&adc, 1, &forwardResult);

   (void)_lwadc_init_input(&adc, ADC1_SOURCE_AD20);
   (void)_lwadc_read_average(&adc, 1, &reflectedResult);

   // Turn ADC Continuous conversions off
   adc.context_ptr->adc_ptr->SC3 &= ~ADC_SC3_ADCO_MASK;

   OS_MUTEX_Unlock(&intAdcMutex_);

   // CHECK IF REFLECTED > FORWARD -- THIS CAN HAPPEN DUE TO SLIGHTLY DIFFERENT MEASURE TIMES
   // Indicative of an open antenna... This must be corrected in order for the VSWR equation to function properly.
   if (reflectedResult >= forwardResult)
   {
      reflectedResult = forwardResult - 1;
   }

   // VSWR = (vForward + vReflect) / (vForward - vReflect)
   return (forwardResult + reflectedResult) / (float)(forwardResult - reflectedResult);
}
#endif

/*******************************************************************************

  Function name: ADC_GetHWRevVoltage

  Purpose: Wrapper for ADC_Get_Ch_Voltage to get the Hardware Revision Voltage

  Returns: The result from the ADC capture in volts

  Notes:

*******************************************************************************/
float ADC_GetHWRevVoltage ( void )
{
#if ( ( USE_LWADC == 1 ) || ( ENABLE_ADC1 == 0 ) || ( MCU_SELECTED == RA6E1 ) )
   return ( ADC_Get_Ch_Voltage( ADC0_HW_REV_CH ) );
#else // LWDAC==0 && USE_ADC1==1
   return ( ADC_Get_Ch_Voltage( ADC1_HW_REV_CH ) );
#endif
}/* end ADC_GetHWRevVoltage () */

/*******************************************************************************

  Function name: ADC_GetHWRevLetter

  Purpose: This function is used to get the hardware revision letter

  Arguments:

  Returns: Letter representing the hardware revision

  Notes:

*******************************************************************************/
uint8_t ADC_GetHWRevLetter ( void )
{
   float    ADC_Voltage;
   uint8_t  rtnVal;

   ADC_Voltage = ADC_GetHWRevVoltage();
   rtnVal = 'A';  // (Default) Rev A as a floating input usually returns a value in the noise.

   if ((ADC_Voltage > 0.28) && (ADC_Voltage < 0.32))
   {
      // Rev B returns 0.3 volts
      rtnVal = 'B';
   }
   else if ((ADC_Voltage > 0.58) && (ADC_Voltage < 0.62))
   {
      // Rev C returns 0.6 volts
      rtnVal = 'C';
   }

   return ( rtnVal );
} /* end ADC_GetHWRevLetter () */

#if ( TM_ADC_UNIT_TEST == 1 )
float NonZeroValue = 0;
/*******************************************************************************

   Function name: ADC_UnitTest

   Purpose: This function will test all 3 ADC channel

   Arguments: None

   Returns: bool - 0 if everything was successful, 1 if something failed

*******************************************************************************/
bool ADC_UnitTest(void){
   NonZeroValue = ADC_GetHWRevVoltage();
   if(NonZeroValue == 0)
   {
     return 1;
   }
   NonZeroValue = ADC_Get_SC_Voltage();
   if(NonZeroValue == 0)
   {
     return 1;
   }
   NonZeroValue = ADC_Get_4V0_Voltage(); /*Requires External 4V Power Supply*/
   if(NonZeroValue == 0)
   {
     return 1;
   }
    return 0;
}
#endif