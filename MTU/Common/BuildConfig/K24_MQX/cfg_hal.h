/***********************************************************************************************************************
 *
 * Filename: cfg_hal.h
 *
 * Contents: Hardware Abstraction Layer
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2011-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ KAD Created May 6, 2011
 *
 **********************************************************************************************************************/
#ifndef CFG_HAL_H
#define CFG_HAL_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
/* include common hal definitions */
#include "cfg_hal_defs.h"

/* include project specific HAL definitions */
#include "hal.h"

// check for valid target hardware choice from hardware specific HAL
#if ( ( HAL_TARGET_HARDWARE != HAL_TARGET_Y84001_REV_A )    && \
      ( HAL_TARGET_HARDWARE != HAL_TARGET_Y84020_1_REV_A )  && \
      ( HAL_TARGET_HARDWARE != HAL_TARGET_Y84114_1_REV_A )  && \
      ( HAL_TARGET_HARDWARE != HAL_TARGET_Y99852_1_REV_A )  && \
      ( HAL_TARGET_HARDWARE != HAL_TARGET_Y84030_1_REV_A ) )

#error "Invalid HAL_TARGET_HARDWARE setting"
#endif

// check for valid processor choice from hardware specific HAL
#if ( ( MCU_SELECTED != RA6E1 )    && \
      ( MCU_SELECTED != NXP_K24 ))

#error "Invalid MCU_SELECTED setting"
#endif

// check for valid RTOS choice from hardware specific HAL
#if ( ( RTOS_SELECTION != MQX_RTOS )    && \
      ( RTOS_SELECTION != FREE_RTOS )  && \
      ( RTOS_SELECTION != BARE_METAL))

#error "Invalid RTOS_SELECTION setting"
#endif

#include <intrinsics.h>
#include <MK24F12.h>
// Include HAL common to all End Point hardware
/* ****************************************************************************************************************** */
/* GLOBAL DEFINTION */


/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define PERSISTENT                  __no_init
#define CLRWDT()                    WDOG_Kick()
#define NOP()                       asm("nop")
#define RESET()                     { SCB_AIRCR = SCB_AIRCR_VECTKEY(0x5FA)| SCB_AIRCR_SYSRESETREQ_MASK; while(1){} }

#ifdef _DEBUG_MODE                   /* When using debugger, don't use the sleep instruction, use a NOP instead. */
   #define SLEEP()                  NOP()
   #define SLEEP_CPU_IDLE()         NOP()
#else
   #ifdef TM_POWER_UP_TIMING        /* For Test Mode, we need the perif. clk running. */
      #define SLEEP()               NOP()
   #else
      #define SLEEP()               sleepDeep()
   #endif
   #define SLEEP_CPU_IDLE()         sleepIdle()  /* WFI will start entry into WAIT mode (CPU Sleeps, Peripherals run) */
   #define SLEEP_RESTORE()
#endif

/* ------------------------------------------------------------------------------------------------------------------ */
/* General I/O Definitions */

#define  HIGH                    ((uint8_t)1)  /* Pin logic High */
#define  LOW                     ((uint8_t)0)  /* Pin Logic Low */

#define  TRIS_INPUT              1           /* Set data I/O Pin as an Input */
#define  TRIS_OUTPUT             0           /* Set data I/O Pin as an Output */

#define  ANALOG                  1           /* Set data I/O Pin as an Analog Input */
#define  DIGITAL                 0           /* Set data I/O Pin as a Digital I/O */

#define  OPEN_DRAIN_ON           1           /* Set data output pin as open drain output */
#define  OPEN_DRAIN_OFF          0           /* Set data output pin to drive output high/low */

#define  PULL_UP_EN              1           /* Enable pull-ups on a pin  */
#define  PULL_UP_DIS             0           /* Disable pull-ups on a pin  */

#define  PULL_DN_EN              1           /* Enable pull-downs on an input pin  */
#define  PULL_DN_DIS             0           /* Disable pull-downs on an input pin  */

#define  PERIPHERAL_DIS_FLAG     1           /* State that disables a module in the Peripheral Module. */
#define  PERIPHERAL_EN_FLAG      0           /* State that enables a module in the Peripheral Module. */


/* ------------------------------------------------------------------------------------------------------------------ */
/* Power Outage Definitions */

