/***********************************************************************************************************************
 *
 * Filename: error_codes.h
 *
 * Contents: This file defines the global enumerated error codes used in the project.
 *
 *           The global error codes are grouped into sections,     (e.g. System, Hardware, Driver, OS, ... )
 *           Each section is then divided into subsection          (e.g. Driver has SPI, I2C, UART).
 *           Finally each subsection has error code enumerated     ( eUART_RX_OVERFLOW, eUART_RX_PARITY, ...)
 *
 *           It is expected that this file will be modifed during the design and development phase of the project
 *           as new error codes required.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012-2020 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$ kdavlin Created Feb 16, 2011
 * v0.1 3/23/11 mkv - Fixed overlap of error codes (E_SMMU_TYPE and E_BUFFER_TYPE).
 *
 **********************************************************************************************************************/
#ifndef ERROR_CODES_H_
#define ERROR_CODES_H_

#ifdef error_codes_GLOBAL
   #define ERROR_CODES_EXTERN
#else
   #define ERROR_CODES_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/*! General Error Types */
#define E_SYS_TYPE      0x0000    /*!< System   Error  */
#define E_HW_TYPE       0x1000    /*!< Hardware Errors */
#define E_CFG_TYPE      0x2000    /*!< Configuration Errors */
#define E_DRIVER_TYPE   0x3000    /*!< Driver Errors          */
#define E_OS_TYPE       0x4000    /*!< OS            Errors   */
#define E_IPC_TYPE      0x5000    /*!< Interprocess Communications Errors */
#define E_APP_TYPE      0x6000    /*!< Application Errors */
#define E_PROTO_TYPE    0x7000    /*!< Protocol Errors */
#define E_HMC_TYPE      0x8000    /*!< Host Meter Error */
#define E_ILC_TYPE      0x9000    /*!< ILC Error */

/*! Driver Error Types */
#define E_PDCA_TYPE         (E_DRIVER_TYPE +  0x00)  /*!< PDCA   Errors  */
#define E_SPI_TYPE          (E_DRIVER_TYPE +  0x10)  /*!< SPI    Errors  */
#define E_I2C_TYPE          (E_DRIVER_TYPE +  0x20)  /*!< I2C    Errors  */
#define E_UART_TYPE         (E_DRIVER_TYPE +  0x30)  /*!< UART   Errors  */
#define E_FILEIO_TYPE       (E_DRIVER_TYPE +  0x40)  /*!< File IO  Errors  */
#define E_INTF_DEV_DRV_TYPE (E_DRIVER_TYPE +  0x50)  /*!< Interface Device Driver Errors  */
#define E_NV_DRV_TYPE       (E_DRIVER_TYPE +  0x60)  /*!< Interface Device Driver Errors  */
#define E_PGM_MEM_DRV_TYPE  (E_DRIVER_TYPE +  0x70)  /*!< Interface Device Driver Errors  */

// Where should this one be? Driver, OS, or other???
#define E_BUFFER_TYPE       (E_DRIVER_TYPE + 0x80)  /*!< Buffer Manager */

/*! OS Error Error Types */
#define E_OS_TASK_TYPE      (E_OS_TYPE     +     0)  /*!< OS Task       Errors */
#define E_OS_SEM_TYPE       (E_OS_TYPE     +  0x20)  /*!< OS Semaphore  Errors */
#define E_OS_MUTEX_TYPE     (E_OS_TYPE     +  0x40)  /*!< OS Mutex      Errors */
#define E_OS_QUE_TYPE       (E_OS_TYPE     +  0x50)  /*!< OS Queue      Errors */

/*! Application Error Types */
#define E_APP_GENERAL_TYPE  (E_APP_TYPE    +      0) /*!< Common App errors */
#define E_APP_METERING_TYPE (E_APP_TYPE    +  0x100) /*!< Metering Errors */
#define E_APP_DEMAND_TYPE   (E_APP_TYPE    +  0x200) /*!< Demand   Errors */
#define E_APP_INTERVAL_TYPE (E_APP_TYPE    +  0x300) /*!< Interval Data   Errors */

