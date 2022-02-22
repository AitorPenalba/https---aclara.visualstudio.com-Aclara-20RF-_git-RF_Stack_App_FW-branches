/***********************************************************************************************************************
 *
 * Filename: buffer.h
 *
 * Contents: Buffer function prototypes and typedefs.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2022 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ Created May 1, 2010
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - 5/1/2010 - Initial Release
 *
 * @version    0.1 - Ported from F1 electric
 * #since      2010-05-1
 **********************************************************************************************************************/

#ifndef _BUFFER_H_
#define _BUFFER_H_

#ifdef BM_GLOBAL
#define BM_EXTERN
#else
#define BM_EXTERN extern
#endif

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include "OS_aclara.h" // Include buffer_t to avoid compiler/linker headache
#include "DBG_SerialDebug.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define BM_USE_KERNEL_AWARE_DEBUGGING  0

/** Number of buffer pools (based on the number of pool definitions) */
#define BUFFER_N_POOLS        ((uint16_t)(sizeof(BM_bufferPoolParams) / sizeof(BM_bufferPoolParams[0])))

#define BM_alloc(x)      BM_Alloc(x, __FILE__, __LINE__)
#define BM_allocDebug(x) BM_AllocDebug(x, __FILE__, __LINE__)
#define BM_allocStack(x) BM_AllocStack(x, __FILE__, __LINE__)
#define BM_free(x)       BM_Free(x, __FILE__, __LINE__)

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef enum
{
   eBM_APP,          /* Allocate a buffer for application purposes */
   eBM_STACK,        /* Allocate a buffer for communication stack purposes */
   eBM_DEBUG         /* Allocate a buffer for debugging */

} eBM_BufferUsage_t; /* This will help guarantee that the application will not run out of buffers due to debugging */

typedef enum
{
   eBM_INT_RAM,    /* Allocate a buffer in internal RAM */
   eBM_EXT_RAM     /* Allocate a buffer in external RAM */
} eBM_BufferLoc_t; /* This will help guarantee that the application will not run out of buffers due to debugging */

/** Parameters for the creation and management of a buffer pool */
typedef struct
{
#if BM_USE_KERNEL_AWARE_DEBUGGING
   const char        *pName;  /**< pool name for kernel-aware debugging */
#endif
   uint16_t          size;    /**< size of data area */
   uint16_t          cnt;     /**< number of buffers */
   eBM_BufferUsage_t type;    /**< Buffer intended for application or debugging */
   eBM_BufferLoc_t   ramLoc;  /**< Location of Buffer */
} bufferPoolParams_t;

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

/**
 * Firmware configuration parameters for buffer pool creation and management.
 *
 * @warning The entries must be in ascending order by size.
 *
 * @Note: Buffer length size should be modulo 4. If not, space is wasted.
 *
 * @todo    Reevaluate the sizes and numbers of buffers, based on usage and on limits imposed by queues and limit
 *          semaphores.  Make sure '??' is adjusted in 'BM_bufferPoolParams[??]' if buffers are added or subtracted! */