#if HAL_IGNORE_BROWN_OUT_SIGNAL == 1
#define BRN_OUT()                   0 /* Ignores the brown out signal  */
#warning "You have built the project so that it ignores the meter's /PF_METER signal!"
/* Disable this interrupt if ignoring the signal! */
#define BRN_OUT_IRQ_EI()            BRN_OUT_IRQ_DI()
#else
#define BRN_OUT()                   (!((GPIOD_PDIR & (1<<0)) >> 0)) /* Brown out - Active Low Signal */
#define BRN_OUT_TRIS()              {  PORTD_PCR0 = (PORTD_PCR0 & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX( 1 ); \
                                       GPIOD_PDDR &= ~(1<<0); } /* Set PCR for GPIO, Make Input */
#define BRN_OUT_IRQ_EI()            { BRN_OUT_TRIS(); \
                                      PORTD_PCR0 |= (PORT_PCR_IRQC(0xb)); } /* Interrupt on either edge */
#define BRN_OUT_ISF()               (PORTD_ISFR & 1)  /* For testing state of BRN_OUT interrupt flag. */
#define BRN_OUT_CLR_IF()            (PORTD_ISFR = 1)  /* For clearing BRN_OUT interrupt flag. */
#endif
#define BRN_OUT_PULLDN_ENABLE()     ( PORTD_PCR0 |= (    PORT_PCR_PE_MASK  & ~PORT_PCR_PS_MASK ) )
#define BRN_OUT_PULLUP_ENABLE()     ( PORTD_PCR0 |= (    PORT_PCR_PE_MASK | PORT_PCR_PS_MASK   ) )
#define BRN_OUT_PULLUP_DISABLE()    ( PORTD_PCR0 &= ( ~( PORT_PCR_PE_MASK | PORT_PCR_PS_MASK ) ) )
#define BRN_OUT_IRQIsrIndex         ((int)INT_PORTD)           /* Set to the same port that the BO signal is on */
#define BRN_OUT_ISR_PRI             ((uint8_t)2)
#define BRN_OUT_ISR_SUB_PRI         ((uint8_t)0)
#define BRN_OUT_IRQ_DI()            (PORTD_PCR0 &= (~PORT_PCR_IRQC(0x0))) /* Disable Interrupt */
#define DEBOUNCE_DELAY_VAL          ((uint32_t)10)       /* 100us (number of 10us delays) */
#define DEBOUNCE_CNT_RST_VAL        ((uint16_t)10)       /* Number of debounce delays to ensure signal stable */

#define LLWU_LG_IRQInterruptIndex   ( (int) INT_LLW )  // To enable the LLWU interrupt handler
#define LLWU_LG_ISR_PRI             ( (uint8_t)2)
#define LLWU_LG_ISR_SUB_PRI         ( (uint8_t)0)

/* ------------------------------------------------------------------------------------------------------------------ */
/* GPIO I/O Definitions */

#if ( TEST_REGULATOR_CONTROL_DISABLE == 0 )
// 3P6LDO_EN
#define PWR_3P6LDO_EN_OFF()      GPIOB_PCOR = 1<<1       /* 3.6V LDO Off  */
#define PWR_3P6LDO_EN_ON()       GPIOB_PSOR = 1<<1       /* 3.6V LDO On */
#define PWR_3P6LDO_EN_ON_DLY_MS  ((uint32_t)1)

/* Set PCR MUX for GPIO, Turn LDO On/Off, Make Output */
#define PWR_3P6LDO_EN_TRIS_ON()  { PORTB_PCR1 = 0x100; PWR_3P6LDO_EN_ON();  GPIOB_PDDR |= (1<<1); }
#define PWR_3P6LDO_EN_TRIS_OFF() { PORTB_PCR1 = 0x100; PWR_3P6LDO_EN_OFF(); GPIOB_PDDR |= (1<<1); }

// 3V6BOOST_EN
#define PWR_3V6BOOST_EN_OFF()      GPIOC_PCOR = 1<<0       /* 3.6V BOOST Off  */
#define PWR_3V6BOOST_EN_ON()       GPIOC_PSOR = 1<<0       /* 3.6V BOOST On */
#define PWR_3V6BOOST_EN_ON_DLY_MS  ((uint32_t)10)

/* Set PCR for GPIO, Turn Boost Off/On, Make Output */
#define PWR_3V6BOOST_EN_TRIS_OFF() { PORTC_PCR0 = 0x100; PWR_3V6BOOST_EN_OFF(); GPIOC_PDDR |= (1<<0);}
#define PWR_3V6BOOST_EN_TRIS_ON()  { PORTC_PCR0 = 0x100; PWR_3V6BOOST_EN_ON();  GPIOC_PDDR |= (1<<0);}

