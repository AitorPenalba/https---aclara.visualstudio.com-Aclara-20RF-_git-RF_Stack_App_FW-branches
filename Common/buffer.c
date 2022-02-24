/**********************************************************************************************************************

   Filename:   buffer.c

   Global Designator: BM_  (buffer manager)

   Contents: Utility routines for buffer management.

   Notes:

   These routines support the allocation and deallocation of buffers used for network communication.  These routines are
   non-blocking and can be used from any task or low-priority ISR (interrupt priorities at or below the
   configMAX_SYSCALL_INTERRUPT_PRIORITY level).  Multiple buffer sizes are supported and the utility will allocate the
   smallest available buffer size that satisfies the allocation request.

   The buffers include meta-data in a header that describes the buffer, including its source buffer pool and the maximum
   size of its data area.  (This header may be expanded if system requirements are added that require additional
   meta-data to be maintained for *all* buffers, otherwise the extra meta-data should be placed in the data payload
   area.)

   The free buffer pools are implemented using FreeRTOS queues to hold pointers to free buffers.  This leverages the
   existing queue mechanism to support re-entrancy of these routines.

   The utility also implements logic to detect attempts to free a buffer that is already free.  This is done even if the
   statistics option is disabled, although a separate statistics counter can be maintained for this software fault.
   Unfortunately, it is not practical to detect the complementary case where a buffer is allocated but ever freed.

   @todo    Reconsider the strategy of allocating larger buffers if the proper buffer size is not available.  If there
            is a large difference in buffer sizes (e.g., 48 bytes vs. 256 bytes) then it may be better to fail an
            allocation request rather than consume a more limited larger buffer resource.  Alternately, the strategy
            could be modified to consider only one size larger.


 ***********************************************************************************************************************
   A product of Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2004-2020 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************

   @author Karl Davlin

   $Log$ Created May 1, 2010

 ***********************************************************************************************************************
   Revision History:
   v0.1 - 5/1/2010 - Initial Release

   @version    0.1 - Ported from F1 electric
   #since      2010-05-1
 **********************************************************************************************************************/
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include "project.h"
#include <stdlib.h>
#include <bsp.h>
#include "DBG_SerialDebug.h"
#include "user_settings.h"

#define BM_GLOBAL
#include "buffer.h"
#undef  BM_GLOBAL

#include "EVL_event_log.h"

/* ****************************************************************************************************************** */
/* LINT SUPPRESSIONS */
//lint -esym(429, MessageData)    Custodial pointer has not been freed or returned
//lint -esym(429, QueueElement)   Custodial pointer has not been freed or returned

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
#define ALLOC_WATCHDOG_TIMEOUT 15 // Reset in 15 minutes if we can't get buffers
#if( RTOS_SELECTION == FREE_RTOS )
#define BUFFERS_NUM_MSGQ_ITEMS 10 //NRJ: TODO Figure out sizing
#else
#define BUFFERS_NUM_MSGQ_ITEMS 0 
#endif
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* This structure describes the memory returned to external libraries using the BM_alloc mechanism. Since the undelying buffer_t structure is not
   known to the library, it has no way of returning the buffer_t structure. This led to an often lengthy search to reinsert the returned buffer into
   the bufferPools_. The search was observed to be up to 30 seconds on the T-board, when using external memory. By keeping the buffer_t pointer with
   the data memory passed to the library, the search can be eliminated. The buffer_t pointer is inserted at the beginning of the memory given to the
   calling routine, but the address returned to the calling routine is bumped past this pointer. The retrieval then uses a negative offset to get the
   original buffer_t pointer.  */
/*lint -esym(751,specLibBuf_t, pspecLibBuf_t)   Not used */
typedef struct specLibBuffer
{
   buffer_t *pBuffer;            /* buffer_t pointer returned by BM_alloc  */
   void     *data;               /* void * pointer given to caller         */
} specLibBuf_t, *pspecLibBuf_t;


/* ****************************************************************************************************************** */
/* CONSTANTS  */

/** Buffer pool queue handles */
STATIC OS_MSGQ_Obj bufferPools_[BUFFER_N_POOLS];

/* ****************************************************************************************************************** */

static OS_MUTEX_Obj bufMutex_;                     /* Serialize access to the for status variable access. */
static uint32_t bMetaSize;                         // Size of buffer_t aligned on 4 bytes boundary
static buffer_t *pBufIntMeta = NULL;               // Location of the internal metadata buffer.
static uint32_t bIntMetaNbBuffers = 0;             // Number of internal metadata buffers.
#if ( DCU == 1 )
static buffer_t *pBufExtMeta = NULL;               // Location of the external metadata buffer.
static uint32_t bExtMetaNbBuffers = 0;             // Number of external metadata buffers.