#if ( DTLS_DEBUG == 1 )
BM_EXTERN const bufferPoolParams_t BM_bufferPoolParams[15]   /* Note:  Must change array size # if buf add or sub */
#else
BM_EXTERN const bufferPoolParams_t BM_bufferPoolParams[14]   /* Note:  Must change array size # if buf add or sub */
#endif
#ifdef BM_GLOBAL
=
   {
#if ( DCU == 1 )
#if BM_USE_KERNEL_AWARE_DEBUGGING
   /* name,           size, count, Type */
   { "qBuffer52",       52,   15, eBM_APP,       eBM_INT_RAM      },
   { "qBuffer104",     104,    8, eBM_APP,       eBM_INT_RAM      },
   { "qBuffer260",     260,    6, eBM_APP,       eBM_INT_RAM      },
   { "qBuffer1224",   1224,    3, eBM_APP,       eBM_INT_RAM      },
   { "qBufferS20",     20,    16, eBM_STACK,     eBM_EXT_RAM      },
   { "qBufferS156",    156, 2000, eBM_STACK,     eBM_EXT_RAM      },
   { "qBufferS260",    260,  500, eBM_STACK,     eBM_EXT_RAM      },
   { "qBufferS344",    344,   12, eBM_STACK,     eBM_EXT_RAM      },
   { "qBufferS516",    516,    6, eBM_STACK,     eBM_EXT_RAM      },
   { "qBufferS1320",  1320,   10, eBM_STACK,     eBM_EXT_RAM      },
   { "qBufferS1752",  1920,    3, eBM_STACK,     eBM_EXT_RAM      },
   { "qBufferD120",    120,  170, eBM_DEBUG,     eBM_EXT_RAM      }, // Most Debug prints on one line (less than 80 characters)
   { "qBufferD180",    180,    6, eBM_DEBUG,     eBM_EXT_RAM      },
   { "qBufferD400",    DEBUG_MSG_SIZE,    9, eBM_DEBUG,     eBM_EXT_RAM      }  // Maximum debug line size. Very rare; may be used by dtls and PHY and U? command
#else //No BM_USE_KERNEL_AWARE_DEBUGGING
   /* size, count, Buffer Type */
   {    52,    15, eBM_APP,   eBM_INT_RAM    },
   {   104,     8, eBM_APP,   eBM_INT_RAM    },
   {   260,     6, eBM_APP,   eBM_INT_RAM    },
   {  1224,     8, eBM_APP,   eBM_INT_RAM    },
   {    20,    16, eBM_STACK, eBM_INT_RAM    },
   {   156,  2000, eBM_STACK, eBM_EXT_RAM    },
   {   260,   500, eBM_STACK, eBM_EXT_RAM    },
   {   344,    12, eBM_STACK, eBM_INT_RAM    }, // DTLS ecc_point
   {   516,     6, eBM_STACK, eBM_EXT_RAM    },
   {  1320,    10, eBM_STACK, eBM_EXT_RAM    },
   {  1920,     6, eBM_STACK, eBM_EXT_RAM    },
#if ( DTLS_DEBUG == 1 )
   {    36,   100, eBM_DEBUG, eBM_EXT_RAM    }, // Many DTLS_DEBUG alloc/free messages fit this smaller size
#endif
   {   120,   170, eBM_DEBUG, eBM_EXT_RAM    }, // Most Debug prints on one line (less than 120 characters)
   {   192,    10, eBM_DEBUG, eBM_EXT_RAM    },
   {   DEBUG_MSG_SIZE,     9, eBM_DEBUG, eBM_EXT_RAM    }  // Maximum debug line size. Very rare; may be used by dtls and PHY and U? command
#endif

#elif ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A ) //K22
#if BM_USE_KERNEL_AWARE_DEBUGGING
   /* name,           size, count, Type */
   { "qBuffer52",       52,   15, eBM_APP,   eBM_INT_RAM      },
   { "qBuffer104",     104,    8, eBM_APP,   eBM_INT_RAM      },
   { "qBuffer260",     260,    6, eBM_APP,   eBM_INT_RAM      },
   { "qBuffer1024",   1024,    2, eBM_APP,   eBM_INT_RAM      },
   { "qBufferS20",     20,    16, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS156",    156,   20, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS260",    260,    8, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS344",    344,   12, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS516",    516,    6, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS1320",  1320,    4, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS1752",  1920,    3, eBM_STACK, eBM_INT_RAM      },
#if ( DTLS_DEBUG == 1 )
   {  "qBufferS1752",   36,  100, eBM_DEBUG, eBM_EXT_RAM    }, // Many DTLS_DEBUG alloc/free messages fit this smaller size
#endif
   { "qBufferD120",    120,   40, eBM_DEBUG, eBM_INT_RAM      },
   { "qBufferD192",    192,    5, eBM_DEBUG, eBM_INT_RAM      },
   { "qBufferD400",    DEBUG_MSG_SIZE,    3, eBM_DEBUG,     eBM_INT_RAM      }  // Maximum debug line size. Very rare; may be used by dtls and PHY and U? command
#else //No BM_USE_KERNEL_AWARE_DEBUGGING
   /* size, count, Buffer Type */
   {    52,    15, eBM_APP,   eBM_INT_RAM    },
   {   104,     8, eBM_APP,   eBM_INT_RAM    },
   {   260,     6, eBM_APP,   eBM_INT_RAM    },
   {  1024,     2, eBM_APP,   eBM_INT_RAM    },
   {    20,    16, eBM_STACK, eBM_INT_RAM    },
   {   156,    16, eBM_STACK, eBM_INT_RAM    },
   {   260,     8, eBM_STACK, eBM_INT_RAM    },
   {   344,    12, eBM_STACK, eBM_INT_RAM    }, // DTLS ecc_point
   {   516,     6, eBM_STACK, eBM_INT_RAM    },
   {  1320,     4, eBM_STACK, eBM_INT_RAM    },
   {  1920,     3, eBM_STACK, eBM_INT_RAM    },
#if ( DTLS_DEBUG == 1 )
   {    36,   100, eBM_DEBUG, eBM_EXT_RAM    }, // Many DTLS_DEBUG alloc/free messages fit this smaller size
#endif
   {   120,    20, eBM_DEBUG, eBM_INT_RAM    }, // Most Debug prints on one line (less than 120 characters)
   {   192,     5, eBM_DEBUG, eBM_INT_RAM    },
   {   DEBUG_MSG_SIZE,     3, eBM_DEBUG, eBM_INT_RAM    }  // Maximum debug line size. Very rare; may be used by dtls and PHY and U? command
#endif

#else //K24
#if BM_USE_KERNEL_AWARE_DEBUGGING
   /* name,           size, count, Type */
   { "qBuffer52",       52,   15, eBM_APP,   eBM_INT_RAM      },
   { "qBuffer104",     104,    8, eBM_APP,   eBM_INT_RAM      },
   { "qBuffer260",     260,    6, eBM_APP,   eBM_INT_RAM      },
   // This could be 1271 which is the largest application message with the smallest practicle network overhead
   { "qBuffer1224",   1224,    2, eBM_APP,   eBM_INT_RAM      },
   { "qBufferS20",     20,    16, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS156",    156,   20, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS260",    260,    8, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS344",    344,   12, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS516",    516,    6, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS1320",  1320,    4, eBM_STACK, eBM_INT_RAM      },
   { "qBufferS1752",  1920,    3, eBM_STACK, eBM_INT_RAM      },
#if ( DTLS_DEBUG == 1 )
   { "qBufferS1752",    36,  100, eBM_DEBUG, eBM_EXT_RAM    }, // Many DTLS_DEBUG alloc/free messages fit this smaller size
#endif
   { "qBufferD120",    120,   40, eBM_DEBUG, eBM_INT_RAM      },
   { "qBufferD192",    192,    5, eBM_DEBUG, eBM_INT_RAM      },
   { "qBufferD400",    DEBUG_MSG_SIZE,    3, eBM_DEBUG,     eBM_INT_RAM      }  // Maximum debug line size. Very rare; may be used by dtls and PHY and U? command
#else //No BM_USE_KERNEL_AWARE_DEBUGGING
   /* size, count, Buffer Type */
   {    52,    30, eBM_APP,   eBM_INT_RAM    },
   {   104,    16, eBM_APP,   eBM_INT_RAM    },
   {   260,    12, eBM_APP,   eBM_INT_RAM    },
   // This could be 1271 which is the largest application message with the smallest practicle network overhead
   {  1224,     8, eBM_APP,   eBM_INT_RAM    },
   {    20,    16, eBM_STACK, eBM_INT_RAM    },
   {   156,    32, eBM_STACK, eBM_INT_RAM    },
   {   260,    16, eBM_STACK, eBM_INT_RAM    },
   {   344,    24, eBM_STACK, eBM_INT_RAM    }, // DTLS ecc_point
   {   516,    12, eBM_STACK, eBM_INT_RAM    },
   {  1320,     8, eBM_STACK, eBM_INT_RAM    },
   {  1920,     6, eBM_STACK, eBM_INT_RAM    },
#if ( DTLS_DEBUG == 1 )
   {    36,   100, eBM_DEBUG, eBM_EXT_RAM    }, // Many DTLS_DEBUG alloc/free messages fit this smaller size
#endif
   {   120,    40, eBM_DEBUG, eBM_INT_RAM    }, // Most Debug prints on one line (less than 120 characters)
   {   192,    10, eBM_DEBUG, eBM_INT_RAM    },
   {   DEBUG_MSG_SIZE,     6, eBM_DEBUG, eBM_INT_RAM    }  // Maximum debug line size. Very rare; may be used by dtls and PHY and U? command
#endif
#endif
}
#endif
;

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS (Continued) */

/** Generic buffer structure (actual data size determined on allocation).
 *
 *  Application code that uses this facility may use these fields as follows:
 *    - bufMaxSize:  read-only  - indicates allocated (max) size of data field
 *    - data:        read/write - application stores data here
 *  The other fields should be consider as private and should not be accessed.
 */
typedef struct
{
#if 0 // TODO: RA6E1 - Queue support
    OS_QUEUE_Element x;            /*!< ! Must be here for MQX to use this buffer in messages. */
#endif
    eSysFormat_t     eSysFmt;      /**< User filled - This is the format of the buffer */
    uint16_t         bufMaxSize;   /**< physical size of the data area */
    const char       *pfile;       /* Pointer to file name that allocated the buffer     */
    uint32_t         line;         /* Line number in the file that allocated the buffer  */
    /* Make sure data is on 32-bit boundary */
    uint8_t          *data;        /**< user data area - sized at allocation.  data MUST align on a native boundary */
}buffer_t;

typedef struct
{
   uint32_t allocOk;             /**< Count of successful allocations */
   uint32_t freeOk;              /**< Count of successful free operations */
   uint32_t allocFail;           /**< Count of failed allocations */
   uint32_t freeFail;            /**< Count of failed free operations */
   uint32_t currAlloc;           /**< Count of current number of allocations */
   uint32_t highwater;           /**< Max number of simultaneous allocations */
} bufferPoolStats_t;

/**
 * Buffer allocation statistics.
 *
 * The per-pool statistics track the usage of each buffer pool; the other statistics track buffer allocation as a whole.
 * Ideally, all of the failure/error counts will remain zero.
 *
 * The per-pool allocFail counts will increment if a pool was the appropriate size but was empty, even if the allocation
 * was eventually performed from a pool with larger buffers.  This is not necessarily an error, although perhaps the
 * pool sizing should be reconsidered.
 *
 * Non-zero counts for any of the free error counters - especially the double-free-attempt counter - indicate serious
 * software errors.
 */
typedef struct
{
   uint32_t            allocFailApp;         /**< Total failed allocs for Application */
   uint32_t            allocFailStack;       /**< Total failed allocs for Stack */
   uint32_t            allocFailDebug;       /**< Total failed allocs for Debugging */
   uint32_t            freeFail;             /**< Total failed frees */
   uint32_t            freeDoubleFree;       /**< Total double-free attempts */
   bufferPoolStats_t   pool[BUFFER_N_POOLS]; /**< per-pool statistics */
} bufferStats_t;

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */
BM_EXTERN bufferStats_t BM_bufferStats;      /* buffer allocation statistics */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

#ifdef ERROR_CODES_H_
returnStatus_t BM_init(void);
#endif
buffer_t       *BM_Alloc(uint16_t minSize, const char *file, int line);
buffer_t       *BM_AllocDebug( uint16_t minSize, const char *file, int line );
buffer_t       *BM_AllocStack( uint16_t minSize, const char *file, int line );
#ifdef WOLFSSL_DEBUG_MEMORY
void *BM_SpecLibMalloc( size_t reqSize, const char *func, int32_t line );
#else
void           *BM_SpecLibMalloc( size_t reqSize );
#endif
void           BM_SpecLibFree( void *ptr );
void           BM_AllocStatic( buffer_t *ptr, eSysFormat_t sysFormat );
void           BM_Free(buffer_t *pBuf, const char *file, int line );
void           BM_getStats(bufferStats_t *pStats);
void           BM_resetStats( void );
void           BM_showAlloc( bool safePrint );
#undef BM_EXTERN

#endif /* !_BUFFER_H_ */