/* Use selected regulator */
#define PWR_USE_LDO()            { PWR_3P6LDO_EN_TRIS_ON();   OS_TASK_Sleep( PWR_3P6LDO_EN_ON_DLY_MS );   PWR_3V6BOOST_EN_TRIS_OFF(); }
#define PWR_USE_BOOST()          { PWR_3V6BOOST_EN_TRIS_ON(); OS_TASK_Sleep( PWR_3V6BOOST_EN_ON_DLY_MS ); PWR_3P6LDO_EN_TRIS_OFF(); }

/* Regulator selected? */
#define PWR_LDO_IN_USE()         ( ( GPIOB_PDOR & (1<<1) ) == 0 ? false : true )
#define PWR_BOOST_IN_USE()       ( ( GPIOC_PDOR & (1<<0) ) == 0 ? false : true )

#else  // TEST_REGULATOR_CONTROL_DISABLE == 1
/* Remove the control of Pins */
/* Note: DG: Changed the functionality leaving the name of the MACRO unchanged. Tried to minimize the code change */

// 3P6LDO_EN
#define PWR_3P6LDO_EN_OFF()
#define PWR_3P6LDO_EN_ON()
#define PWR_3P6LDO_EN_ON_DLY_MS  ((uint32_t)1)

/* Disable the MUX */
#define PWR_3P6LDO_EN_TRIS_ON()  { PORTB_PCR1 = PORT_PCR_MUX(0); }
#define PWR_3P6LDO_EN_TRIS_OFF() { PORTB_PCR1 = PORT_PCR_MUX(0); }

// 3V6BOOST_EN
#define PWR_3V6BOOST_EN_OFF()
#define PWR_3V6BOOST_EN_ON()
#define PWR_3V6BOOST_EN_ON_DLY_MS  ((uint32_t)10)

/* Disable the pin  */
#define PWR_3V6BOOST_EN_TRIS_OFF() { PORTC_PCR0 = PORT_PCR_MUX(0); }
#define PWR_3V6BOOST_EN_TRIS_ON()  { PORTC_PCR0 = PORT_PCR_MUX(0); }

/* Use selected regulator */
#define PWR_USE_LDO()            { PWR_3P6LDO_EN_TRIS_ON(); }
#define PWR_USE_BOOST()          { PWR_3V6BOOST_EN_TRIS_ON(); }


#define PWR_LDO_IN_USE()         ( true )
#define PWR_BOOST_IN_USE()       ( true )

#endif //( TEST_REGULATOR_CONTROL_DISABLE == 0 )

// ZCD_METER
#define HMC_ZCD_METER()          (((GPIOD_PDIR & (1<<1))>>1)^1) /* ZCD_METER signal from the host meter */
#define HMC_ZCD_METER_TRIS()     { PORTD_PCR1 = 0x400; }         /* Map ZCD_METER signal to FTM3_CH1 input */

/* HMC trouble pin interrupt definitions   */
#define HMC_TROUBLE_IRQIsrIndex       ((int)INT_PORTC)   /* Set to the same port that the HMC_TROUBLE signal is on */
#define HMC_TROUBLE_ISR_PRI           ((uint8_t)2)       /* ISR priority for the busy signal */
#define HMC_TROUBLE_ISR_SUB_PRI       ((uint8_t)0)       /* ISR sub-priority for the busy signal */
#define HMC_TROUBLE_EDGE_TRIGGERED 1

/* Enable Meter trouble IRQ   */
#if ( HMC_TROUBLE_EDGE_TRIGGERED != 0 )
#define HMC_TROUBLE_BUSY_IRQ_EI()   {PORTC_ISFR = (1 << 11);PORTC_PCR11 |= PORT_PCR_IRQC(0xb);} /* IRQ - either edge */
#else
#define HMC_TROUBLE_BUSY_IRQ_EI()   {PORTC_ISFR = (1 << 11);PORTC_PCR11 |= PORT_PCR_IRQC(0xc);} /* IRQ - high level  */
#endif

/* Disable Meter trouble IRQ and reset IRQ flag */
#define HMC_TROUBLE_BUSY_IRQ_DI()   { PORTC_PCR11 &= ~PORT_PCR_IRQC(0xf); PORTC_ISFR = ( 1 << 11 ); }
#define HMC_TROUBLE_BUSY_TRIG       (PORTC_ISFR & (1 << 11)   /* ISF Triggered? */

/* Definitions for network status signal */
#define SIGNAL_NETWORK_UP() {PORTC_PCR1 = 0x100; GPIOC_PSOR = 1<<1; GPIOC_PDDR |= (1<<1);}
#define SIGNAL_NETWORK_DOWN() {PORTC_PCR1 = 0x100; GPIOC_PCOR = 1<<1;  GPIOC_PDDR |= (1<<1);}