/* The size of the external memory pool is based on the size and number of eBM_DEBUG buffers in BM_bufferPoolParams. */
static uint8_t      extMemPool[644000] @ "EXTERNAL_RAM" ; /*lint !e430*/
#endif
static OS_TICK_Struct AllocWatchDog[2];            // Keep track of the first time when run out of buffers.  Used as a form of watchdog.
                                                   // Assume that eBM_APP = 0 and eBM_STACK = 1.
#ifndef _lint
static_assert( eBM_APP == 0,   "Code relies on eBM_APP being 0" );
static_assert( eBM_STACK == 1, "Code relies on eBM_STACK being 0" );
#endif

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

static buffer_t *bufAlloc( uint16_t minSize, eBM_BufferUsage_t type, const char *file, int line );

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION DEFINITIONS */

/***********************************************************************************************************************

   Function name: BM_init

   Purpose: This routine allocates RTOS queues for the buffer pools and allocates the buffers from the system heap RAM.
            It also clears the buffer allocation statistics.

   Arguments: None

   Returns: returnStatus_t (eSUCCESS or eFAILURE)

   Side Effects: Heap is used

   Reentrant Code: No

   Notes:   This routine can be called only once from the system initialization task, and it must be called before any
            task or ISR attempts to allocate a buffer.

 ******************************************************************************************************************** */