/*! Protocol Error Types */
#define E_PROTO_MERCURY_TYPE (E_PROTO_TYPE  +     0)  /*!< Mercury protocol Errors */


/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/*! Global Error Code Enumerations */
typedef enum
{
   /* ---------------------------------------------------------------------------------------------------------------- *
    * System Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eSUCCESS = E_SYS_TYPE,  /*!< No errors (e.g Successfull) */
   eFAILURE,               /*!< Generic  Error Code         */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Hardware Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eHARDWARE_FAILURE = E_HW_TYPE,  /*!< Generic hardware error */

  /* ---------------------------------------------------------------------------------------------------------------- *
    * Driver Errors
    * --------------------------------------------------------------------------------------------------------------- */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Programmable DMA Controller Errors
    * --------------------------------------------------------------------------------------------------------------- */
   ePDCA_INVALID_CHANNEL = E_PDCA_TYPE,  /*!< Invalid PDCA Channel */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * SPI Driver Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eSPI_INVALID_PORT = E_SPI_TYPE,  /*!< SPI - Invalid Port Id    */
   eSPI_INVALID_CS,                 /*!< SPI - Invalid chip select*/
   eSPI_INVALID_PARM,               /*!< SPI - Invalid Parameter  */
   eSPI_INVALID_CLK_RATE,           /*!< SPI - Invalid Clock Rate */
   eSPI_INVALID_DELAY,              /*!< SPI - Invalid Delay between TX or Clk */
   eSPI_INVALID_BITS_PER_TX,        /*!< SPI - Invalid number of bits to TX */
   eSPI_INVALID_CMD,                /*!< SPI - Invalid Command    */
   eSPI_INVALID_MODE,               /*!< SPI - Only Modes 0-3 are supported! */
   eSPI_TIME_OUT,                   /*!< SPI - Driver Timeout     */
   eSPI_BUSY,                       /*!< SPI - Port Busy          */
   eSPI_WRITE_FAIL,                 /*!< SPI - Write  Failed      */
   eSPI_READ_FAIL,                  /*!< SPI - Read   Failed      */
   eSPI_VERIFY_FAIL,                /*!< SPI - Verify Failed      */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * I2C Driver Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eI2C_INVALID_PORT = E_I2C_TYPE,  /*!< I2C - Invalid Port Id    */
   eI2C_INVALID_PARM,               /*!< I2C - Invalid Parameter  */
   eI2C_INVALID_CMD,                /*!< I2C - Invalid Command    */
   eI2C_TIME_OUT,                   /*!< I2C - Driver Timeout     */
   eI2C_BUSY,                       /*!< I2C - Port Busy          */
   eI2C_WRITE_FAIL,                 /*!< I2C - Write  Failed      */
   eI2C_READ_FAIL,                  /*!< I2C - Read   Failed      */
   eI2C_VERIFY_FAIL,                /*!< I2C - Verify Failed      */
   eI2C_WRITE_COL,                  /*!< I2C - Collision occured while writing */
   eI2C_WRITE_ACK,                  /*!< I2C - No ACK while writing */
   eI2C_READ_COL,                   /*!< I2C - Collision occured while reading */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * UART Driver Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eUART_CONFIG_ERR = E_UART_TYPE,  /*!< UART - Configuration Error        */
   eUART_RX_OVERFLOW,               /*!< UART - Receive buffer  overflow   */
   eUART_RX_PARITY,                 /*!< UART - Receive Parity  Error      */
   eUART_RX_FRAMING,                /*!< UART - Receive Framing Error      */
   eUART_TX_UNDERFLOW,              /*!< UART - Transmit buffer underflow  */
   eUART_TX_CHAR_TIME_OUT,          /*!< UART - Character time out, sending to queue  */
   eUART_TX_MSB_TIME_OUT,           /*!< UART - Message took too long to send, port error?  */
   eUART_RX_CHAR_TIME_OUT,          /*!< UART - character took too long to receive */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * FILE I/O Memory Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eFILEIO_INV_ADDRESS = E_FILEIO_TYPE, /*!< FILEIO - Invalid Address, Device Not Found. */
   eFILEIO_MEM_RANGE_ERROR,             /*!< FILEIO - Memory Range Error (Spanning?)   */
   eFILEIO_DVR_CMD_NOT_SUPPORTED,       /*!< FILEIO - Driver does not support the command */
   eFILEIO_FILE_CHECKSUM_ERROR,         /*!< FILEIO - File has a checksum failure */
   eFILEIO_FILE_CHCKSM_SIZE_ERROR,      /*!< FILEIO - File is too large to support a checksum */
   eFILEIO_FILE_SIZE_MISMATCH,          /*!< FILEIO - File found, but the size doesn't match. */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * FILE I/O Memory Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eINTF_DEV_DRV_ADDRESS_ERROR = E_INTF_DEV_DRV_TYPE, /*!< Interface Device Driver - Invalid Address. */

   /* ---------------------------------------------------------------------------------------------------------------- *
     * FILE I/O Memory Errors
     * -------------------------------------------------------------------------------------------------------------- */
   eNV_INTERNAL_NV_ERROR = E_NV_DRV_TYPE,   /*!< NV Driver - Internal NV memory RD/Wr Error */
   eNV_EXTERNAL_NV_ERROR,                   /*!< NV Driver - Internal NV memory RD/Wr Error */
   eNV_EXT_NV_ERASE_LOCKED,                 /*!< NV Driver - External NV memory erase locked Error */
   eNV_ADDRESS_ERROR,                       /*!< NV Driver - NV memory, Addr out of bounds Error */
   eNV_LEN_ERROR,                           /*!< NV Driver - NV memory, Req Len Error */
   eNV_BANKED_DRIVER_CFG_ERROR,             /*!< NV Driver - Banked driver configuration error. */
   ePAR_NOT_FOUND,                          /*!< Partition Manager - Partition name not found */

   /* ---------------------------------------------------------------------------------------------------------------- *
     * FILE I/O Memory Errors
     * -------------------------------------------------------------------------------------------------------------- */
   eNV_PGM_MEM_RD_ERROR = E_PGM_MEM_DRV_TYPE,   /*!< PRM Memory Driver - Program memory error (Internal flash memory) */
   eNV_PGM_MEM_WR_ERROR,
   eNV_PGM_MEM_INV_CNT_ERROR,
   eNV_PGM_MEM_PWR_ERROR,
   eNV_PGM_MEM_RANGE_ERROR,

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Memory Buffer Manager Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eBUFFER_CREATE_FAILED = E_BUFFER_TYPE, /*!< Could not create the memory buffer  */
   eBUFFER_HANDLE_INVALID,                /*!< Invalid buffer handle               */
   eBUFFER_ALLOC_FAILED,                  /*!< Buffer allocation failed, no buffers available */
   eBUFFER_FREE_FAILED,                   /*!< Buffer free failed, buffer already free  */
   eBUFFER_FREE_NULL,                     /*!< Buffer free failed, buffer was NULL      */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * OS Errors
    * --------------------------------------------------------------------------------------------------------------- */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Task Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eOS_TASK_CREATE_ERR = E_OS_TASK_TYPE,  /*!< Task Creation Failed     */
   eOS_TIMEOUT_ERR,

  /* ---------------------------------------------------------------------------------------------------------------- *
   * Semaphore Errors
   * ---------------------------------------------------------------------------------------------------------------- */
   eOS_SEM_CREATE_ERR = E_OS_SEM_TYPE,    /*!< Semaphore Create Failed  */
   eOS_SEM_HANDLE_INVALID,                /*!< Semaphore Invalid Handle */
   eOS_SEM_PEND_ERR,                      /*!< Semaphore Pend */
   eOS_SEM_POST_ERR,                      /*!< Semaphore Post */

  /* ---------------------------------------------------------------------------------------------------------------- *
   * Mutex Errors
   * ---------------------------------------------------------------------------------------------------------------- */
   eOS_MUTEX_CREATE_ERR = E_OS_MUTEX_TYPE, /*!< Mutex Creation Failed  */
   eOS_MUTEX_HANDLE_INVALID,               /*!< Mutex Invalid Handle   */
   eOS_MUTEX_LOCK_ERR,                     /*!< Mutex Lock Error       */
   eOS_MUTEX_UNLOCK_ERR,                   /*!< Mutex Unlock Error     */

  /* ---------------------------------------------------------------------------------------------------------------- *
   * Recursive Mutex Errors
   * ---------------------------------------------------------------------------------------------------------------- */
   eOS_RECURSIVE_MUTEX_CREATE_ERR = E_OS_MUTEX_TYPE, /*!< Mutex Creation Failed  */
   /* --------------------------------------------------------------------------------------------------------------- */
   eOS_RECURSIVE_MUTEX_HNDL_INV,                     /*!< Mutex Invalid Handle   */
   eOS_RECURSIVE_MUTEX_LOCK_ERR,                     /*!< Mutex Lock Error       */
   eOS_RECURSIVE_MUTEX_UNLOCK_ERR,                   /*!< Mutex Unlock Error     */

  /* ---------------------------------------------------------------------------------------------------------------- *
   * Queue Errors
   * ---------------------------------------------------------------------------------------------------------------- */
   eOS_QUE_CREATE_ERR = E_OS_QUE_TYPE,     /*!< Queue Creation Failed         */
   eOS_QUE_HANDLE_INVALID,                 /*!< Queue Invalid Handle          */
   eOS_QUE_SEND_ERR,                       /*!< General Queue send Error      */
   eOS_QUE_FULL_ERR,                       /*!< Queue Write (Send)    Error   */
   eOS_QUE_EMPTY_ERR,                      /*!< Queue Read  (Receive) Error   */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Interpocess Communications Error
    * --------------------------------------------------------------------------------------------------------------- */
   eIPC_ERR = E_IPC_TYPE,                  /*!< IPC Error                 */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Common Applications Errors
    * ************************************************************************ */
   eAPP_INVALID_CMD_CAT_CMD_CODE = E_APP_GENERAL_TYPE, /*!< Invalid cmd cat/code       */
   eAPP_CMD_CRC_REINIT,                                /*!< CRC Module Re-initialized  */
   eAPP_INVALID_CMD_CRC_NINT,                          /*!< CRC Module Not Initialized */
   eAPP_CP_CRC32_MISMATCH,                             /*!< CRC PGM mismatch           */
   eAPP_CHECKSUM_MISMATCH,                             /*!< Checksum mismatch          */
   eAPP_NOT_HANDLED,                                   /*!< Command not handled by this routine */
   eAPP_INVALID_VALUE,                                 /*!< Value specified not allowed for this variable */
   eAPP_INVALID_TYPECAST,                              /*!< Typecast specified not allowed for this variable */
   eAPP_READ_ONLY,                                     /*!< An attempt was made to write a read only value */
   eAPP_WRITE_ONLY,                                    /*!< An attempt was made to read a write only value */
   eAPP_OVERFLOW_CONDITION,                            /*!< An attempt was made to write a value larger than memory allocated */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Metering Application Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eAPP_METERING = E_APP_METERING_TYPE,    /*!< Metering Application Error    */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Demand Application Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eAPP_DEMAND = E_APP_DEMAND_TYPE,        /*!< Demand Applicaiton Error      */
   eAPP_DMD_RST_REQ_TIMEOUT,               /*!< Demand Reset Request has timed out due to no logout */
   eAPP_DMD_RST_MTR_LOCKOUT,               /*!< Demand Reset attempted while meter in lockout period*/

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Interval Data Application Errors
    * --------------------------------------------------------------------------------------------------------------- */
   eAPP_ID_CFG_CH = E_APP_INTERVAL_TYPE,     /*!< Channel is out of range - Applicaiton Error      */
   eAPP_ID_CFG_SAMPLE,                       /*!< Sample Rate is not correct - Applicaiton Error      */
   eAPP_ID_CFG_PARAM,                        /*!< One of the parameters are incorrect - Applicaiton Error      */
   eAPP_ID_REQ_FUTURE,                       /*!< Requested date is in the future - Applicaiton Error      */
   eAPP_ID_REQ_TIME_BOUNDARY,                /*!< Requested Time is not on a boundary - Applicaiton Error      */
   eAPP_ID_REQ_PAST,                         /*!< Requested date is too far in the past - Applicaiton Error      */
   /* ---------------------------------------------------------------------------------------------------------------- *
    * Protocol Error
    * --------------------------------------------------------------------------------------------------------------- */
   eMERCURY_INVALID_NWK_PROTO = E_PROTO_MERCURY_TYPE, /*!< Invalid NWK protocol */
   eMERCURY_INVALID_TR_PROTO,                 /*!< Invalid TR protocol */
   eMERCURY_INVALID_APP_PROTO,                /*!< Invalid APP protocol */
   eMERCURY_SERVICE_NOT_SUPPORTED,            /*!< SAP/MAP not supported */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * HMC Error
    * --------------------------------------------------------------------------------------------------------------- */
   eHMC_LP_INVALID_REQ_TIME = E_HMC_TYPE,             /*!< Invalid Time requested - In the future! */
   eHMC_LP_INVALID_REQ_TIME_RANGE,                    /*!< Invalid Time requested - Too far in the past */
   eHMC_LP_INVALID_REQ_QUANT,                         /*!< Invalid quanity requested - Quantity not in LP data */
   eHMC_LP_INVALID_TIME_BASE,                         /*!< Invalid Time Base requested - Time base != LP Time Base */
   eHMC_LP_MISSING_VALUE,                             /*!< LP Data is missing the data within in the tables.  */
   eHMC_TBL_READ_FAILURE,                             /*!< HMC failed to read - there was an error. */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * ILC Error
    * --------------------------------------------------------------------------------------------------------------- */
   eDRU_UNSUPPORTED_REGISTER_ERROR = E_ILC_TYPE,      /* Invalid DRU Register to be read */
   eDRU_NOT_PROCCESSED,                               /* The processing of the request for DRU Driver was not completed */
   eDRU_NACK_RESPONSE,                                /* The DRU responded with a NACK */
   eDRU_UNSUPPORTED_OPERATION_ERROR,                  /* Invalid operation request received */
   eDRU_COMMUNICATION_TIMED_OUT,                      /* The DRU did not responded within the expected time out time */

   /* ---------------------------------------------------------------------------------------------------------------- *
    * HMC Program Meter Error
    * --------------------------------------------------------------------------------------------------------------- */
//   eSUCCESS,  // All passed
//   eFAILURE,  // Command failed
   ePRG_MTR_SECTION_NOT_TESTABLE,
   ePRG_MTR_INTERNAL_ERROR,
   ePRG_MTR_FILE_FORMAT_ERROR,
   ePRG_MTR_METER_COMM_FAILURE,

   /* ---------------------------------------------------------------------------------------------------------------- *
    * Last status, this should be the last entry
    * --------------------------------------------------------------------------------------------------------------- */
   eLAST_RETURN_STATUS

}returnStatus_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#undef ERROR_CODES_EXTERN

#endif

