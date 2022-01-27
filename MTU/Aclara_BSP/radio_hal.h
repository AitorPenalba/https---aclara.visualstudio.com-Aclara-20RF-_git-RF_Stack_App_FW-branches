/*!
 * File:
 *  radio_hal.h
 *
 * Description:
 *  This file contains RADIO HAL.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */

#ifndef _RADIO_HAL_H_
#define _RADIO_HAL_H_

#include <stdbool.h>
#include <fio.h>

                /* ======================================= *
                 *              I N C L U D E              *
                 * ======================================= */

                /* ======================================= *
                 *          D E F I N I T I O N S          *
                 * ======================================= */
/* Radioes */
typedef enum   /* Do NOT start at 0 - mqx uses 0 to DESELECT a SPI device  */
{
   /* Note: Only 1 radio on Samwise */
   RADIO_0  = 0,        /* Only Radio */
   MAX_RADIO
} RadioPort;

#define RADIO_FIRST_RX           RADIO_0
#define RADIO_SPI_NAME           "spi1:"
#define SIG_ASSERT               ((uint8_t)0)
#define SIG_RELEASE              ((uint8_t)1)

#define RADIO_0_SPI_PORT_NUM     ((uint8_t)1)            /* SPI Port # used in the Micro.       */
#define RADIO_0_CS_ACTIVE()      GPIOB_PCOR = 1<<10      /* Radio chip select pin asserted      */
#define RADIO_0_CS_INACTIVE()    GPIOB_PSOR = 1<<10      /* Radio chip select pin released      */
#define RADIO_0_CS_TRIS()        { RADIO_0_CS_INACTIVE(); PORTB_PCR10 = 0x100; GPIOB_PDDR |= 1<<10;}
#define RADIO_0_SDN_ACTIVE()     GPIOB_PSOR = 1<<19      /* Radio powered OFF      */
#define RADIO_0_SDN_INACTIVE()   GPIOB_PCOR = 1<<19      /* Radio powered ON      */
#define RADIO_0_SDN_TRIS()       { RADIO_0_SDN_ACTIVE(); PORTB_PCR19 = 0x100; GPIOB_PDDR |= (1<<19);}
#define RADIO_0_WP_USED          0                       /* Set to 1 if WP pin is used */

// Maximum SPIspeed needed for soft-demodulator
#define RADIO_0_SLOW_SPEED_KHZ   ((uint16_t)500)         /* Todo:  This will change - Speed of the SPI Clock in kHz. */
#define RADIO_0_FAST_SPEED_KHZ   ((uint16_t)10000)       /* Todo:  This will change - Speed of the SPI Clock in kHz. */
#define RADIO_0_SPI_MODE         ((uint8_t)0)            /* Use SPI mode 0, 1, 2 or 3 */
#define RADIO_0_TX_BYTE_WHEN_RX  ((uint8_t)0xff)         /* Byte to send when receiving data */

#define RX_RADIO_SDN_ACTIVE()
#define RX_RADIO_SDN_INACTIVE()
#define RX_RADIO_SDN_TRIS()

// SDN_SI4460 Signal
#define RDO_SDN_ACTIVE()         RADIO_0_SDN_ACTIVE()    /* Radio Off */
#define RDO_SDN_INACTIVE()       RADIO_0_SDN_INACTIVE()  /* Radio On  */
/* Set PCR MUX for GPIO, Make Output, Shutdown Radio */
#define RDO_SDN_TRIS()           RADIO_0_SDN_TRIS()

// /OSC_EN Pin
#define RDO_OSC_EN_OFF()         GPIOC_PSOR = 1<<7       /* Radio OSC Off */
#define RDO_OSC_EN_ON()          GPIOC_PCOR = 1<<7       /* Radio OSC On  */
/* Set PCR MUX for GPIO, Make Output, Disable OSC */
#define RDO_OSC_EN_TRIS()        { RDO_OSC_EN_OFF(); PORTC_PCR7 = 0x100; GPIOC_PDDR |= (1<<7);}
/* For Last Gasp recovery from low power, Set PCR MUX for GPIO, Make Output, Disable OSC */
#define RDO_OSC_EN_TRIS_LG()     { RDO_OSC_EN_OFF(); PORTC_PCR7 = 0x100; GPIOC_PDDR |= (1<<7);}

// RX0TX1 Pin
#define RDO_RX0TX1_RX()          GPIOC_PCOR = 1<<10       /* Radio RX\TX to RX */
#define RDO_RX0TX1_TX()          GPIOC_PSOR = 1<<10       /* Radio RX\TX to TX */
/* Set PCR MUX for GPIO, Make Output, RX */
#define RDO_RX0TX1_TRIS()        { RDO_RX0TX1_RX(); PORTC_PCR10 = 0x100; GPIOC_PDDR |= (1<<10);}
/* For Last Gasp recovery from low power, Set PCR MUX for GPIO, Make Output, RX */
#define RDO_RX0TX1_TRIS_LG()     { RDO_RX0TX1_RX(); PORTC_PCR10 = 0x100; GPIOC_PDDR |= (1<<10);}