returnStatus_t BM_init( void )
{
#if ( DCU == 1 )
   /* externMemIndex_ is static to make it simpler to size the external memory array. */
   static   uint32_t       externMemIndex_ = 0;     /* memory index into the external memory */
   buffer_t      *pExtMeta;            /* External metadata pointer */
#endif
   returnStatus_t retVal = eSUCCESS;   /* Return value */
   uint8_t        pool;                /* Used as an index through the memory pool */
   uint16_t       buf;                 /* Used as an index through each buffer  */
   buffer_t      *pIntMeta;            /* Internal metadata pointer */

   if ( OS_MUTEX_Create( &bufMutex_ ) == false )
   {
      //Create mutex failed
      retVal = eFAILURE;
   }
   else
   {
      ( void )memset( &BM_bufferStats, 0, sizeof( BM_bufferStats ) ); //clear buffer allocation statistics

      // Align each meta buffer on a 32 bit boundary
      bMetaSize = ( sizeof( buffer_t ) + 3 ) & 0xFFFFFFFC;

      for ( pool = 0; ( eSUCCESS == retVal ) && ( pool < BUFFER_N_POOLS ); pool++ )
      {
         if ( eBM_INT_RAM == BM_bufferPoolParams[pool].ramLoc )
         {
            bIntMetaNbBuffers += BM_bufferPoolParams[pool].cnt;
         }
#if ( DCU == 1 )
         else
         {
            bExtMetaNbBuffers += BM_bufferPoolParams[pool].cnt;
         }
#endif
      }
      // Allocate all internal buffer's metadata in one call
      pBufIntMeta = ( buffer_t * )malloc( bIntMetaNbBuffers * bMetaSize );

      // Allocate all external buffer's metadata in one call
      // Align on a 32 bit boundary
#if ( DCU == 1 )
      externMemIndex_ = ( externMemIndex_ + 3 ) & 0xFFFFFFFC;
      pBufExtMeta = ( buffer_t* )( void* )&extMemPool[externMemIndex_];
      // point to the end of metadata buffers
      externMemIndex_ += bExtMetaNbBuffers * bMetaSize;
#endif
      if ( NULL == pBufIntMeta )
      {
         // Cannot allocate heap memory for a buffer
         retVal = eFAILURE;
      }
      else
      {
         pIntMeta = pBufIntMeta;
#if ( DCU == 1 )
         pExtMeta = pBufExtMeta;
#endif
         // Allocate all buffer's data
         for ( pool = 0; ( eSUCCESS == retVal ) && ( pool < BUFFER_N_POOLS ); pool++ )
         {
            // Create a queue to hold the buffer pointers for this pool
            if ( OS_MSGQ_Create( &bufferPools_[pool], BUFFERS_NUM_MSGQ_ITEMS ) == false )
            {
               //Message queue creation failed
               return eFAILURE;
            }
            else
            {
               // Allocate all buffer's data
               // Align each buffer on a 32 bit boundary
               uint32_t bsize = ( BM_bufferPoolParams[pool].size + 3 ) & 0xFFFFFFFC;
               // Get the size of all buffers for a given pool.
               uint32_t allsize = bsize * BM_bufferPoolParams[pool].cnt;

               // Fill the queue with buffer pointers, allocating space from heap
               if ( eBM_INT_RAM == BM_bufferPoolParams[pool].ramLoc )
               {
                  // Location of the data part of the buffer.
                  uint8_t *pData = ( uint8_t * )malloc( allsize ); /*lint -esym(593,pData)*/

                  if ( NULL == pData )
                  {
                     //cannot allocate heap memory for a buffer
                     retVal = eFAILURE;
                  }
                  else
                  {
                     if ( ( ( uint32_t )pData < 0x20000000 ) && ( ( ( uint32_t )pData + allsize ) > 0x20000000 ) )
                     {
                        DBG_LW_printf( "BM_init: got buffer starting at 0x%08x, ending at: 0x%08x\n",
                                       ( uint32_t )pData, ( uint32_t )pData + allsize - 1  );
                     }

                     for ( buf = 0; ( eSUCCESS == retVal ) && ( buf < BM_bufferPoolParams[pool].cnt ); buf++ )
                     {
                        pIntMeta->bufMaxSize      = BM_bufferPoolParams[pool].size; //initialize buffer header fields
                        pIntMeta->x.bufPool       = pool;
                        pIntMeta->x.flag.isFree   = false;
                        pIntMeta->x.flag.isStatic = false;
                        pIntMeta->x.flag.inQueue  = 0;
                        pIntMeta->x.dataLen       = 0;
                        pIntMeta->data            = pData;
                        OS_MSGQ_POST( &bufferPools_[pool], pIntMeta, ( bool )false, __FILE__, __LINE__ ); // Function will not return if it fails
                        pIntMeta = ( buffer_t* )( void* )( ( ( uint8_t* )pIntMeta ) + bMetaSize );
                        pData += bsize;
                     }
                  }
               } /*lint +esym(593,pData)*/
#if ( DCU == 1 )
               else
               {
                  // Location of the data part of the buffer.
                  uint8_t *pData = &extMemPool[externMemIndex_];

                  if ( ( externMemIndex_ + allsize ) > sizeof( extMemPool ) ) /* Will the allocated buffer overrun? */
                  {
                     // Can't allocate memory for a buffer
                     DBG_LW_printf( "BM_init: not enough external RAM buffer space. Need at least %u bytes more\n",
                                    ( externMemIndex_ + allsize ) - sizeof( extMemPool ) );

                     retVal = eFAILURE;
                  }
                  else  /* The buffer will fit in the allocated area */
                  {
                     // Adjust index for next time around
                     externMemIndex_ += allsize;

                     for ( buf = 0; ( eSUCCESS == retVal ) && ( buf < BM_bufferPoolParams[pool].cnt ); buf++ )
                     {
                        pExtMeta->bufMaxSize      = BM_bufferPoolParams[pool].size; //initialize buffer header fields
                        pExtMeta->x.bufPool       = pool;
                        pExtMeta->x.flag.isFree   = false;
                        pExtMeta->x.flag.isStatic = false;
                        pExtMeta->x.flag.inQueue  = 0;
                        pExtMeta->x.dataLen       = 0;
                        pExtMeta->data            = pData;
                        OS_MSGQ_POST( &bufferPools_[pool], pExtMeta, ( bool )false, __FILE__, __LINE__ ); // Function will not return if it fails
                        pExtMeta = ( buffer_t* )( void* )( ( ( uint8_t* )pExtMeta ) + bMetaSize );
                        pData += bsize;
                     }
                  }
               }
#endif
            }  //lint !e429  !e593  Custodial pointer 'pBuf' (line 217) has not been freed or returned
         }
      }
   }
#if ( DCU == 1 )
   if ( retVal == eSUCCESS )
   {
      DBG_LW_printf( "BM_init: %u bytes are not used in external RAM and could be reclaimed for MAC buffers.\n", sizeof( extMemPool ) - externMemIndex_ );
   }
#endif
   return ( retVal );
}
/***********************************************************************************************************************

   Function name: BM_alloc

   Purpose: Allocate a buffer (from task context).  This routine attempts to allocate a buffer from a buffer pool.  It
   selects the pool with the smallest buffer size that satisfies the requested size and has buffers available.  For
   example, if there are three pools with 48, 256, and 1024 byte buffers and the caller requests a 100 byte buffer, then
   this will allocate from the 256 byte buffer pool if it is non-empty, else it will allocate from the 1024 byte buffer
   pool if it is non-empty, else it reports a failure.

   This routine does not block waiting for a free buffer.  It returns NULL if no buffers are available.

   Arguments: uint16_t minSize - The minimum buffer size required.  The allocated buffer will have a data area of at
                                 least this size. The application stores its data in the "data" field of the returned
                                 buffer structure.

   Returns: buffer_t * - A pointer to the allocated buffer, or NULL if no buffer of an acceptable size was available.

   Side Effects:

   Reentrant Code: No

   Notes:   There is currently no concept of allocation "priority" (i.e., keeping some buffers in reserve for high-
            priority use).  Any & all callers will be given a buffer if there are any available of a satisfactory size.

 ******************************************************************************************************************** */
