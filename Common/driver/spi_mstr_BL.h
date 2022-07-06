/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename: spi_mstr.h
 *
 * Contents: SPI driver for the Freescale controller.  This driver uses the SPI hardware in the Freescale K60.
 *
 ***********************************************************************************************************************
 * Copyright (c) 2013 Aclara Power-Line Systems Inc.  All rights reserved.  This program may not be reproduced, in whole
 * or in part, in any form or by any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 *
 * $Log$ kdavlin Created January 15, 2013
 *
 **********************************************************************************************************************/
#ifndef SPI_MSTR_H_
#define SPI_MSTR_H_

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "project_BL.h"
#include "partitions_BL.h"

/* ****************************************************************************************************************** */
/* Global Definition */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define SPI_MODE_0      ((uint8_t)0)
#define SPI_MODE_1      ((uint8_t)1)
#define SPI_MODE_2      ((uint8_t)2)
#define SPI_MODE_3      ((uint8_t)3)

#define SPI_MASTER      ((bool)true)
#define SPI_SLAVE       ((bool)false)

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   uint16_t   clkBaudRate_Khz;      // Serial clock baud rate
   uint8_t    txByte;               // Sent when receiving data.
   unsigned mode: 2;                // Clock Phase and clock polarity settings
                                    // Mode 0: CKP = false, SMP = true
                                    // Mode 1: CKP = false, SMP = false
                                    // Mode 2: CKP = true,  SMP = true
                                    // Mode 3: CKP = true,  SMP = false
}spiCfg_t;                          // SPI Port Configuration

typedef enum
{
   SPI_MISO_SPI_e = 0,                  // Configure MISO for SPI functionality
   SPI_MISO_GPIO_e                      // Configure MISO for GPIO functionality
}spiMisoCfg_e;

