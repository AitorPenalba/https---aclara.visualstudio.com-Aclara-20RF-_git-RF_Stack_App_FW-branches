/**
 * Implements suspend and restore of DTLS sessions
 */

#include <stddef.h>

#include <wolfssl/quicksession.h>

static const byte key[] = { 0x7b, 0xcc, 0x60, 0x9f, 0x2c, 0x1e, 0x30, 0xd4,
                            0x12, 0x19, 0x39, 0x51, 0x92, 0x84, 0xa5, 0x93,
                            0x33, 0x8d, 0x91, 0x7f, 0x92, 0x0f, 0x20, 0x74,
                            0x9c, 0x44, 0xbf, 0xd9, 0x11, 0x89, 0x59, 0x5a
                          };

static const byte iv[] = { 0x56, 0x70, 0x8f, 0x5e, 0x11, 0x4b, 0x11, 0x69,
                           0x9d, 0x3e, 0x88, 0xaa, 0xb1, 0x3e, 0x92, 0x3c
                         };


/***********************************************************************************************************************
 *
 * Function name: encryptBuffer
 *
 * Purpose: Encrypt the buffer with AES-CBC mode, which requires block padding before pass-in.
 *
 * Arguments:	byte*	dest	- encrypt destinaion
 *				byte*	src		- encrypt soure
 *				int		size	- soure size, must be multiple integer size of AES block size.
 *
 * Returns: None
 ******************************************************************************************************************** */
static void encryptBuffer( byte* dest, byte* src, int size )
{
   Aes aes;

   wc_AesSetKey( &aes, key, sizeof( key ), iv, AES_ENCRYPTION );
   wc_AesCbcEncrypt( &aes, dest, src, size );
}

/***********************************************************************************************************************
 *
 * Function name: decryptBuffer
 *
 * Purpose: Decrypt the buffer with AES-CBC mode, which requires block padding before pass-in.
 *
 * Arguments:	byte*	dest	- decrypt destinaion
 *				byte*	src		- decrypt soure
 *				int		size	- soure size, must be multiple integer size of AES block size.
 *
 * Returns: None
 ******************************************************************************************************************** */
static void decryptBuffer( byte* dest, byte* src, int size )
{
   Aes aes;

   wc_AesSetKey( &aes, key, sizeof( key ), iv, AES_DECRYPTION );
   wc_AesCbcDecrypt( &aes, dest, src, size );
}

/***********************************************************************************************************************
 *
 * Function name: generateMD5
 *
 * Purpose: Generate MD5 at the end of buffer.
 *
 * Arguments:	byte*	buf		- buffer
 *				int		size	- buffer size.
 *
 * Returns: None
 ******************************************************************************************************************** */
static void generateMD5( byte* buf, int size )
{
   int dataSize = size - MD5_DIGEST_SIZE;
   wc_Md5Hash( buf, dataSize, buf + dataSize );
}

/***********************************************************************************************************************
 *
 * Function name: validateMD5
 *
 * Purpose: Verify buffer data integration with the MD5 sotred at the end of buffer.
 *
 * Arguments: bool
 *
 * Returns: None
 ******************************************************************************************************************** */
static bool validateMD5( byte* buf, int size )
{
   byte actual[MD5_DIGEST_SIZE];
   int dataSize = size - MD5_DIGEST_SIZE;
   bool result;

   wc_Md5Hash( buf, dataSize, actual );

   result = XMEMCMP( buf + dataSize, actual, MD5_DIGEST_SIZE );
   return !result;
}



/***********************************************************************************************************************
 *
 * Function name: wolfSSL_export_session_state
 *
 * Purpose: This function is called to copy the "window" (ssl session state) to a buffer. 
 *
 * Arguments: WOLFSSL *ssl                  - pointer to the WOLFSSL session to save
 *			  void* buf
 *			  int sz
 *
 * Returns: int size of serialized window
 *				< 0		wolfSSL Errors
 *				= 0		SSL_ERROR_NONE, no action; inputs may has a Null pointer.
 *				> 0		the size of serialized session on success 
 ******************************************************************************************************************** */
int wolfSSL_export_session_state( WOLFSSL* ssl, void* buf, int sz )
{
   int ret;
   
   ret = wolfSSL_dtls_export_state_only(ssl, buf, (unsigned int*)&sz);
   if (ret < 0) 
   {
       return ret;
   }
   if (ret == 0)
   {
	   return SSL_ERROR_NONE;
   }

   return ret;
}



