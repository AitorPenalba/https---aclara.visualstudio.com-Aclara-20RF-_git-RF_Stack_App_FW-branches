/**
 * Implements suspend and restore of DTLS sessions
 */

#include <stddef.h>
#include "quicksession-altered.h"
#include <wolfssl/internal.h>
#include <wolfssl/error-ssl.h>

static const byte key[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
                          };

static const byte iv[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                         };

static void encryptBuffer( byte* dest, byte* src, int size )
{
   Aes aes;

   wc_AesSetKey( &aes, key, sizeof( key ), iv, AES_ENCRYPTION );
   wc_AesCbcEncrypt( &aes, dest, src, size );
}

static void decryptBuffer( byte* dest, byte* src, int size )
{
   Aes aes;

   wc_AesSetKey( &aes, key, sizeof( key ), iv, AES_DECRYPTION );
   wc_AesCbcDecrypt( &aes, dest, src, size );
}

static void generateMD5( byte* buffer, int size )
{
   int dataSize = size - MD5_DIGEST_SIZE;
   wc_Md5Hash( buffer, dataSize, buffer + dataSize );
}

static int validateMD5( byte* buffer, int size )
{
   byte actual[MD5_DIGEST_SIZE];
   int dataSize = size - MD5_DIGEST_SIZE;
   int result;

   wc_Md5Hash( buffer, dataSize, actual );

   result = XMEMCMP( buffer + dataSize, actual, MD5_DIGEST_SIZE );
   return !result;
}

int wolfSSL_get_window( WOLFSSL* ssl, void* buffer, int offset, int sz )
{
   byte* src = ( byte* )ssl + offset;

   XMEMCPY( buffer, src, sz );

   return SSL_ERROR_NONE;
}

int wolfSSL_get_window_offset( WOLFSSL* ssl, int* offset, int* sz )
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
   *offset = ( int )offsetof( struct WOLFSSL, keys.aead_exp_IV );

   return SSL_ERROR_NONE;
}

int wolfSSL_get_suspend_memsize( WOLFSSL* ssl )
{
   int ret;
   unsigned int size;
  
   if ((ret = wolfSSL_dtls_export(ssl, NULL, &size) != 0)) {
    return ret;
   }
   size += MD5_DIGEST_SIZE;

//#if defined(BUILD_AES) || defined(BUILD_AESGCM)
//   size++;
//   if ( ssl->encrypt.aes != NULL )
//   {
//      size += sizeof( Aes );
//   }
//#endif
//
//#ifdef BUILD_ARC4
//   size++;
//   if ( ssl->encrypt.arc4 != NULL )
//   {
//      size += sizeof( Arc4 );
//   }
//#endif
//
//#ifdef BUILD_DES3
//   size++;
//   if ( ssl->encrypt.des3 != NULL )
//   {
//      size += sizeof( Des3 );
//   }
//#endif
//
//#ifdef BUILD_RABBIT
//   size++;
//   if ( ssl->encrypt.rabbit != NULL )
//   {
//      size += sizeof( Rabbit );
//   }
//#endif
//
//#if defined(BUILD_AES) || defined(BUILD_AESGCM)
//   size++;
//   if ( ssl->decrypt.aes != NULL )
//   {
//      size += sizeof( Aes );
//   }
//#endif
//
//#ifdef BUILD_ARC4
//   size++;
//   if ( ssl->decrypt.arc4 != NULL )
//   {
//      size += sizeof( Arc4 );
//   }
//#endif
//
//#ifdef BUILD_DES3
//   size++;
//   if ( ssl->decrypt.des3 != NULL )
//   {
//      size += sizeof( Des3 );
//   }
//#endif
//
//#ifdef BUILD_RABBIT
//   size++;
//   if ( ssl->decrypt.rabbit != NULL )
//   {
//      size += sizeof( Rabbit );
//   }
//#endif
//
//   size += sizeof( ssl->session );
//
//   size += sizeof( ssl->curRL );
//
//   size += sizeof( ssl->msgsReceived );
//
//   size += sizeof( ssl->version );
//
//   size += sizeof( ssl->specs );
//
//   size += sizeof( ssl->keys );
//
//   size += sizeof( ssl->options );
//
//   size += sizeof( ssl->dtls_timeout_init );
//
//   size += sizeof( ssl->dtls_timeout_max );
//
//   size += sizeof( ssl->dtls_timeout );
//
   // Align to AES block size
   size = ( size + AES_BLOCK_SIZE - 1 ) / 16 * 16;

   return size;
}