buffer_t *BM_Alloc( uint16_t minSize, const char *file, int line )
{
   return bufAlloc( minSize, eBM_APP, file, line );
}
/***********************************************************************************************************************

   Function name: BM_allocDebug

   Purpose: Allocate a buffer (from task context).  This routine attempts to allocate a buffer from a buffer pool.  It
   selects the pool with the smallest buffer size that satisfies the requested size and has buffers available.  For
   example, if there are three pools with 48, 256, and 1024 byte buffers and the caller requests a 100 byte buffer, then
   this will allocate from the 256 byte buffer pool if it is non-empty, else it will allocate from the 1024 byte buffer
   pool if it is non-empty, else it reports a failure.

   This routine does not block waiting for a free buffer.  It returns NULL if no buffers are available.

   Arguments: uint16_t minSize - The minimum buffer size required.  The allocated buffer will have a data area of at
                                 least this size. The application stores its data in the "data" field of the returned
                                 buffer structure.

   Returns: buffer_t * - A pointer to the allocated buffer, or NULL if no buffer of an acceptable size was available.

   Side Effects:

   Reentrant Code: No

   Notes:   There is currently no concept of allocation "priority" (i.e., keeping some buffers in reserve for high-
            priority use).  Any & all callers will be given a buffer if there are any available of a satisfactory size.

 ******************************************************************************************************************** */
buffer_t *BM_AllocDebug( uint16_t minSize, const char *file, int line )
{
   return bufAlloc( minSize, eBM_DEBUG, file, line );
}
/***********************************************************************************************************************

   Function name: BM_allocStack

   Purpose: Allocate a buffer (from task context).  This routine attempts to allocate a buffer from a buffer pool.  It
   selects the pool with the smallest buffer size that satisfies the requested size and has buffers available.  For
   example, if there are three pools with 48, 256, and 1024 byte buffers and the caller requests a 100 byte buffer, then
   this will allocate from the 256 byte buffer pool if it is non-empty, else it will allocate from the 1024 byte buffer
   pool if it is non-empty, else it reports a failure.

   This routine does not block waiting for a free buffer.  It returns NULL if no buffers are available.

   Arguments: uint16_t minSize - The minimum buffer size required.  The allocated buffer will have a data area of at
                                 least this size. The application stores its data in the "data" field of the returned
                                 buffer structure.

   Returns: buffer_t * - A pointer to the allocated buffer, or NULL if no buffer of an acceptable size was available.

   Side Effects:

   Reentrant Code: No

   Notes:   There is currently no concept of allocation "priority" (i.e., keeping some buffers in reserve for high-
            priority use).  Any & all callers will be given a buffer if there are any available of a satisfactory size.

 ******************************************************************************************************************** */
buffer_t *BM_AllocStack( uint16_t minSize, const char *file, int line )
{
   return bufAlloc( minSize, eBM_STACK, file, line );
}
/***********************************************************************************************************************

   Function name: BM_allocStatic

   Purpose: Initialize a buffer that was allocated statically.


   Arguments: buffer_t *ptr - Point to a buffer_t type to be initialized.

   Returns:

   Side Effects:

   Reentrant Code: Yes

 ******************************************************************************************************************** */