/***********************************************************************************************************************
 *
 * Function name: wolfSSL_export_session
 *
 * Purpose: This function is called to save the ssl session into a buffer. 
 *
 * Arguments:	WOLFSSL *ssl	- pointer to the WOLFSSL session to save
 *				void*	buf		- pointer to a buffer to save
 *				int		sz		- size of the buffer
 *
 * Returns: int size of serialized session
 *				< 0		wolfSSL Errors
 *				= 0		SSL_ERROR_NONE, no action;
 *				> 0		return the size of serialized session on success 
 *						(currently we encypted the max_size session + padding + MD5)
 *
 * Note:	Serialized Format
 *			| ... serialized session ...| ... Padding 0s (if have) ... | ... MD5 (from start to end of padding) ... |
 *			| <-----								AES-CBC Encrypted										 -----> |
 ******************************************************************************************************************** */
int wolfSSL_export_session( WOLFSSL* ssl, void* buf, int sz )
{
   int ret;
   word32 data_size = 0;

   ret = wolfSSL_dtls_export(ssl, buf, (unsigned int*)&sz);
   if (ret < 0) 
   {
       return ret;
   }
   if (ret == 0) 
   {
	   return SSL_ERROR_NONE;
   }

   /* Calculate buffer Size needed */
   data_size += ret;
   data_size += MD5_DIGEST_SIZE;
   if (data_size % AES_BLOCK_SIZE != 0)
   {
	   /* Padding to Align the Block size */
	   data_size += (AES_BLOCK_SIZE - (data_size % AES_BLOCK_SIZE));
   }

   if (data_size > sz)
   {
	   return -1;	// buffer too small, data may overflow
   }

   generateMD5((byte*)buf, data_size);
   encryptBuffer( ( byte* )buf, buf, sz );
   WOLFSSL_LEAVE( "wolfSSL_export_session", SSL_ERROR_NONE );
   return data_size;
}

/***********************************************************************************************************************
 *
 * Function name: wolfSSL_restore_session_with_window
 *
 * Purpose: This function is called to restore the ssl session. 
 *
 * Arguments:	WOLFSSL_CTX*	ctx							- pointer to the current WOLFSSL context
 *				byte*			last_saved_session			- pointer to a new WOLFSSL session to save
 *				int				saved_session_sz
 *				byte*			last_saved_session_state
 *				int				saved_state_sz              
 *
 * Returns:  WOLFSSL* ssl
 ******************************************************************************************************************** */
WOLFSSL_API WOLFSSL* wolfSSL_restore_session_with_window( WOLFSSL_CTX* ctx, byte* last_saved_session, int saved_session_sz, byte* last_saved_session_state, int saved_state_sz)
{
   WOLFSSL* ssl = NULL;
   byte* ptr = NULL;
   int ret = SSL_ERROR_NONE;
   void* heap = NULL;

   if (ctx == NULL) {
       return NULL;
   }
   heap = wolfSSL_CTX_GetHeap(ctx, NULL);

   WOLFSSL_ENTER( "wolfSSL_restore_session" );

   ssl = wolfSSL_new( ctx );
   if ( ssl != NULL )
   {
      ptr = XMALLOC(saved_session_sz, heap, DYNAMIC_TYPE_IN_BUFFER );
      if ( ptr == NULL )
      {
         wolfSSL_free( ssl );
         ssl = NULL;
         ret = MEMORY_E;
      }
      else
      {
         decryptBuffer( ptr, last_saved_session, saved_session_sz);
         if ( !validateMD5( ptr, saved_session_sz) )
         {
            WOLFSSL_MSG( "MD5 digest validation failed." );
            XFREE( ptr, heap, DYNAMIC_TYPE_IN_BUFFER );
            wolfSSL_free( ssl );
            ssl = NULL;
            ret = MEMORY_E;
         }
         else
         {
            ret = SSL_ERROR_NONE;
            if (wolfSSL_dtls_import(ssl, ptr, saved_session_sz) < 0) {
                ret = -1;
            }

            XFREE( ptr, heap, DYNAMIC_TYPE_IN_BUFFER );
            if ( ret == SSL_ERROR_NONE
                  && last_saved_session_state != NULL )
            {
                if (wolfSSL_dtls_import(ssl, last_saved_session_state, saved_state_sz) < 0) {
                    ret = -1;
                }
                else {
                    ret = SSL_ERROR_NONE;
                }
            }

            if (ret != SSL_ERROR_NONE)
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

