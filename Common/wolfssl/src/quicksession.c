/**
 * Implements suspend and restore of DTLS sessions
 */

#include <stddef.h>
#include <wolfssl/quicksession.h>
#include <wolfssl/internal.h>
#include <wolfssl/error-ssl.h>
#include "user_settings.h"

static const uint8_t key[] =
{
   0x7b, 0xcc, 0x60, 0x9f, 0x2c, 0x1e, 0x30, 0xd4,
   0x12, 0x19, 0x39, 0x51, 0x92, 0x84, 0xa5, 0x93,
   0x33, 0x8d, 0x91, 0x7f, 0x92, 0x0f, 0x20, 0x74,
   0x9c, 0x44, 0xbf, 0xd9, 0x11, 0x89, 0x59, 0x5a
};

static const uint8_t iv[] =
{
   0x56, 0x70, 0x8f, 0x5e, 0x11, 0x4b, 0x11, 0x69,
   0x9d, 0x3e, 0x88, 0xaa, 0xb1, 0x3e, 0x92, 0x3c
};
#if ( DTLS_CHECK_UNENCRYPTED == 1 )
static bool fileIsEncrypted = (bool)true; /* Set false by wolfSSL_restore_session_with_window if NOT encrypted */
#endif

/***********************************************************************************************************************

   Function name: encryptBuffer

   Purpose: Encrypt the buffer with AES-CBC mode, which requires block padding before pass-in.

   Arguments:
            uint8_t  *dest - encrypt destinaion
            uint8_t  *src  - encrypt soure
            int32_t   size  - soure size, must be multiple integer size of AES block size.

   Returns: None
 ******************************************************************************************************************** */
static void encryptBuffer( uint8_t *dest, uint8_t *src, int32_t size )
{
   Aes aes;

   wc_AesSetKey( &aes, key, sizeof( key ), iv, AES_ENCRYPTION );
   wc_AesCbcEncrypt( &aes, dest, src, size );
}

/***********************************************************************************************************************

   Function name: decryptBuffer

   Purpose: Decrypt the buffer with AES-CBC mode, which requires block padding before pass-in.

   Arguments:
            uint8_t  *dest - decrypt destinaion
            uint8_t  *src  - decrypt soure
            int32_t   size  - soure size, must be multiple integer size of AES block size.

   Returns: None
 ******************************************************************************************************************** */
static void decryptBuffer( uint8_t *dest, uint8_t *src, int32_t size )
{
   Aes aes;

   wc_AesSetKey( &aes, key, sizeof( key ), iv, AES_DECRYPTION );
   wc_AesCbcDecrypt( &aes, dest, src, size );
}

/***********************************************************************************************************************

   Function name: generateMD5

   Purpose: Generate MD5 at the end of buffer.

   Arguments:
            uint8_t  *buf  - buffer
            int32_t   size  - buffer size.

   Returns: None
 ******************************************************************************************************************** */
static void generateMD5( uint8_t *buffer, int32_t size )
{
   int32_t  dataSize = size - MD5_DIGEST_SIZE;
   wc_Md5Hash( buffer, dataSize, buffer + dataSize );
}

/***********************************************************************************************************************

   Function name: validateMD5

   Purpose: Verify buffer data integration with the MD5 sotred at the end of buffer.

   Arguments: bool

   Returns: None
 ******************************************************************************************************************** */
static int32_t validateMD5( uint8_t *buffer, int32_t size )
{
   uint8_t  actual[MD5_DIGEST_SIZE];
   int32_t  dataSize = size - MD5_DIGEST_SIZE;
   int32_t  result;

   wc_Md5Hash( buffer, dataSize, actual );

   result = XMEMCMP( buffer + dataSize, actual, MD5_DIGEST_SIZE );
   return !result;
}

int32_t wolfSSL_get_window( WOLFSSL *ssl, void *buffer, int32_t offset, int32_t sz )
{
   uint8_t  *src = ( uint8_t *)ssl + offset;

   XMEMCPY( buffer, src, sz );

   return SSL_ERROR_NONE;
}