/* Definitions for DA host reset signal */
#define DA_HOLD_HOST_RESET() {PORTC_PCR9 = 0x100; GPIOC_PSOR = 1<<9; GPIOC_PDDR |= (1<<9);}
#define DA_RELEASE_HOST_RESET() {PORTC_PCR9 = 0x100; GPIOC_PCOR = 1<<9;  GPIOC_PDDR |= (1<<9);}
#define DA_HOLD_SOM_OFF() {PORTE_PCR25 = 0x100; GPIOE_PSOR = 1<<25; GPIOE_PDDR |= (1<<25);}
#define DA_RELEASE_SOM_OFF() {PORTE_PCR25 = 0x100; GPIOE_PCOR = 1<<25;  GPIOE_PDDR |= (1<<25);}

/* Definitions for DA host GPS chip */
#define DA_GPS_EN_ON() {PORTC_PCR18 = 0x100; GPIOC_PSOR = 1<<18; GPIOC_PDDR |= (1<<18);}
#define DA_GPS_EN_OFF() {PORTC_PCR18 = 0x100; GPIOC_PCOR = 1<<18;  GPIOC_PDDR |= (1<<18);}

/* ------------------------------------------------------------------------------------------------------------------ */
/* LED definitions */

#define LED0_PIN                 (1<<0)
#define LED1_PIN                 (1<<1)
#define LED2_PIN                 (1<<2)

//LED0
#define GRN_LED_OFF()            (GPIOE_PCOR = LED0_PIN)     /* LED Off  */
#define GRN_LED_ON()             (GPIOE_PSOR = LED0_PIN)     /* LED On */
#define GRN_LED_TOGGLE()         (GPIOE_PTOR = LED0_PIN)     /* LED Toggle */
#define GRN_LED_PIN_DRV_HIGH()   (PORTE_PCR0 = 0x140)        /* LED High Drive strength and set muxing slot as GPIO */
#define GRN_LED_PIN_DRV_LOW()    (PORTE_PCR0 = 0x100)        /* LED Low Drive strength and set muxing slot as GPIO  */
/* Set PCR for GPIO, Make Output */
#define GRN_LED_PIN_TRIS()       { GRN_LED_PIN_DRV_HIGH();   GRN_LED_OFF(); GPIOE_PDDR |= LED0_PIN; }
/* Configuring the unused LED0's pin mode to analog which will result in lowest power consumption */
#define GRN_LED_PIN_DISABLE()    (PORTE_PCR0 = 0x00)

//LED1
#define RED_LED_OFF()            (GPIOE_PCOR = LED1_PIN)     /* LED Off  */
#define RED_LED_ON()             (GPIOE_PSOR = LED1_PIN)     /* LED On */
#define RED_LED_TOGGLE()         (GPIOE_PTOR = LED1_PIN)     /* LED Toggle */
#define RED_LED_PIN_DRV_HIGH()   (PORTE_PCR1 = 0x140)        /* LED High Drive strength and set muxing slot as GPIO */
#define RED_LED_PIN_DRV_LOW()    (PORTE_PCR1 = 0x100)        /* LED Low Drive strength and set muxing slot as GPIO  */
/* Set PCR for GPIO, Make Output */
#define RED_LED_PIN_TRIS()       { RED_LED_PIN_DRV_HIGH();   RED_LED_OFF(); GPIOE_PDDR |= LED1_PIN; }
/* Configuring the unused LED1's pin mode to analog which will result in lowest power consumption */
#define RED_LED_PIN_DISABLE()    (PORTE_PCR1 = 0x00)

//LED2
#define BLU_LED_OFF()            (GPIOE_PCOR = LED2_PIN)     /* LED Off  */
#define BLU_LED_ON()             (GPIOE_PSOR = LED2_PIN)     /* LED On */
#define BLU_LED_TOGGLE()         (GPIOE_PTOR = LED2_PIN)     /* LED Toggle */
#define BLU_LED_PIN_DRV_HIGH()   (PORTE_PCR2 = 0x140)        /* LED High Drive strength and set muxing slot as GPIO */
#define BLU_LED_PIN_DRV_LOW()    (PORTE_PCR2 = 0x100)        /* LED Low Drive strength and set muxing slot as GPIO  */
/* Set PCR for GPIO, Make Output */
#define BLU_LED_PIN_TRIS()       { BLU_LED_PIN_DRV_HIGH();   BLU_LED_OFF(); GPIOE_PDDR |= LED2_PIN; }
/* Configuring the unused LED2's pin mode to analog which will result in lowest power consumption */
#define BLU_LED_PIN_DISABLE()    (PORTE_PCR2 = 0x00)