// PA_EN Pin
#define RDO_PA_EN_OFF()          GPIOC_PCOR = 1<<8       /* Radio PA Off */
#define RDO_PA_EN_ON()           GPIOC_PSOR = 1<<8       /* Radio PA On  */
/* Set PCR MUX for GPIO, Make Output, PA Off */
#define RDO_PA_EN_TRIS()         { RDO_PA_EN_OFF(); PORTC_PCR8 = 0x100; GPIOC_PDDR |= (1<<8);}

// /IRQ_SI4460 Pin
#define RDO_0_IRQ()              (GPIOB_PDIR & 1)         /* /IRQ signal from the Radio */
#define RDO_0_IRQ_TRIS()         ( PORTB_PCR0 = 0x300 )   /* Set PCR for FTM1_CH0. Make Input */

// GPIO0 Pin
#define RDO_0_GPIO0_Bit           5
#define RDO_0_GPIO0()            ((GPIOC_PDIR >> RDO_0_GPIO0_Bit) & 1)  /* GPIO0 level from radio */
#define RDO_0_GPIO0_TRIS()       { PORTC_PCR5 = 0x100;    GPIOC_PDDR &= ~(1<<RDO_0_GPIO0_Bit); } /* Set PCR for GPIO0, Make Input */

// GPIO1 Pin
#define RDO_0_GPIO1_Bit           6
#define RDO_0_GPIO1()            ((GPIOC_PDIR >> RDO_0_GPIO1_Bit) & 1)  /* GPIO1 level from radio */
#define RDO_0_GPIO1_TRIS()       { PORTC_PCR6 = 0x100;    GPIOC_PDDR &= ~(1<<RDO_0_GPIO1_Bit); } /* Set PCR for GPIO1, Make Input */

// GPIO2 Pin
#define RDO_0_GPIO2_Bit           12
#define RDO_0_GPIO2()            ((GPIOA_PDIR >> RDO_0_GPIO2_Bit) & 1)  /* GPIO2 level from radio */
#define RDO_0_GPIO2_TRIS()       { PORTA_PCR12 = 0x100;    GPIOA_PDDR &= ~(1<<RDO_0_GPIO2_Bit); } /* Set PCR for GPIO2, Make Input */

// GPIO3 Pin
#define RDO_0_GPIO3_Bit           13
#define RDO_0_GPIO3()            ((GPIOA_PDIR >> RDO_0_GPIO3_Bit) & 1)  /* GPIO3 level from radio */
#define RDO_0_GPIO3_TRIS()       { PORTA_PCR13 = 0x100;    GPIOA_PDDR &= ~(1<<RDO_0_GPIO3_Bit); } /* Set PCR for GPIO3, Make Input */

// /RESET_ZICM357 Pin
#define ZB_RESET_INACTIVE()      GPIOC_PSOR = 1<<9       /* ZB Radio Off */
#define ZB_RESET_ACTIVE()        GPIOC_PCOR = 1<<9       /* ZB Radio On  */
/* Set PCR MUX for GPIO, Make Output, ZB Off */
#define ZB_RESET_TRIS()          { ZB_RESET_INACTIVE(); PORTC_PCR9 = 0x100; GPIOC_PDDR |= (1<<9);}
/* For Last Gasp recovery from low power, Set PCR MUX for GPIO, Make Output, ZB Off */
#define ZB_RESET_TRIS_LG()       { ZB_RESET_ACTIVE();   PORTC_PCR9 = 0x100; GPIOC_PDDR |= (1<<9);}

/* ------------------------------------------------------------------------------------------------------------------ */

                /* ======================================= *
                 *     G L O B A L   V A R I A B L E S     *
                 * ======================================= */

                /* ======================================= *
                 *  F U N C T I O N   P R O T O T Y P E S  *
                 * ======================================= */

void     radio_hal_AssertShutdown( uint8_t radioNum );
void     radio_hal_DeassertShutdown( uint8_t radioNum );
uint8_t  radio_hal_ClearNsel( uint8_t radioNum );
void     radio_hal_SetNsel(uint8_t spifd);
bool     radio_hal_NirqLevel(void);

void     radio_hal_SpiInit( bool fastSPI );
void     radio_hal_SpiWriteByte(uint8_t spifd, uint8_t byteToWrite);
uint8_t  radio_hal_SpiReadByte(uint8_t spifd);

void     radio_hal_SpiWriteData(uint8_t spifd, uint8_t byteCount, uint8_t const * pData);
void     radio_hal_SpiReadData(uint8_t spifd, uint8_t byteCount, uint8_t* pData);
void     radio_hal_RadioImmediateSDN(void);

#endif //_RADIO_HAL_H_