#if 0
int32_t wolfSSL_get_window_offset( WOLFSSL *ssl, int32_t *offset, int32_t *sz )
{
   *offset = 0;
   *sz = 0;
   size_t calculatedSize = 0;

   if ( ssl == NULL )
   {
      return SSL_ERROR_NONE;
   }

   calculatedSize += sizeof( ssl->keys.aead_exp_IV );
   calculatedSize += sizeof( ssl->keys.aead_enc_imp_IV );
   calculatedSize += sizeof( ssl->keys.aead_dec_imp_IV );

   // @JRB
   calculatedSize += sizeof( ssl->keys.peer_sequence_number_hi );
   calculatedSize += sizeof( ssl->keys.peer_sequence_number_lo );
   calculatedSize += sizeof( ssl->keys.sequence_number_hi );
   calculatedSize += sizeof( ssl->keys.sequence_number_lo );
   calculatedSize += sizeof( ssl->keys.dtls_sequence_number_hi );
   calculatedSize += sizeof( ssl->keys.dtls_sequence_number_lo );
   calculatedSize += sizeof( ssl->keys.peerSeq );

   calculatedSize += sizeof( ssl->keys.dtls_peer_handshake_number );
   calculatedSize += sizeof( ssl->keys.dtls_peer_handshake_number );
   calculatedSize += sizeof( ssl->keys.dtls_epoch );
   calculatedSize += sizeof( ssl->keys.dtls_handshake_number );

   *sz = calculatedSize;
   *offset = ( int32_t )offsetof( struct WOLFSSL, keys.aead_exp_IV );

   return SSL_ERROR_NONE;
}
#endif

int32_t wolfSSL_get_suspend_memsize( WOLFSSL *ssl )
{
   int32_t  ret;
   uint32_t size;

   if ((ret = wolfSSL_dtls_export(ssl, NULL, &size) != 0))
   {
      return ret;
   }
   size += MD5_DIGEST_SIZE;

   // Align to AES block size
   size = ( size + AES_BLOCK_SIZE - 1 ) / 16 * 16;

   return size;
}

/***********************************************************************************************************************

   Function name: wolfSSL_suspend_session

   Purpose: This function is called to save the (full) ssl session into a buffer.

   Arguments:
            WOLFSSL  *ssl  - pointer to the WOLFSSL session to save
            void     *buf  - pointer to a buffer to save
            int32_t  sz    - size of the buffer

   Returns: int32_t size of serialized session
            < 0      wolfSSL Errors
            = 0      SSL_ERROR_NONE, no action;
            > 0      return the size of serialized session on success
                     (currently we encypted the max_size session + padding + MD5)

   Note: Serialized Format
         | ... serialized session ...| ... Padding 0s (if have) ... | ... MD5 (from start to end of padding) ... |
         | <-----                      AES-CBC Encrypted                                                  -----> |
 ******************************************************************************************************************** */
int32_t wolfSSL_suspend_session( WOLFSSL *ssl, void *mem, int32_t sz )
{
   uint8_t  *ptr = mem;
   int32_t  size;

   WOLFSSL_ENTER( "wolfSSL_suspend_session" );

   size = wolfSSL_get_suspend_memsize( ssl );
   if ( sz < size )
   {
      WOLFSSL_MSG( "Memory buffer too small" );
      return BUFFER_E;
   }
   if( wolfSSL_dtls_export( ssl, mem, (uint32_t *) &sz ) < 0 )
   {
      WOLFSSL_MSG( "wolfSSL_dtls_export failed" );
      return BUFFER_E;
   }

   generateMD5( ( uint8_t *)mem, size );
   encryptBuffer( ( uint8_t *)mem, mem, size );
   WOLFSSL_LEAVE( "wolfSSL_suspend_session", SSL_ERROR_NONE );

   return SSL_ERROR_NONE;
}