/* LED configuration. */
#define LED_HMC_OFF()            LED0_PIN_OFF()
#define LED_HMC_ON()             LED0_PIN_ON()
#define LED_HMC_INIT()           LED0_PIN_TRIS()

#if ( ENABLE_TRACE_PINS_LAST_GASP == 1 )
/* Trace D0 Config */
#define TRACE_D0_PIN             ( 1 << 4 )
#define TRACE_D0_HIGH()          ( GPIOE_PSOR |= TRACE_D0_PIN )
#define TRACE_D0_LOW()           ( GPIOE_PCOR |= TRACE_D0_PIN )
#define TRACE_D0_TRIS()          { PORTE_PCR4 = ( PORT_PCR_MUX(0x01) | PORT_PCR_DSE_MASK );  TRACE_D0_LOW() ; GPIOE_PDDR |= TRACE_D0_PIN;  }
#define TRACE_D0_TOGGLE()        ( GPIOE_PTOR = TRACE_D0_PIN )
/* Trace D1 Config */
#define TRACE_D1_PIN             ( 1 << 3 )
#define TRACE_D1_HIGH()          ( GPIOE_PSOR |= TRACE_D1_PIN )
#define TRACE_D1_LOW()           ( GPIOE_PCOR |= TRACE_D1_PIN )
#define TRACE_D1_TRIS()          { PORTE_PCR3 = ( PORT_PCR_MUX(0x01) | PORT_PCR_DSE_MASK );  TRACE_D1_LOW() ; GPIOE_PDDR |= TRACE_D1_PIN;  }
#define TRACE_D1_TOGGLE()        ( GPIOE_PTOR = TRACE_D1_PIN )
#endif

/* ------------------------------------------------------------------------------------------------------------------ */
/* Devices - Peripherals */
/* ------------------------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------------------------ */
/* Serial Com Ports */

#define MFG_PORT_IO_CHANNEL      "ittya:"
#define DBG_PORT_IO_CHANNEL      "ittyc:"

#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84030_1_REV_A )    /* KV2c  */
#define OPTICAL_PORT_IO_CHANNEL  "ittyb:"
#define HOST_PORT_IO_CHANNEL     "ittyd:"

#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84020_1_REV_A )  /* i210+c   */
#define HOST_PORT_IO_CHANNEL     "ittyd:"

#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y99852_1_REV_A )  /* Aclara LC   */
#define HOST_PORT_IO_CHANNEL     "ittyd:"

#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84114_1_REV_A )  /* Aclara DA   */
#define HOST_PORT_IO_CHANNEL     "ittye:"
#endif


/* ------------------------------------------------------------------------------------------------------------------ */
/* External Flash Driver */

#define NV_SPI_PORT_NUM                ((uint8_t)0)            /* SPI Port # used in the Micro. */
#define NV_SPI_PORT_CONTROLS_CS        0                       /* 1 = SPI port controls the CS line */

#define NV_SPI_PORT_INIT               SPI_initPort
#define NV_SPI_PORT_OPEN               SPI_OpenPort
#define NV_SPI_PORT_CLOSE              SPI_ClosePort
#define NV_SPI_PORT_WRITE              SPI_WritePort
#define NV_SPI_PORT_READ               SPI_ReadPort

/* Set PCR for GPIO, high drive strength, Make Output */
#define NV_CS_TRIS()                   { PORTA_PCR14 = 0x100; GPIOA_PDDR |= 1<<14; }
/* For Last Gasp recovery from low power, Set PCR MUX for GPIO, Make Output, Chip Not Selected */
#define NV_CS_TRIS_LG()                { PORTA_PCR14 = 0x100; GPIOA_PDDR |= 1<<14; NV_CS_INACTIVE(); }
#define NV_CS_ACTIVE()                 (GPIOA_PCOR = 1<<14)    /* NV memory chip select pin */
#define NV_CS_INACTIVE()               (GPIOA_PSOR = 1<<14)    /* NV memory chip select pin */
#define NV_BUSY()                      (~(GPIOA_PDIR >> 17) & 1)/* NV memory chip busy pin */
#define NV_MISO_CFG(port, cfg)         SPI_misoCfg(port, cfg)