int wolfSSL_suspend_session( WOLFSSL* ssl, void* mem, int sz )
{
   byte * ptr = mem;

   WOLFSSL_ENTER( "wolfSSL_suspend_session" );

   if ( sz < wolfSSL_get_suspend_memsize( ssl ) )
   {
      WOLFSSL_MSG( "Memory buffer too small" );
      return BUFFER_E;
   }

#if defined(BUILD_AES) || defined(BUILD_AESGCM)
   if ( ssl->encrypt.aes != NULL )
   {
      *( ptr++ ) = 1;
      XMEMCPY( ptr, ssl->encrypt.aes, sizeof( Aes ) );
      ptr += sizeof( Aes );
   }
   else
   {
      *( ptr++ ) = 0;
   }
#endif

#ifdef BUILD_ARC4
   if ( ssl->encrypt.arc4 != NULL )
   {
      *( ptr++ ) = 1;
      XMEMCPY( ptr, ssl->encrypt.arc4, sizeof( Arc4 ) );
      ptr += sizeof( Arc4 );
   }
   else
   {
      *( ptr++ ) = 0;
   }
#endif

#ifdef BUILD_DES3
   if ( ssl->encrypt.des3 != NULL )
   {
      *( ptr++ ) = 1;
      XMEMCPY( ptr, ssl->encrypt.des3, sizeof( Des3 ) );
      ptr += sizeof( Des3 );
   }
   else
   {
      *( ptr++ ) = 0;
   }
#endif

#ifdef BUILD_RABBIT
   if ( ssl->encrypt.rabbit != NULL )
   {
      *( ptr++ ) = 1;
      XMEMCPY( ptr, ssl->encrypt.rabbit, sizeof( Rabbit ) );
      ptr += sizeof( Rabbit );
   }
   else
   {
      *( ptr++ ) = 0;
   }
#endif

#if defined(BUILD_AES) || defined(BUILD_AESGCM)
   if ( ssl->decrypt.aes != NULL )
   {
      *( ptr++ ) = 1;
      XMEMCPY( ptr, ssl->decrypt.aes, sizeof( Aes ) );
      ptr += sizeof( Aes );
   }
   else
   {
      *( ptr++ ) = 0;
   }
#endif

#ifdef BUILD_ARC4
   if ( ssl->decrypt.arc4 != NULL )
   {
      *( ptr++ ) = 1;
      XMEMCPY( ptr, ssl->decrypt.arc4, sizeof( Arc4 ) );
      ptr += sizeof( Arc4 );
   }
   else
   {
      *( ptr++ ) = 0;
   }
#endif

#ifdef BUILD_DES3
   if ( ssl->decrypt.des3 != NULL )
   {
      *( ptr++ ) = 1;
      XMEMCPY( ptr, ssl->decrypt.des3, sizeof( Des3 ) );
      ptr += sizeof( Des3 );
   }
   else
   {
      *( ptr++ ) = 0;
   }
#endif

#ifdef BUILD_RABBIT
   if ( ssl->decrypt.rabbit != NULL )
   {
      *( ptr++ ) = 1;
      XMEMCPY( ptr, ssl->decrypt.rabbit, sizeof( Rabbit ) );
      ptr += sizeof( Rabbit );
   }
   else
   {
      *( ptr++ ) = 0;
   }
#endif

   XMEMCPY( ptr, &ssl->session, sizeof( ssl->session ) );
   ptr += sizeof( ssl->session );

   XMEMCPY( ptr, &ssl->curRL, sizeof( ssl->curRL ) );
   ptr += sizeof( ssl->curRL );

   XMEMCPY( ptr, &ssl->msgsReceived, sizeof( ssl->msgsReceived ) );
   ptr += sizeof( ssl->msgsReceived );

   XMEMCPY( ptr, &ssl->version, sizeof( ssl->version ) );
   ptr += sizeof( ssl->version );

   XMEMCPY( ptr, &ssl->chVersion, sizeof( ssl->chVersion ) );
   ptr += sizeof( ssl->chVersion );

   XMEMCPY( ptr, &ssl->specs, sizeof( ssl->specs ) );
   ptr += sizeof( ssl->specs );

   XMEMCPY( ptr, &ssl->keys, sizeof( ssl->keys ) );
   ptr += sizeof( ssl->keys );

   XMEMCPY( ptr, &ssl->options, sizeof( ssl->options ) );
   ptr += sizeof( ssl->options );

   XMEMCPY( ptr, &ssl->dtls_timeout_init, sizeof( ssl->dtls_timeout_init ) );
   ptr += sizeof( ssl->dtls_timeout_init );

   XMEMCPY( ptr, &ssl->dtls_timeout_max, sizeof( ssl->dtls_timeout_max ) );
   ptr += sizeof( ssl->dtls_timeout_max );

   XMEMCPY( ptr, &ssl->dtls_timeout, sizeof( ssl->dtls_timeout ) );
   ptr += sizeof( ssl->dtls_timeout );

   generateMD5( ( byte* )mem, sz );

   encryptBuffer( ( byte* )mem, mem, sz );

   WOLFSSL_LEAVE( "wolfSSL_suspend_session", SSL_ERROR_NONE );

   return SSL_ERROR_NONE;
}