void BM_AllocStatic( buffer_t *ptr, eSysFormat_t sysFormat )
{
   ptr->eSysFmt = sysFormat;
   ptr->x.flag.isFree   = false;
   ptr->x.flag.isStatic = true; // Buffer is static. It wasn't taken from the BM pool of buffers.
   ptr->x.flag.inQueue  = 0;    // Not in a queue yet.
}
/***********************************************************************************************************************

   Function name: bufAlloc

   Purpose: Allocate a buffer (from task context).  This routine attempts to allocate a buffer from a buffer pool.  It
   selects the pool with the smallest buffer size that satisfies the requested size and has buffers available.  For
   example, if there are three pools with 48, 256, and 1024 byte buffers and the caller requests a 100 byte buffer, then
   this will allocate from the 256 byte buffer pool if it is non-empty, else it will allocate from the 1024 byte buffer
   pool if it is non-empty, else it reports a failure.

   This routine does not block waiting for a free buffer.  It returns NULL if no buffers are available.

   Arguments:  uint16_t minSize -   The minimum buffer size required.  The allocated buffer will have a data area of at
                                    least this size. The application stores its data in the "data" field of the returned
                                    buffer structure.
               eBM_BufferUsage_t type - allocates from the application or debugger

   Returns: buffer_t * - A pointer to the allocated buffer, or NULL if no buffer of an acceptable size was available.

   Side Effects:

   Reentrant Code: No

   Notes:   There is currently no concept of allocation "priority" (i.e., keeping some buffers in reserve for high-
            priority use).  Any & all callers will be given a buffer if there are any available of a satisfactory size.

 ******************************************************************************************************************** */
static buffer_t *bufAlloc( uint16_t minSize, eBM_BufferUsage_t type, const char *file, int line )
{
   uint8_t pool;
   buffer_t *pBuf = NULL;
   OS_TICK_Struct CurrentTime;
   bool           overflow;
   static bool outOfBuffErrorLogged_ = false; //To restrict logging out of buffers event/error once per power up

   for ( pool = 0; pool < BUFFER_N_POOLS; pool++ )
   {
      if ( ( type != BM_bufferPoolParams[pool].type ) || ( BM_bufferPoolParams[pool].size < minSize ) )
      {
         /* Wrong type or buffer size of this pool is too small - keep searching */
         continue;
      }
      if ( OS_MSGQ_PEND( &bufferPools_[pool], ( void * )&pBuf, 0, ( bool )false, __FILE__, __LINE__ ) )
      {
         /* was able to allocate a buffer */
         // Do some sanity check
         if ( pBuf->x.flag.inQueue )
         {
            // The buffer is in use.
            DBG_printf( "\nERROR: BM_alloc got a buffer marked as in used (pending on a queue). Size: %u, pool = %u, addr=0x%p\n"
                        "ERROR: BM_alloc called from %s:%d", minSize, pool, pBuf, file, line );
         }
#if (BM_DEBUG==1)
         if ( strcasecmp( file, "dbg_serialdebug.c" ) != 0 )
         {
            char *name = _task_get_template_ptr( _task_get_id() )->TASK_NAME;
            DBG_printf( "\nalloc pool: %2d, reqSize: %4d, poolSize:%4d, ptr: 0x%08x, addr: 0x%08x, called from (%s) %s:%d",
                        pool, minSize, BM_bufferPoolParams[pool].size, ( uint32_t )pBuf, pBuf->data, name, file, line );
         }
#endif
         pBuf->pfile             = file;
         pBuf->line              = ( uint32_t )line;
         pBuf->x.flag.isFree     = false; // buffer has been allocated
         pBuf->x.flag.isStatic   = false;
         pBuf->x.flag.inQueue    = 0; // Not in queue yet
         pBuf->x.dataLen         = minSize; /* Initialize this with the size requested */
         OS_INT_disable( );
         BM_bufferStats.pool[pool].allocOk++;
         BM_bufferStats.pool[pool].currAlloc++;
         if ( BM_bufferStats.pool[pool].currAlloc > BM_bufferStats.pool[pool].highwater )
         {
            BM_bufferStats.pool[pool].highwater = BM_bufferStats.pool[pool].currAlloc;
         }
         OS_INT_enable( );
         break;
      }
      else
      {
         OS_INT_disable( );
         BM_bufferStats.pool[pool].allocFail++; /* pool empty - try remaining pools (in increasing size order) */
         OS_INT_enable( );
      }
   }
   if ( NULL == pBuf )
   {
      OS_INT_disable( );
      // Buffer watchdog
      if ( ( eBM_APP == type ) || ( eBM_STACK == type ) )
      {
         // Get current time
         OS_TICK_Get_CurrentElapsedTicks( &CurrentTime );

         // Is this the first time we run out of buffers?
         if ( ( AllocWatchDog[type].TICKS[0] == 0 ) && ( AllocWatchDog[type].TICKS[1] == 0 ) )
         {
            // Save timestamp
            ( void )memcpy( &AllocWatchDog[type], &CurrentTime, sizeof( OS_TICK_Struct ) );
         }
         else
         {
            // Have we been out of buffers for a long time?
            if ( ( _time_diff_minutes( &CurrentTime, &AllocWatchDog[type], &overflow ) > ALLOC_WATCHDOG_TIMEOUT ) || overflow )
            {
               // Need to reboot
               if ( outOfBuffErrorLogged_ == false )
               {
                  //Just log it once to avoid recursive calls
                  outOfBuffErrorLogged_ = true;
                  DBG_LW_printf ( "\nFatal - Out of buffers. Type requested: %hhu, Size requested: %hu", ( uint8_t )type, minSize );
                  BM_showAlloc( ( bool )false );
                  EVL_FirmwareError( "bufAlloc", ( char * )file, line );
               }
            }
         }
      }

      if ( eBM_APP == type )
      {
         BM_bufferStats.allocFailApp++;
      }
      else if ( eBM_STACK == type )
      {
         BM_bufferStats.allocFailStack++;
      }
      else if ( eBM_DEBUG == type )
      {
         BM_bufferStats.allocFailDebug++;
      }

      OS_INT_enable( );
   }
   return pBuf;
}
/***********************************************************************************************************************

   Function name: BM_free

   Purpose: Free an allocated a buffer (from task context).  This routine returns a buffer to its original free buffer
   pool, based on information in its header.  * This routine can be called from any task.  Use bufferFreeFromISR() from
   ISR context.

   Arguments: buffer_t *pBuf - The address of the buffer to free.

   Returns: None

   Side Effects:

   Reentrant Code: No

   Notes:  The application is responsible for ensuring that each allocated buffer is freed exactly one time.

 ******************************************************************************************************************** */