/* Set PCR for GPIO, Make Output */
#define NV_WP_TRIS()                   { PORTB_PCR18 = 0x100; GPIOB_PDDR |= 1<<18; }
/* For Last Gasp recovery from low power, Set PCR MUX for GPIO, Make Output, Write Protect */
#define NV_WP_TRIS_LG()                { PORTB_PCR18 = 0x100; GPIOB_PDDR |= 1<<18; NV_WP_ACTIVE(); }
#define NV_WP_ACTIVE()                 (GPIOB_PCOR = 1<<18)     /* NV memory Write Protect pin */
#define NV_WP_INACTIVE()               (GPIOB_PSOR = 1<<18)     /* NV memory Write Protect pin */

/* External Flash memory driver. */
#define DVR_EFL_BUSY_IRQIsrIndex       ((int)INT_PORTA)    /* Set to the same port that the SPI MISO signal is on */
#define DVR_EFL_BUSY_ISR_PRI           ((uint8_t)2)        /* ISR priority for the busy signal */
#define DVR_EFL_BUSY_ISR_SUB_PRI       ((uint8_t)0)        /* ISR sub-priority for the busy signal */
#define DVR_EFL_BUSY_EDGE_TRIGGERED 0
#if ( DVR_EFL_BUSY_EDGE_TRIGGERED != 0 )
#define DVR_EFL_BUSY_IRQ_EI()          { PORTA_ISFR = (1 << 17); PORTA_PCR17 |= PORT_PCR_IRQC(0x9); } /*  IRQ on risiing
                                                                                                          edge */
#else
#define DVR_EFL_BUSY_IRQ_EI()          { PORTA_ISFR = (1 << 17); PORTA_PCR17 |= PORT_PCR_IRQC(0xc); } /* IRQ on high
                                                                                                         level  */
#endif
/* Disable flash busy IRQ and reset IRQ flag */
#define DVR_EFL_BUSY_IRQ_DI()          { PORTA_PCR17 &= ~PORT_PCR_IRQC(0xf); PORTA_ISFR = ( 1 << 17 ); }
#define DVR_EFL_BUSY_TRIG              (PORTA_ISFR & (1 << 17)   /* ISF Triggered? */

/* The following definition is only used when the SPI driver in the device is used to control the CS pin.
 * NOTE:  For this project, this is not used.  The CS pin is controlled directly with the Ext flash driver. */
#define EXT_FLASH_CS                   ((uint8_t)1)         /* Used to define which CS to use. */

#define EXT_FLASH_WP_USED              1                    /* Set to 1 if WP pin is used */
#define EXT_FLASH_SPEED_KHZ            ((uint16_t)5000)     /* Todo:  This will change - Speed of the SPI Clock in kHz. */
#define EXT_FLASH_SPI_MODE             ((uint8_t)0)         /* Use SPI mode 0, 1, 2 or 3 */
#define EXT_FLASH_TX_BYTE_WHEN_RX      ((uint8_t)0xff)      /* Byte to send when receiving data */

/* The following time out values are based on the processor (pbus) speed and the SPI clock rate!!! */
#define EXT_FLASH_BUSY_TOUT_READS      ((uint32_t)10000)    /* # of status reads before giving up on busy signal. */
#define EXT_FLASH_BUSY_RETRIES         ((uint8_t)2)         /* # of times to retry checking for /busy signal */

/* Every part defined uses a sector size of 4096 and a block size of 65536.  The 32K block size is NOT used because
 * not every part supports it.  */
#if ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )
#define EXT_FLASH_SIZE                 ((uint32_t)1048576)  /* Size in bytes of the external flash memory */
#else
#define EXT_FLASH_SIZE                 ((uint32_t)2097152)  /* Size in bytes of the external flash memory */
#endif
#define EXT_FLASH_SECTOR_SIZE          ((uint32_t)4096)     /* Minimum sector size */
#define EXT_FLASH_BLOCK_SIZE           ((uint32_t)65536)    /* Maximum erase size (aside from chip erase) */

/* Timer used by the flash driver.  See the "busyCheck()" function. */
#define EXT_FLASH_TIMER_EN()           { LPTMR0_CSR |= (LPTMR_CSR_TEN_MASK | LPTMR_CSR_TIE_MASK); }  /* En and strt */
#define EXT_FLASH_TIMER_DIS()          { LPTMR0_CSR &= ~(LPTMR_CSR_TEN_MASK | LPTMR_CSR_TIE_MASK); } /* Disable & stp */
#define EXT_FLASH_TIMER_COMPARE        LPTMR0_CMR
#define EXT_FLASH_TIMER_RST()          { LPTMR0_CNR = 0; }

/* ------------------------------------------------------------------------------------------------------------------ */
/* SPI and SPI DMA Specific Macros */