/*lint -esym(754,rsvd,rsvd2,rsvd3,rsvd4,rsvd5) */
// <editor-fold defaultstate="collapsed" desc="SpiMcr_t - DSPI Module Configuration Register (SPIx_MCR)">
/*lint --e{754} Ignore symbol not referenced. */
typedef struct
{
   unsigned halt:    1;    // Starts and stops DSPI transfers. 0 = Start, 1 = Stop
   unsigned rsvd:    7;    // Not used and always has the value zero
   unsigned smplPt:  2;    /* Controls when the DSPI master samples SIN in Modified Transfer Format. This field is valid
                            * only when CPHA bit in CTAR register is 0.
                            *    00 = system clocks between SCK edge and SIN sample
                            *    01 = system clock between SCK edge and SIN sample
                            *    10 = system clocks between SCK edge and SIN sample
                            *    11 = Reserved */
   unsigned clrRxf:  1;    /* Flushes the RX FIFO. Writing a 1 to CLR_RXF clears the RX Counter. The CLR_RXF bit is
                            * always read as zero.
                            *    0 Do not clear the Rx FIFO counter.
                            *    1 Clear the Rx FIFO counter. */
   unsigned clrTxf:  1;    /* Clear TX FIFO Flushes the TX FIFO. Writing a 1 to CLR_TXF clears the TX FIFO Counter. The
                            * CLR_TXF bit is always read as zero.
                            *    0 Do not clear the Tx FIFO counter.
                            *    1 Clear the Tx FIFO counter. */
   unsigned disRxf:  1;    /* When the RX FIFO is disabled, the receive part of the DSPI operates as a simplified
                            * double-buffered SPI.  This bit can only be written when the MDIS bit is cleared.
                            *    0 Rx FIFO is enabled.
                            *    1 Rx FIFO is disabled. */
   unsigned disTxf:  1;    /* When the TX FIFO is disabled, the transmit part of the DSPI operates as a simplified
                            * double-buffered SPI. This bit can only be written when the MDIS bit is cleared.
                            *    0 Tx FIFO is enabled.
                            *    1 Tx FIFO is disabled. */
   unsigned mdis:    1;    /* Allows the clock to be stopped to the non-memory mapped logic in the DSPI effectively
                            * putting the DSPI in a software controlled power-saving state. The reset value of the MDIS
                            * bit is parameterized, with a default reset value of "0".
                            *    0 Enable DSPI clocks.
                            *    1 Allow external logic to disable DSPI clocks. */
   unsigned doze:    1;    /* Provides support for an externally controlled Doze mode power-saving mechanism.
                            *    0 Doze mode has no effect on DSPI.
                            *    1 Doze mode disables DSPI. */
   unsigned pcsis:   6;    /* Peripheral Chip Select x Inactive State Determines the inactive state of PCSx.
                            *    0 The inactive state of PCSx is low.
                            *    1 The inactive state of PCSx is high. */
   unsigned rsvd2:   2;    /* Not used, all zeros. */
   unsigned rooe:    1;    /* Receive FIFO Overflow Overwrite Enable
                            * In the RX FIFO overflow condition, configures the DSPI to ignore the incoming serial data
                            * or overwrite existing data. If the RX FIFO is full and new data is received, the data from
                            * the transfer, generating the overflow, is ignored or shifted into the shift register.
                            *    0 Incoming data is ignored.
                            *    1 Incoming data is shifted into the shift register. */
   unsigned pcsse:   1;    /* Peripheral Chip Select Strobe Enable
                            * Enables the PCS[5]/ PCSS to operate as a PCS Strobe output signal.
                            *    0 PCS[5]/PCSS is used as the Peripheral Chip Select[5] signal.
                            *    1 PCS[5]/PCSS is used as an active-low PCS Strobe signal. */
   unsigned mtfe:    1;    /* Modified Timing Format Enable.  Enables a modified transfer format to be used.
                            *    0 Modified SPI transfer format disabled.
                            *    1 Modified SPI transfer format enabled. */
   unsigned frz:     1;    /* Freeze, Enables the DSPI transfers to be stopped on the next frame boundary when the
                            * device enters Debug mode.
                            *    0 Do not halt serial transfers in debug mode.
                            *    1 Halt serial transfers in debug mode. */
   unsigned dconf:   2;    /* DSPI Configuration - Selects among the different configurations of the DSPI.
                            *    00 SPI
                            *    01 Reserved
                            *    10 Reserved
                            *    11 Reserved */
   unsigned contScke: 1;   /* Continuous SCK Enable - Enables the Serial Communication Clock (SCK) to run continuously.
                            *    0 Continuous SCK disabled.
                            *    1 Continuous SCK enabled. */
   unsigned mstr:    1;    /* Master/Slave Mode Select - Configures the DSPI for either master mode or slave mode.
                            *    0 DSPI is in slave mode.
                            *    1 DSPI is in master mode. */
}spiMcr_t;                 /* DSPI Module Configuration Register (SPIx_MCR) */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiTcr_t - DSPI Transfer Count Register (SPIx_TCR)">
typedef struct
{
   unsigned rsvd:    16;   /* reserved - always 0 */
   unsigned spiTcnt: 16;   /* SPI Transfer Counter - Counts the number of SPI transfers the DSPI makes. The SPI_TCNT
                            * field increments every time the last bit of a SPI frame is transmitted. A value written
                            * to SPI_TCNT presets the counter to that value. SPI_TCNT is reset to zero at the beginning
                            * of the frame when the CTCNT field is set in the executing SPI command. The Transfer
                            * Counter wraps around; incrementing the counter past 65535 resets the counter to zero. */
} spiTcr_t;                 /* DSPI Transfer Count Register (SPIx_TCR) */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiCtar_t - DSPI Clock/Transfer Attributes (Master Mode) (SPIx_CTARn)">
typedef struct
{
   unsigned br:   4;    /* Baud Rate Scaler - Selects the scaler value for the baud rate. This field is used only in
                         * master mode. The prescaled system clock is divided by the Baud Rate Scaler to generate the
                         * frequency of the SCK. The baud rate is computed according to the following equation:
                         * SCK baud rate = (fSYS/PBR) x [(1+DBR)/BR]
                         * The following table lists the baud rate scaler values.
                         *    0000 = 2, 0001 = 4, 00010 = 6, 0011 = 8, 0100 = 16, etc... See datasheet. */
   unsigned dt:   4;    /* Delay After Transfer Scaler - Selects the Delay after Transfer Scaler. This field is used
                         * only in master mode. The Delay after Transfer is the time between the negation of the PCS
                         * signal at the end of a frame and the assertion of PCS at the beginning of the next frame. In
                         * the Continuous Serial Communications Clock operation, the DT value is fixed to one SCK clock
                         * period, The Delay after Transfer is a multiple of the system clock period, and it is computed
                         * according to the following equation: tDT = (1/fSYS) x PDT x DT
                         * See Delay Scaler Encoding table in CTARn[CSSCK] bit field description for scaler values. */
   unsigned asc:  4;    /* After SCK Delay Scaler - Selects the scaler value for the After SCK Delay. This field is used
                         * only in master mode. The After SCK Delay is the delay between the last edge of SCK and the
                         * negation of PCS. The delay is a multiple of the system clock period, and it is computed
                         * according to the following equation: tASC = (1/fSYS) x PASC x ASC
                         * See Delay Scaler Encoding table in CTARn[CSSCK] bit field description for scaler values.
                         * Refer After SCK Delay (tASC) for more details. */
   unsigned cssck: 4;   /* PCS to SCK Delay Scaler - Selects the scaler value for the PCS to SCK delay. This field is
                         * used only in master mode. The PCS to SCK Delay is the delay between the assertion of PCS and
                         * the first edge of the SCK. The delay is a multiple of the system clock period, and it is
                         * computed according to the following equation: tCSC = (1/fSYS) x PCSSCK x CSSCK
                         * The following table lists the delay scaler values.
                         * 0000 = 2, 0001 = 4, 00010 = 6, 0011 = 8, 0100 = 16, etc... See datasheet. */
   unsigned pbr:  2;    /* Baud Rate Prescaler - Selects the prescaler value for the baud rate. This field is used only
                         * in master mode. The baud rate is the frequency of the SCK. The system clock is divided by the
                         * prescaler value before the baud rate selection takes place. See the BR field description for
                         * details on how to compute the baud rate.
                         *    00 Baud Rate Prescaler value is 2.
                         *    01 Baud Rate Prescaler value is 3.
                         *    10 Baud Rate Prescaler value is 5.
                         *    11 Baud Rate Prescaler value is 7. */
   unsigned pdt:  2;    /* Delay after Transfer Prescaler - Selects the prescaler value for the delay between the
                         * negation of the PCS signal at the end of a frame and the assertion of PCS at the beginning of
                         * the next frame. The PDT field is only used in master mode. See the DT field description for
                         * details on how to compute the Delay after Transfer. Refer Delay after Transfer (tDT) for more
                         * details.
                         *    00 Delay after Transfer Prescaler value is 1.
                         *    01 Delay after Transfer Prescaler value is 3.
                         *    10 Delay after Transfer Prescaler value is 5.
                         *    11 Delay after Transfer Prescaler value is 7. */
   unsigned pasc: 2;    /* After SCK Delay Prescaler - Selects the prescaler value for the delay between the last edge
                         * of SCK and the negation of PCS. See the ASC field description for information on how to
                         * compute the After SCK Delay.Refer After SCK Delay (tASC) for more details.
                         *    00 Delay after Transfer Prescaler value is 1.
                         *    01 Delay after Transfer Prescaler value is 3.
                         *    10 Delay after Transfer Prescaler value is 5.
                         *    11 Delay after Transfer Prescaler value is 7. */
   unsigned pcssck: 2;  /* PCS to SCK Delay Prescaler - Selects the prescaler value for the delay between assertion of
                         * PCS and the first edge of the SCK. See the CSSCK field description for information on how to
                         * compute the PCS to SCK Delay. Refer PCS to SCK Delay (tCSC) for more details.
                         *    00 PCS to SCK Prescaler value is 1.
                         *    01 PCS to SCK Prescaler value is 3.
                         *    10 PCS to SCK Prescaler value is 5.
                         *    11 PCS to SCK Prescaler value is 7. */
   unsigned lsbfe: 1;   /* LBS First - Specifies whether the LSB or MSB of the frame is transferred first.
                         *    0 Data is transferred MSB first.
                         *    1 Data is transferred LSB first. */
   unsigned cpha: 1;    /* Clock Phase - Selects which edge of SCK causes data to change and which edge causes data to
                         * be captured. This bit is used in both master and slave mode. For successful communication
                         * between serial devices, the devices must have identical clock phase settings. In Continuous
                         * SCK mode, the bit value is ignored and the transfers are done as if the CPHA bit is set to 1.
                         *    0 Data is captured on the leading edge of SCK and changed on the following edge.
                         *    1 Data is changed on the leading edge of SCK and captured on the following edge. */
   unsigned cpol: 1;    /* Clock Polarity - Selects the inactive state of the Serial Communications Clock (SCK). This
                         * bit is used in both master and slave mode. For successful communication between serial
                         * devices, the devices must have identical clock polarities. When the Continuous Selection
                         * Format is selected, switching between clock polarities without stopping the DSPI can cause
                         * errors in the transfer due to the peripheral device interpreting the switch of clock polarity
                         * as a valid clock edge.
                         *    0 The inactive state value of SCK is low.
                         *    1 The inactive state value of SCK is high. */
   unsigned fmsz: 4;    /* Frame Size - The number of bits transferred per frame is equal to the FMSZ field value plus
                         * 1. The minimum valid FMSZ field value is 3. */
   unsigned dbr:  1;    /* Double Baud Rate - Doubles the effective baud rate of the Serial Communications Clock (SCK).
                         * This field is used only in master mode. It effectively halves the Baud Rate division ratio,
                         * supporting faster frequencies, and odd division ratios for the Serial Communications Clock
                         * (SCK). When the DBR bit is set, the duty cycle of the Serial Communications Clock (SCK)
                         * depends on the value in the Baud Rate Prescaler and the Clock Phase bit as listed in the
                         * following table. See the BR field description for details on how to compute the baud rate. */
} spiCtar_t;    /* DSPI Clock and Transfer Attributes Register (In Master Mode) (SPIx_CTARn) */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiSr_t - DSPI Status Register (SPIx_SR)">
typedef struct
{
   unsigned popnxtptr: 4;/* Pop Next Pointer - Contains a pointer to the RX FIFO entry to be returned when the POPR is
                         * read. The POPNXTPTR is updated when the POPR is read. */
   unsigned rxctr:   4; /* RX FIFO Counter - Indicates the number of entries in the RX FIFO. The RXCTR is decremented
                         * every time the POPR is read. The RXCTR is incremented every time data is transferred from the
                         * shift register to the RX FIFO. */
   unsigned txnxtptr: 4;/* Transmit Next Pointer - Indicates which TX FIFO Entry is transmitted during the next
                         * transfer. The TXNXTPTR field is updated every time SPI data is transferred from the TX FIFO
                         * to the shift register. */
   unsigned txctr:   4; /* TX FIFO Counter - Indicates the number of valid entries in the TX FIFO. The TXCTR is
                         * incremented every time the PUSHR is written. The TXCTR is decremented every time a SPI
                         * command is executed and the SPI data is transferred to the shift register. */
   unsigned rsvd:    1;
   unsigned rfdf:    1; /* Receive FIFO Drain Flag - Provides a method for the DSPI to request that entries be removed
                         * from the RX FIFO. The bit is set while the RX FIFO is not empty. The RFDF bit can be cleared
                         * by writing 1 to it or by acknowledgement from DMA controller when the RX FIFO is empty.
                         *    0 Rx FIFO is empty.
                         *    1 Rx FIFO is not empty. */
   unsigned rsvd2:   1;
   unsigned rfof:    1; /* Receive FIFO Overflow Flag - Indicates an overflow condition in the RX FIFO. The bit is set
                         * when the RX FIFO and shift register are full and a transfer is initiated. The bit remains set
                         * until it is cleared by writing a 1 to it.
                         *    0 No Rx FIFO overflow.
                         *    1 Rx FIFO overflow has occurred. */
   unsigned rsvd3:   5;
   unsigned tfff:    1; /* Transmit FIFO Fill Flag - Provides a method for the DSPI to request more entries to be added
                         * to the TX FIFO. The TFFF bit is set while the TX FIFO is not full. The TFFF bit can be
                         * cleared by writing 1 to it or by acknowledgement from the DMA controller to the TX FIFO full
                         * request. The Reset Value of this bit is 0 if MCR[MDIS] = 1. The Reset Value of this bit is 1
                         * if MCR[MDIS] = 0.
                         *    0 Tx FIFO is full.
                         *    1 Tx FIFO is not full */
   unsigned rsvd4:   1;
   unsigned tfuf:    1; /* Transmit FIFO Underflow Flag - Indicates an underflow condition in the TX FIFO. The transmit
                         * underflow condition is detected only for DSPI blocks operating in slave mode and SPI
                         * configuration. TFUF is set when the TX FIFO of a DSPI operating in SPI slave mode is empty
                         * and an external SPI master initiates a transfer. The TFUF bit remains set until cleared by
                         * writing 1 to it.
                         *    0 No Tx FIFO underflow.
                         *    1 Tx FIFO underflow has occurred. */
   unsigned eoqf:    1; /* End of Queue Flag - Indicates that the last entry in a queue has been transmitted when the
                         * DSPI is in master mode. The EOQF bit is set when the TX FIFO entry has the EOQ bit set in the
                         * command halfword and the end of the transfer is reached. The EOQF bit remains set until
                         * cleared by writing a 1 to it. When the EOQF bit is set, the TXRXS bit is automatically
                         * cleared.
                         *    0 EOQ is not set in the executing command.
                         *    1 EOQ is set in the executing SPI command. */
   unsigned rsvd5:   1;
   unsigned txrxs:   1; /* TX and RX Status - Reflects the run status of the DSPI.
                         *    0 Transmit and receive operations are disabled (DSPI is in stopped state).
                         *    1 Transmit and receive operations are enabled (DSPI is in running state). */
   unsigned tcf:     1; /* Transfer Complete Flag - Indicates that all bits in a frame have been shifted out. TCF
                         * remains set until it is cleared by writing a 1 to it.
                         *    0 Transfer not complete.
                         *    1 Transfer complete. */
} spiSr_t;               /* DSPI Status Register (SPIx_SR) */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiRser_t - DSPI DMA/IRQ Select and Enable Register (SPIx_RSER)">
typedef struct
{
   unsigned rsvd:       16;
   unsigned rfdfDirs:   1; /* Receive FIFO Drain DMA or Interrupt Request Select. Selects between generating a DMA
                            * request or an interrupt request. When the RFDF flag bit in the SR is set, and the RFDF_RE
                            * bit in the RSER is set, the RFDF_DIRS bit selects between generating an interrupt request
                            * or a DMA request.
                            *    0 Interrupt request.
                            *    1 DMA request. */
   unsigned rfdfRe:     1; /* Receive FIFO Drain Request Enable - Enables the RFDF flag in the SR to generate a request.
                            * The RFDF_DIRS bit selects between generating an interrupt request or a DMA request.
                            *    0 RFDF interrupt or DMA requests are disabled
                            *    1 RFDF interrupt or DMA requests are enabled */
   unsigned rsvd2:      1;
   unsigned rfofRe:     1; /* Receive FIFO Overflow Request Enable - Enables the RFOF flag in the SR to generate an
                            * interrupt request.
                            *    0 RFOF interrupt requests are disabled.
                            *    1 RFOF interrupt requests are enabled. */
   unsigned rsvd3:      4;
   unsigned tfffDirs:   1; /* Transmit FIFO Fill DMA or Interrupt Request Select - Selects between generating a DMA
                            * request or an interrupt request. When the TFFF flag bit in the SR is set and the TFFF_RE
                            * bit in the RSER register is set, this bit selects between generating an interrupt request
                            * or a DMA request.
                            *    0 TFFF flag generates interrupt requests.
                            *    1 TFFF flag generates DMA requests. */
   unsigned tfffRe:     1; /* Transmit FIFO Fill Request Enable - Enables the TFFF flag in the SR to generate a request.
                            * The TFFF_DIRS bit selects between generating an interrupt request or a DMA request.
                            *    0 TFFF interrupts or DMA requests are disabled.
                            *    1 TFFF interrupts or DMA requests are enabled. */
   unsigned rsvd4:      1; /* Transmit FIFO Underflow Request Enable - Enables the TFUF flag in the SR to generate an
                            * interrupt request.
                            *    0 TFUF interrupt requests are disabled.
                            *    1 TFUF interrupt requests are enabled. */
   unsigned tfufRe:     1; /* Transmit FIFO Underflow Request Enable - Enables the TFUF flag in the SR to generate an
                            * interrupt request.
                            * 0 TFUF interrupt requests are disabled.
                            * 1 TFUF interrupt requests are enabled. */
   unsigned eoqfRe:     1; /* DSPI Finished Request Enable - Enables the EOQF flag in the SR to generate an interrupt
                            * request.
                            *    0 EOQF interrupt requests are disabled.
                            *    1 EOQF interrupt requests are enabled. */
   unsigned rsvd5:      2;
   unsigned tcfRe:      1; /* Transmission Complete Request Enable - Enables TCF flag in the SR to generate an interrupt
                            * request.
                            *    0 TCF interrupt requests are disabled.
                            *    1 TCF interrupt requests are enabled. */
} spiRser_t;                /* DSPI DMA/Interrupt Request Select and Enable Register (SPIx_RSER) */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiPushr_t - DSPI PUSH TX FIFO Register In Master Mode (SPIx_PUSHR)">
typedef struct
{
   unsigned txdata:  16; /* Transmit Data - Holds SPI data to be transferred according to the associated SPI command. */
   unsigned pcs:     6;  /* Select which PCS signals are to be asserted for the transfer. Refer to the chip
                          * configuration chapter for the number of PCS signals used in this MCU.
                          *   0 Negate the PCS[x] signal.
                          *   1 Assert the PCS[x] signal. */
   unsigned rsvd:    4;
   unsigned ctcnt:   1;  /* Clear Transfer Counter. - Clears the SPI_TCNT field in the TCR register. The SPI_TCNT field
                          * is cleared before the DSPI starts transmitting the current SPI frame.
                          *   0 Do not clear the TCR[SPI_TCNT] field.
                          *   1 Clear the TCR[SPI_TCNT] field. */
   unsigned eoq:     1;  /* End Of Queue - Host software uses this bit to signal to the DSPI that the current SPI
                          * transfer is the last in a queue. At the end of the transfer, the EOQF bit in the SR is set.
                          *   0 The SPI data is not the last data to transfer.
                          *   1 The SPI data is the last data to transfer. */
   unsigned ctas:    3;  /* Clock and Transfer Attributes Select. - Selects which CTAR register to use in master mode to
                          * specify the transfer attributes for the associated SPI frame. In SPI slave mode, CTAR0 is
                          * used. See the Chip Configuration chapter to determine how many CTAR registers this device
                          * has. You should not program a value in this field for a register that is not present.
                          *   000 CTAR0
                          *   001 CTAR1
                          *   010 - 111 - Reserved */
   unsigned cont:    1;  /* Continuous Peripheral Chip Select Enable - Selects a Continuous Selection Format. The bit is
                          * used in SPI master mode. The bit enables the selected PCS signals to remain asserted between
                          * transfers.
                          *   0 Return PCSn signals to their inactive state between transfers.
                          *   1 Keep PCSn signals asserted between transfers. */
} spiPushr_t;            /* DSPI PUSH TX FIFO Register In Master Mode (SPIx_PUSHR) */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiPort_t - SPI SFRs in the order they appear in the memory map.">
typedef struct /* Allows for bit access to the SFRs needed to control the master spi port. */
{
   union
   {
      spiMcr_t bits;
      uint32_t   reg;
   } mcr;                /* DSPI Module Configuration Register (SPI0_MCR) */
   uint32_t   rsvd;
   union
   {
      spiTcr_t bits;
      uint32_t   reg;
   } tcr;                /* DSPI Transfer Count Register (SPI0_TCR) */
   union
   {
      spiCtar_t bits;
      uint32_t    reg;
   } ctar0;              /* DSPI Clock and Transfer Attributes Register (In Master Mode) (SPI0_CTAR0) */
   union
   {
      spiCtar_t bits;
      uint32_t    reg;
   } ctar1;              /* DSPI Clock and Transfer Attributes Register (In Master Mode) (SPI0_CTAR0) */
   uint32_t rsvd2[6];
   union
   {
      spiSr_t bits;
      uint32_t  reg;
   } sr;                 /* DSPI Status Register (SPI0_SR) */
   union
   {
      spiRser_t bits;
      uint32_t  reg;
   } rser;               /* DSPI DMA/Interrupt Request Select and Enable Register (SPI0_RSER) */
   union
   {
      spiPushr_t bits;
      uint32_t  reg;
   } pushr;             /* DSPI PUSH TX FIFO Register In Master Mode (SPI0_PUSHR) */
   uint32_t   popr;     /* DSPI POP RX FIFO Register (SPI0_POPR) */
   uint32_t   txfr0;    /* DSPI Transmit FIFO Registers (SPI0_TXFR0) */
   uint32_t   txfr1;    /* DSPI Transmit FIFO Registers (SPI0_TXFR1) */
   uint32_t   txfr2;    /* DSPI Transmit FIFO Registers (SPI0_TXFR2) */
   uint32_t   txfr3;    /* DSPI Transmit FIFO Registers (SPI0_TXFR3) */
   uint32_t   rsvd3[12];
   uint32_t   rxfr0;    /* DSPI Receive FIFO Registers (SPI0_RXFR0) */
   uint32_t   rxfr1;    /* DSPI Receive FIFO Registers (SPI0_RXFR1) */
   uint32_t   rxfr2;    /* DSPI Receive FIFO Registers (SPI0_RXFR2) */
   uint32_t   rxfr3;    /* DSPI Receive FIFO Registers (SPI0_RXFR3) */
} spiPort_t; /* SPI SFRs in the order they appear in the memory map. */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="sysClkGateCtrlWithMask_t - Used to set the clock for SPI port x">
typedef struct
{
   uint32_t volatile *pSysClkGateCtrl;  /* Pointer to Sys Clk Gate Ctrl Register */
   uint32_t mask;                       /* Mask to apply to the Gate Ctrl Register */
} sysClkGateCtrlWithMask_t;             /* Used to set the clock for SPI port x */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiPinCtrlCfg_t - SPI Pin control configuration">
typedef struct
{
   uint32_t volatile *pPinCtrlReg;      /* Pointer to PCR register */
   uint32_t value;                      /* Value to apply to the PCR regisetr. */
} spiPinCtrlCfg_t;                      /* SPI Pin control configuration */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiChPinCtrlCfg_t - Configures all pins of a channel of SPI">
typedef struct
{
   spiPinCtrlCfg_t clk;        /* Controls Clock */
   spiPinCtrlCfg_t mosi;       /* Controls MOSI */
   spiPinCtrlCfg_t miso;       /* Controls MISO */
   spiPinCtrlCfg_t cs;         /* Controls CS */
} spiChPinCtrlCfg_t;           /* Configures a channel of SPI */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="spiDmaIsrCfg_t - SPI DMA Configuration">
#if SPI_USES_DMA == 1
typedef struct
{
   IRQInterruptIndex isrTxIndx;     /* IRQ Interrupt Index, MK60F header. */
   IRQInterruptIndex isrRxIndx;     /* IRQ Interrupt Index, MK60F header. */
   INT_ISR_FPTR      isr;           /* ISR Function */
   uint8_t           pri;           /* Priority */
   uint8_t           subPri;        /* Sub Priority */
   uint8_t           txDmaCh;       /* DMA Channel */
   uint8_t           rxDmaCh;
   uint8_t           txDmaSrc;
   uint8_t           rxDmaSrc;
} spiDmaIsrCfg_t;                   /* SPI DMA Configuration */
#endif
// </editor-fold>

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#ifndef __BOOTLOADER
returnStatus_t SPI_initPort (uint8_t port);
void           SPI_SetFastSlewRate(uint8_t port, bool fastRate);
void           SPI_MutexLock(uint8_t port);
void           SPI_MutexUnlock(uint8_t port);
#endif
returnStatus_t SPI_OpenPort (uint8_t port, spiCfg_t const * const pCfg, bool master);
#ifndef __BOOTLOADER
void SPI_ChkSharedPortCfg   (uint8_t port, const spiCfg_t* const pCfg);
returnStatus_t SPI_ClosePort(uint8_t port);
returnStatus_t SPI_misoCfg  (uint8_t port, spiMisoCfg_e cfg );
#endif
returnStatus_t SPI_WritePort(uint8_t port, uint8_t const *pTxData, uint16_t txCnt);
returnStatus_t SPI_ReadPort (uint8_t port, uint8_t *pRxData, uint16_t rxCnt);
void SPI_RtosEnable(bool rtosEnable);
void SPI_pwrMode( ePowerMode powerMode); /* See notes on lower power mode in the function header, SPI_pwrMode(). */

#ifdef TM_SPI_DRIVER_UNIT_TEST
void SPI_UnitTest( void );
#endif

#endif