void BM_Free( buffer_t *pBuf, const char *file, int line )
{

   uint8_t  pool;
   eBM_BufferUsage_t type;

   // NULL pointer
   if ( pBuf == NULL )
   {
      return;
   }

   OS_MUTEX_Lock( &bufMutex_ );


   // When we get here, it means that either:
   // 1) we found the buffer in internal or external RAM
   // 2) the buffer was staticaly allocated
   // 3) the buffer is invalid
   pool = pBuf->x.bufPool;
   // Validate critical buffer header values before using them
   if ( ( !pBuf->x.flag.isStatic ) && ( pool >= BUFFER_N_POOLS || pBuf->bufMaxSize != BM_bufferPoolParams[pool].size ) )
   {
      /* ERROR: corrupted buffer header - cannot free buffer! */
      BM_bufferStats.freeFail++;
      // The buffer is not free.
      DBG_printf( "\nWARNING: BM_Free is trying to free buffer with corrupted header. Can't free! Pool = %u, addr=0x%p\n"
                  "WARNING: BM_Free called from %s:%d", pool, pBuf, file, line );
   }
   else
   {
      if ( pBuf->x.flag.isFree )
      {
         /* ERROR: already marked as free - take no further action */
         BM_bufferStats.freeDoubleFree++;
         // The buffer is not free.
         DBG_printf( "\nWARNING: BM_Free is trying to free an already freed buffer. Pool = %u, addr=0x%p\n"
                     "WARNING: BM_Free called from %s:%d", pool, pBuf, file, line );
      }
      else
      {
         // Buffer hasn't been freed yet
         if ( pBuf->x.flag.inQueue )
         {
            // The buffer is in use.
            DBG_printf( "\nERROR: BM_Free is trying to free a buffer marked as in used (pending on a queue). Pool = %u, addr=0x%p\n"
                        "ERROR: BM_Free called from %s:%d", pool, pBuf, file, line );
         }
         pBuf->x.flag.isFree = true; // Mark as free

         // Free buffer only if not static
         if ( !pBuf->x.flag.isStatic )
         {
            OS_MSGQ_POST( &bufferPools_[pool], pBuf, ( bool )false, __FILE__, __LINE__ ); // Function will not return if it fails
            // Place buffer in pool
#if (BM_DEBUG==1)
            if ( strcasecmp( file, "dbg_serialdebug.c" ) != 0 )
            {
               char *name = _task_get_template_ptr( _task_get_id() )->TASK_NAME;
               DBG_printf( "\nfree  pool: %2d, reqSize: %4d, poolSize:%4d, ptr: 0x%08x, called from (%s) %s:%d, Allocator: %s, line: %d",
                           pool, pBuf->x.dataLen, BM_bufferPoolParams[pool].size, ( uint32_t )pBuf, name, file, line, pBuf->pfile,
                           pBuf->line );
            }
#endif
            if ( BM_bufferStats.pool[pool].currAlloc != 0 )
            {
               BM_bufferStats.pool[pool].currAlloc--;
               BM_bufferStats.pool[pool].freeOk++;
            }
            else
            {
               char *name = _task_get_template_ptr( _task_get_id() )->TASK_NAME;
               DBG_printf( "\nfree  pool: %2d, reqSize: %4d, poolSize:%4d, ptr: 0x%08x, "
                           "called from (%s) %s:%d, Allocator: %s, line: %d\n"
                           "Current allocation already ZERO", pool, pBuf->x.dataLen, BM_bufferPoolParams[pool].size, ( uint32_t )pBuf,
                           name, file, line, pBuf->pfile, pBuf->line );
            }

            // Reset AllocWatchdog
            type = BM_bufferPoolParams[pool].type;
            if ( ( eBM_APP == type ) || ( eBM_STACK == type ) )
            {
               ( void )memset( &AllocWatchDog[type], 0, sizeof( OS_TICK_Struct ) );
            }
         }
      }
   }
   OS_MUTEX_Unlock( &bufMutex_ ); // Function will not return if it fails
}