static int restoreSSL( WOLFSSL* ssl, byte* ptr )
{
   int ret = SSL_ERROR_NONE;

   /* Encrypt setup */

#if defined(BUILD_AES) || defined(BUILD_AESGCM)
   if ( *( ptr++ ) )
   {
      ssl->encrypt.aes = ( Aes* )XMALLOC( sizeof( Aes ), ssl->heap, DYNAMIC_TYPE_CIPHER );
      if ( ssl->encrypt.aes == NULL )
      {
         WOLFSSL_MSG( "Failed to allocate Aes" );
         return MEMORY_E;
      }
      XMEMCPY( ssl->encrypt.aes, ptr, sizeof( Aes ) );
      ptr += sizeof( Aes );
   }
#endif

#ifdef BUILD_ARC4
   if ( *( ptr++ ) )
   {
      ssl->encrypt.arc4 = ( Arc4* )XMALLOC( sizeof( Arc4 ), ssl->heap, DYNAMIC_TYPE_CIPHER );
      if ( ssl->encrypt.aes == NULL )
      {
         WOLFSSL_MSG( "Failed to allocate Arc4" );
         return MEMORY_E;
      }
      XMEMCPY( ssl->encrypt.aes, ptr, sizeof( Arc4 ) );
      ptr += sizeof( Arc4 );
   }
#endif

#ifdef BUILD_DES3
   if ( *( ptr++ ) )
   {
      ssl->encrypt.des3 = ( Des3* )XMALLOC( sizeof( Des3 ), ssl->heap, DYNAMIC_TYPE_CIPHER );
      if ( ssl->encrypt.aes == NULL )
      {
         WOLFSSL_MSG( "Failed to allocate Des3" );
         return MEMORY_E;
      }
      XMEMCPY( ssl->encrypt.aes, ptr, sizeof( Des3 ) );
      ptr += sizeof( Des3 );
   }
#endif

#ifdef BUILD_RABBIT
   if ( *( ptr++ ) )
   {
      ssl->encrypt.rabbit = ( Rabbit* )XMALLOC( sizeof( Rabbit ), ssl->heap, DYNAMIC_TYPE_CIPHER );
      if ( ssl->encrypt.aes == NULL )
      {
         WOLFSSL_MSG( "Failed to allocate Rabbit" );
         return MEMORY_E;
      }
      XMEMCPY( ssl->encrypt.aes, ptr, sizeof( Rabbit ) );
      ptr += sizeof( Rabbit );
   }
#endif

   ssl->encrypt.setup = 1;

   /* Decrypt setup */
#if defined(BUILD_AES) || defined(BUILD_AESGCM)
   if ( *( ptr++ ) )
   {
      ssl->decrypt.aes = ( Aes* )XMALLOC( sizeof( Aes ), ssl->heap, DYNAMIC_TYPE_CIPHER );
      if ( ssl->decrypt.aes == NULL )
      {
         WOLFSSL_MSG( "Failed to allocate Aes" );
         return MEMORY_E;
      }
      XMEMCPY( ssl->decrypt.aes, ptr, sizeof( Aes ) );
      ptr += sizeof( Aes );
   }
#endif

#ifdef BUILD_ARC4
   if ( *( ptr++ ) )
   {
      ssl->decrypt.arc4 = ( Arc4* )XMALLOC( sizeof( Arc4 ), ssl->heap, DYNAMIC_TYPE_CIPHER );
      if ( ssl->decrypt.aes == NULL )
      {
         WOLFSSL_MSG( "Failed to allocate Arc4" );
         return MEMORY_E;
      }
      XMEMCPY( ssl->decrypt.aes, ptr, sizeof( Arc4 ) );
      ptr += sizeof( Arc4 );
   }
#endif

#ifdef BUILD_DES3
   if ( *( ptr++ ) )
   {
      ssl->decrypt.des3 = ( Des3* )XMALLOC( sizeof( Des3 ), ssl->heap, DYNAMIC_TYPE_CIPHER );
      if ( ssl->decrypt.aes == NULL )
      {
         WOLFSSL_MSG( "Failed to allocate Des3" );
         return MEMORY_E;
      }
      XMEMCPY( ssl->decrypt.aes, ptr, sizeof( Des3 ) );
      ptr += sizeof( Des3 );
   }
#endif

#ifdef BUILD_RABBIT
   if ( *( ptr++ ) )
   {
      ssl->decrypt.rabbit = ( Rabbit* )XMALLOC( sizeof( Rabbit ), ssl->heap, DYNAMIC_TYPE_CIPHER );
      if ( ssl->decrypt.aes == NULL )
      {
         WOLFSSL_MSG( "Failed to allocate Rabbit" );
         return MEMORY_E;
      }
      XMEMCPY( ssl->decrypt.aes, ptr, sizeof( Rabbit ) );
      ptr += sizeof( Rabbit );
   }
#endif

   ssl->decrypt.setup = 1;

   XMEMCPY( &ssl->session, ptr, sizeof( ssl->session ) );
   ptr += sizeof( ssl->session );

   XMEMCPY( &ssl->curRL, ptr, sizeof( ssl->curRL ) );
   ptr += sizeof( ssl->curRL );

   XMEMCPY( &ssl->msgsReceived, ptr, sizeof( ssl->msgsReceived ) );
   ptr += sizeof( ssl->msgsReceived );

   XMEMCPY( &ssl->version, ptr, sizeof( ssl->version ) );
   ptr += sizeof( ssl->version );

   XMEMCPY( &ssl->chVersion, ptr, sizeof( ssl->chVersion ) );
   ptr += sizeof( ssl->chVersion );

   XMEMCPY( &ssl->specs, ptr, sizeof( ssl->specs ) );
   ptr += sizeof( ssl->specs );

   XMEMCPY( &ssl->keys, ptr, sizeof( ssl->keys ) );
   ptr += sizeof( ssl->keys );

   XMEMCPY( &ssl->options, ptr, sizeof( ssl->options ) );
   ptr += sizeof( ssl->options );

   XMEMCPY( &ssl->dtls_timeout_init, ptr, sizeof( ssl->dtls_timeout_init ) );
   ptr += sizeof( ssl->dtls_timeout_init );

   XMEMCPY( &ssl->dtls_timeout_max, ptr, sizeof( ssl->dtls_timeout_max ) );
   ptr += sizeof( ssl->dtls_timeout_max );

   XMEMCPY( &ssl->dtls_timeout, ptr, sizeof( ssl->dtls_timeout ) );
   ptr += sizeof( ssl->dtls_timeout );

   return ret;
}