bool QUICK_sessionEncrypted( void )
{
   return fileIsEncrypted;
}
/***********************************************************************************************************************

   Function name: wolfSSL_restore_session_with_window

   Purpose: This function is called to restore the ssl session.

   Arguments:
            WOLFSSL_CTX *ctx                    - pointer to the current WOLFSSL context
            uint8_t *last_saved_session         - pointer to a new WOLFSSL session to save
            int32_t saved_session_sz            - size of saved session (full)
            uint8_t *last_saved_session_state   - pointer to saved session state
            int32_t saved_state_sz              - size of saved session (state only)

   Returns: WOLFSSL *ssl
 ******************************************************************************************************************** */
WOLFSSL_API
WOLFSSL *wolfSSL_restore_session_with_window( WOLFSSL_CTX *ctx, uint8_t *session, int32_t sessionSize, uint8_t *window, int32_t windowSize )
{
   WOLFSSL  *ssl = NULL;
   uint8_t  *ptr = NULL;
   int32_t  ret = SSL_ERROR_NONE;

   WOLFSSL_ENTER( "wolfSSL_restore_session" );

   ssl = wolfSSL_new( ctx );
   if ( ssl != NULL )
   {
      ptr = XMALLOC( sessionSize, ssl->heap, DYNAMIC_TYPE_IN_BUFFER );
      if ( ptr == NULL )
      {
         wolfSSL_free( ssl );
         ssl = NULL;
         ret = MEMORY_E;
      }
      else
      {
         decryptBuffer( ptr, session, sessionSize );  /* Decrypt session buffer into ptr buffer */
#if ( DTLS_CHECK_UNENCRYPTED == 1 )
         /* Handle case where earlier version didn't encrypt the major session data */
         if ( !validateMD5( ptr, sessionSize ) )      /* If this fails, try importing unencrypted data.  */
         {
            fileIsEncrypted = (bool)false;
            ret = wolfSSL_dtls_import( ssl, session, sessionSize );
            XFREE( ptr, ssl->heap, DYNAMIC_TYPE_IN_BUFFER );
         }
         else  /* Decryption/validation passed, proceed with decrypted buffer.   */
         {
            ret = wolfSSL_dtls_import( ssl, ptr, sessionSize );
            XFREE( ptr, ssl->heap, DYNAMIC_TYPE_IN_BUFFER );
         }
         if ( ( ret >= 0 ) && ( window != NULL ) )
         {
            ret = wolfSSL_dtls_import( ssl, window, windowSize );
         }
         else
         {
            WOLFSSL_MSG( "MD5 digest validation failed/Raw import failed." );
         }
         if ( ret < 0 )
         {
            wolfSSL_free( ssl );
            ssl = NULL;
         }
#else
         if ( !validateMD5( ptr, sessionSize ) )
         {
            WOLFSSL_MSG( "MD5 digest validation failed." );
            XFREE( ptr, ssl->heap, DYNAMIC_TYPE_IN_BUFFER );
            wolfSSL_free( ssl );
            ssl = NULL;
            ret = MEMORY_E;
         }
         else
         {
            ret = wolfSSL_dtls_import( ssl, ptr, sessionSize );
            XFREE( ptr, ssl->heap, DYNAMIC_TYPE_IN_BUFFER );
            if ( ( ret >= 0 ) && ( window != NULL ) )
            {
               ret = wolfSSL_dtls_import( ssl, window, windowSize );
            }
            if ( ret < 0 )
            {
               wolfSSL_free( ssl );
               ssl = NULL;
            }
         }
#endif
      }
   }
   else
   {
      WOLFSSL_MSG( "Failed to allocate SSL" );
      ret = MEMORY_E;
   }

   WOLFSSL_LEAVE( "wolfSSL_restore_session", ret );
   return ssl;
}

WOLFSSL *wolfSSL_restore_session( WOLFSSL_CTX *ctx, void *mem, int32_t sz )
{
   return wolfSSL_restore_session_with_window( ctx, mem, sz, NULL, 0 );
}