/***********************************************************************************************************************

   Function name: BM_getStats

   Purpose: Copies the buffer statistics to the pointer location passed in.

   Arguments: bufferStats_t *pStats - location to copy the statics

   Returns: None

   Side Effects:

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void BM_getStats( bufferStats_t *pStats )
{
   OS_MUTEX_Lock( &bufMutex_ );
   ( void )memcpy( pStats, &BM_bufferStats, sizeof( *pStats ) );
   OS_MUTEX_Unlock( &bufMutex_ );
}

/***********************************************************************************************************************

   Function name: BM_resetStats

   Purpose: resets all buffer statistics

   Arguments: none

   Returns: None

   Side Effects:

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void BM_resetStats( void )
{
   ( void )memset( &BM_bufferStats, 0, sizeof( BM_bufferStats ) );
}

/***********************************************************************************************************************

   Function name: BM_showAlloc

   Purpose: Print stats for allocated buffers.

   Arguments: bool safePrint - True will use the debug logger.

   Returns: None

   Side Effects:

   Reentrant Code: No

   Notes:

 ******************************************************************************************************************** */
void BM_showAlloc( bool safePrint )
{
   uint32_t i, j; // loop counter
   uint32_t nbBuffers;
   buffer_t *pBuf;
   char     str[100];

   OS_MUTEX_Lock( &bufMutex_ );

   ( void )snprintf( str, (int32_t)sizeof( str ), "\nAllocated buffers:\nPool reqSize poolSize        ptr  ptr->addr                 file  line\n" );
   if ( safePrint )
   {
      DBG_printfNoCr( str );
   }
   else
   {
      DBG_LW_printf( str );
   }

   // Data for internal memory scan
   pBuf      = pBufIntMeta;
   nbBuffers = bIntMetaNbBuffers;

   for ( j = 0;
#if ( DCU == 1 )
         j < 2;
#else
         j < 1;
#endif
         j++ )
   {
      // Find the buffer header.
      for ( i = 0; i < nbBuffers; i++ )
      {
         // Don't print debug buffer stats because we use them to print those results and this would lead to false positive.
         if ( BM_bufferPoolParams[pBuf->x.bufPool].type != eBM_DEBUG )
         {
            // Is the buffer in use?
            if ( !pBuf->x.flag.isStatic && !pBuf->x.flag.isFree && pBuf->x.dataLen )
            {
               ( void )snprintf( str, (int32_t)sizeof( str ), "  %2u %7u     %4u 0x%08X 0x%08X %20s %5u\n",
                                 pBuf->x.bufPool, pBuf->x.dataLen, BM_bufferPoolParams[pBuf->x.bufPool].size,
                                 pBuf, pBuf->data, pBuf->pfile, pBuf->line );
               if ( safePrint )
               {
                  DBG_printfNoCr( str );
               }
               else
               {
                  DBG_LW_printf( str );
               }
            }
         }
         pBuf = ( buffer_t* )( void* )( ( ( uint8_t* )pBuf ) + bMetaSize );
      }
      // Data for external memory scan
#if ( DCU == 1 )
      pBuf      = pBufExtMeta;
      nbBuffers = bExtMetaNbBuffers;
#endif
   }
   OS_MUTEX_Unlock( &bufMutex_ ); // Function will not return if it fails
}
#ifdef WOLFSSL_DEBUG_MEMORY
/***********************************************************************************************************************
   Function Name: BM_SpecLibMalloc

   Purpose: This function is called by extenal libraries to malloc memory.

   Arguments:
         size_t reqSize            - Size of the memory to allocate

   Returns: Pointer to the allocated buffer.

   NOTE:
   The wolfSSL library will make a lot of small memory allocations, so having
   a fairly large number of small buffers (on the order of 32 8-byte buffers)
   will help with memory usage
 **********************************************************************************************************************/