WOLFSSL_API WOLFSSL* wolfSSL_restore_session_with_window( WOLFSSL_CTX* ctx, byte* session, int sessionSize, byte* window, int windowSize )
{
   WOLFSSL* ssl = NULL;
   byte* ptr = NULL;
   int ret = SSL_ERROR_NONE;

   WOLFSSL_ENTER( "wolfSSL_restore_session" );

   ssl = wolfSSL_new( ctx );
   if ( ssl != NULL )
   {
      FreeHandshakeResources( ssl );

      ptr = XMALLOC( sessionSize, ssl->heap, DYNAMIC_TYPE_IN_BUFFER );
      if ( ptr == NULL )
      {
         wolfSSL_free( ssl );
         ssl = NULL;
         ret = MEMORY_E;
      }
      else
      {
         decryptBuffer( ptr, session, sessionSize );
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
            ret = restoreSSL( ssl, ptr );
            XFREE( ptr, ssl->heap, DYNAMIC_TYPE_IN_BUFFER );
            if ( ret == SSL_ERROR_NONE
                  && window != NULL )
            {
               int tempWindowOffset = 0;
               int tempWindowSize = 0;

               wolfSSL_get_window_offset( ssl, &tempWindowOffset, &tempWindowSize );

               byte* dst = ( byte * )ssl + tempWindowOffset;

               XMEMCPY( dst, window, windowSize );
            }
            else
            {
               wolfSSL_free( ssl );
               ssl = NULL;
            }
         }
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

WOLFSSL* wolfSSL_restore_session( WOLFSSL_CTX* ctx, void* mem, int sz )
{
   return wolfSSL_restore_session_with_window( ctx, mem, sz, NULL, 0 );
}