/* General settings for SPI and DMA */
#ifndef __BOOTLOADER
#define SPI_USES_DMA                      1                      /* 0 = Don't Use DMA (slower), 1 = use DMA */
#else
#define SPI_USES_DMA                      0                      /* 0 = Don't Use DMA (slower), 1 = use DMA */
#endif
#define SPI_UNIT_TEST_PORT                ((uint8_t)1)           /* SPI Port to use when running unit test code */
#define SPI_UNIT_TEST_PORT_INVALID        ((uint8_t)2)           /* Number of SPI ports the controller supports */
#define SPI_MAX_SPEED_MHz                 ((uint32_t)30000000)   /* Max clk speed the controller supports */
#define SPI_WR_TIME_OUT_mS                ((uint32_t)2000)       /* mS to wait for tx/rx to complete before time-out */

/* The pull-up is used so that the MISO pin is pulled to a known state.  Some external flash devices use MISO to
   indicate the device is busy.  The busy indication is a low signal.  Upon assertion of the CS#, pin, the MISO pin
   becomes the Ready/Busy# signal. Using the pull-down results in a very slow decay time from high during the write
   operation to tri-state while deselected. Using pull-up keeps the pin high during the deselected state and accurately
   reflects the chips readiness immediately upon re-selecting the device.  */
#define SPI_MISO_EN_PULLDOWN              ((uint8_t)0x02)        /* Value that indicates a pull-down in PORT_PCR_MUX */
#define SPI_MISO_EN_PULLUP                ((uint8_t)0x03)        /* Value that indicates a pull-up   in PORT_PCR_MUX */

/* Define each pin for Port 0 (Ext Flash)*/
#define SPI_PORT_0_CLK_PIN                (uint32_t volatile *)&PORTA_PCR15
#define SPI_PORT_0_CLK_PIN_MUX            PORT_PCR_MUX(2)
#define SPI_PORT_0_MOSI_PIN               (uint32_t volatile *)&PORTA_PCR16
#define SPI_PORT_0_MOSI_PIN_MUX           PORT_PCR_MUX(2)
#define SPI_PORT_0_MISO_PIN               (uint32_t volatile *)&PORTA_PCR17
#define SPI_PORT_0_MISO_PIN_MUX           PORT_PCR_MUX(2) | SPI_MISO_EN_PULLUP
#define SPI_PORT_0_CS_PIN                 (uint32_t volatile *)&PORTA_PCR14
#define SPI_PORT_0_CS_PIN_MUX             PORT_PCR_MUX(1)   /* Set to 0 if not used by the SPI module */

/* Define each pin for Port 1 (radio)*/
#define SPI_PORT_1_CLK_PIN                (uint32_t volatile *)&PORTB_PCR11
#define SPI_PORT_1_CLK_PIN_MUX            (PORT_PCR_MUX(2) | PORT_PCR_DSE_MASK | PORT_PCR_SRE_MASK)
#define SPI_PORT_1_MOSI_PIN               (uint32_t volatile *)&PORTB_PCR16
#define SPI_PORT_1_MOSI_PIN_MUX           (PORT_PCR_MUX(2) | PORT_PCR_DSE_MASK | PORT_PCR_SRE_MASK)
#define SPI_PORT_1_MISO_PIN               (uint32_t volatile *)&PORTB_PCR17
#define SPI_PORT_1_MISO_PIN_MUX           PORT_PCR_MUX(2) | SPI_MISO_EN_PULLDOWN
#define SPI_PORT_1_CS_PIN                 (uint32_t volatile *)&PORTB_PCR10
#define SPI_PORT_1_CS_PIN_MUX             PORT_PCR_MUX(1)   /* Set to 0 if not used by the SPI module */
/* For Last Gasp recovery from low power, Set PCR MUX for GPIO, Make Output, Disabled */
#define RDO_CS_SI4460_TRIS_LG()           { PORTB_PCR10 = 0x100;   GPIOB_PDDR |= (1<<10); GPIOB_PSOR = 1<<10;}

/* Define each pin for Port 2 (expansion port) */
#define SPI_PORT_2_CLK_PIN                (uint32_t volatile *)&PORTB_PCR21
#define SPI_PORT_2_CLK_PIN_MUX            PORT_PCR_MUX(2)
#define SPI_PORT_2_MOSI_PIN               (uint32_t volatile *)&PORTB_PCR22
#define SPI_PORT_2_MOSI_PIN_MUX           PORT_PCR_MUX(2)
#define SPI_PORT_2_MISO_PIN               (uint32_t volatile *)&PORTB_PCR23
#define SPI_PORT_2_MISO_PIN_MUX           PORT_PCR_MUX(2) | SPI_MISO_EN_PULLDOWN
#define SPI_PORT_2_CS_PIN                 (uint32_t volatile *)&PORTB_PCR20
#define SPI_PORT_2_CS_PIN_MUX             PORT_PCR_MUX(1)   /* Set to 0 if not used by the SPI module */