void *BM_SpecLibMalloc( size_t reqSize, const char *func, int32_t line )
{
   buffer_t       *pBuf;   /* Pointer to the allocated buffer */
   uint16_t       actSize; /* Size adusted to allow for original buffer_t *   */
   pspecLibBuf_t  specBuf; /* Holds aggregated buffer_t * and void * values   */

   actSize = (uint16_t)( reqSize + sizeof( buffer_t * ) );
   pBuf = bufAlloc( actSize, eBM_STACK, func , line );
   if ( pBuf == NULL )
   {
      pBuf = bufAlloc( actSize, eBM_APP, func , line );
   }
   if ( pBuf == NULL )
   {
      INFO_printf( "BM_allocDebug" );
      pBuf = bufAlloc( actSize, eBM_DEBUG, func , line );
   }
   if ( pBuf == NULL )
   {
      ERR_printf( "BM_SpecLibMalloc failure for %u bytes", actSize );
      return NULL;
   }

   /* Treat the data element of the buffer_t returned as a pointer to the special type and insert the original buffer_t in the data area. */
   specBuf = (pspecLibBuf_t)(void *)pBuf->data;
   specBuf->pBuffer = pBuf;
   return ( void * )( ( uint8_t *)(pBuf->data ) + offsetof( specLibBuf_t, data ) );
}
#else
void *BM_SpecLibMalloc( size_t reqSize )
{
   buffer_t       *pBuf;   /* Pointer to the allocated buffer */
   uint16_t       actSize; /* Size adusted to allow for original buffer_t *   */
   pspecLibBuf_t  specBuf; /* Holds aggregated buffer_t * and void * values   */

   actSize = (uint16_t)( reqSize + sizeof( buffer_t * ) );
   pBuf = bufAlloc( actSize, eBM_STACK, __func__ , __LINE__ );
   if ( pBuf == NULL )
   {
      pBuf = bufAlloc( actSize, eBM_APP, __func__ , __LINE__ );
   }
   if ( pBuf == NULL )
   {
      INFO_printf( "BM_allocDebug" );
      pBuf = bufAlloc( actSize, eBM_DEBUG, __func__ , __LINE__ );
   }
   if ( pBuf == NULL )
   {
      ERR_printf( "BM_SpecLibMalloc failure for %u bytes", actSize );
      return NULL;
   }

   /* Treat the data element of the buffer_t returned as a pointer to the special type and insert the original buffer_t in the data area. */
   specBuf = (pspecLibBuf_t)(void *)pBuf->data;
   specBuf->pBuffer = pBuf;
   return ( void * )( ( uint8_t *)(pBuf->data ) + offsetof( specLibBuf_t, data ) );
}
#endif
/***********************************************************************************************************************
   Function Name: BM_SpecLibFree

   Purpose: This function is called by external libraries to free memory.

   Arguments:
         void *ptr                 - Data that contains the buffer to free.

   Returns: None

   Note:  Memory MUST have been allocated by BM_SpecLibMalloc so that the original buffert_t pointer can be retrieved.

 **********************************************************************************************************************/
/*lint -efunc(818, BM_SpecLibFree) parameter could be ptr to const */
void BM_SpecLibFree( void *ptr )
{
   buffer_t *origBuf;   /* Original buffer_t *, saved in data allocated.  */

   /* Extract original buffer_t from memory at ptr - sizeof buffer_t *  */
   origBuf = *( buffer_t **)(void *)(( uint8_t *)ptr - sizeof( buffer_t * ) );
   BM_free( origBuf );
}