/* Define the DMA used for each channel */
#define SPI0_TX_DMA_CH_ISR                INT_DMA0          /* DMA Ch Transfer Complete Interrupt */
#define SPI0_RX_DMA_CH_ISR                INT_DMA1          /* DMA Ch Transfer Complete Interrupt */
#define SPI0_TX_LINK_CHAN                 63                /* DMA channel used for SPI 1 TX       */

#define SPI1_TX_DMA_CH_ISR                INT_DMA2          /* DMA Ch Transfer Complete Interrupt */
#define SPI1_RX_DMA_CH_ISR                INT_DMA3          /* DMA Ch Transfer Complete Interrupt */
#define SPI1_TX_LINK_CHAN                 62                /* DMA channel used for SPI 1 TX       */

#define SPI2_TX_DMA_CH_ISR                INT_DMA4          /* DMA Ch Transfer Complete Interrupt */
#define SPI2_RX_DMA_CH_ISR                INT_DMA5          /* DMA Ch Transfer Complete Interrupt */
#define SPI2_TX_LINK_CHAN                 61                /* DMA channel used for SPI 2 TX       */

/* The source register IDs are located in the K22 Reference Manual, K22P80M120SF5RM, 3.3.9.1 DMA MUX request sources.
 * Table 3-25. DMA request sources - MUX 0 and Table 3-24. DMA Request Sources - Mux 0 */
#define SPI0_RX_DMA_SRC                   ((uint8_t)14)       /* SPI 0 RX DMA Request Source */
#define SPI0_TX_DMA_SRC                   ((uint8_t)15)
#define SPI1_RX_DMA_SRC                   ((uint8_t)16)
#define SPI1_TX_DMA_SRC                   ((uint8_t)16)
#define SPI2_RX_DMA_SRC                   ((uint8_t)17)
#define SPI2_TX_DMA_SRC                   ((uint8_t)17)

// Datasheet table 3-27
#define PORTA_DMA_SRC                     ((uint8_t)49)     /* Used to compute TCXO clock rate */
#define PORTB_DMA_SRC                     ((uint8_t)50)     /* Used to compute TCXO clock rate */
#define PORTC_DMA_SRC                     ((uint8_t)51)     /* Used to compute TCXO clock rate */
#define PORTD_DMA_SRC                     ((uint8_t)52)     /* Used to compute TCXO clock rate */
#define PORTE_DMA_SRC                     ((uint8_t)53)     /* Used to compute TCXO clock rate */

#define SPI0_TX_DMA_CH                    ((uint8_t)0)        /* DMA channel to use for SPI port 0 TX */
#define SPI0_RX_DMA_CH                    ((uint8_t)1)
#define SPI1_TX_DMA_CH                    ((uint8_t)2)
#define SPI1_RX_DMA_CH                    ((uint8_t)3)
#define SPI2_TX_DMA_CH                    ((uint8_t)4)
#define SPI2_RX_DMA_CH                    ((uint8_t)5)
#define RADIO_CLK_DMA_CH                  ((uint8_t)6)

/* Enables the clocks to the eDMA module. */
#define SPI_DMA_CLK_CFG()                 { SIM_SCGC6 |= SIM_SCGC6_DMAMUX_MASK; SIM_SCGC7 |= SIM_SCGC7_DMA_MASK; }

/* Define the ISR priority for the SPI/DMA module.  */
#define SPI_ISR_PRIORITY                  ((uint8_t)5)        /* SPI DMA ISR priority setting */
#define SPI_ISR_SUBPRIORITY               ((uint8_t)0)        /* SPI DMA ISR subpriority setting */// </editor-fold>

/* ------------------------------------------------------------------------------------------------------------------ */
/* I2C Configuration */

/* ------------------------------------------------------------------------------------------------------------------ */
/* File I/O System */

#define FIO_MAX_FILE_SIZE_WITH_CHECKSUM ((uint8_t)64)

/* ------------------------------------------------------------------------------------------------------------------ */
/* Clock Definitions */

/* ------------------------------------------------------------------------------------------------------------------ */
/* Memory Definitions */


/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eINIT_RTI_CLR = (uint8_t)0,
   eINIT_RTI_SET,
   eINIT_WR_REG
}initCmd_t;


/* ****************************************************************************************************************** */
/* CONSTANTS */


/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// End Include HAL common to all End Point hardware
#endif
/* End File:            cfg_hal.h   */
