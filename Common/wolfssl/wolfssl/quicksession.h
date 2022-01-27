/***********************************************************************************************************************
 *
 * Filename: quicksession.h
 *
 * Contents: This file has been added to WolfSSL. Defines the APIs for suspending and restoring a connection.
 *
 ***********************************************************************************************************************
 * Copyright (c) 2018 Aclara Power-Line Systems Inc.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *                ACLARA POWER-LINE SYSTEMS INC.
 *                ST. LOUIS, MISSOURI USA
 ***********************************************************************************************************************
 * Revision History:
 *
 **********************************************************************************************************************/

#ifndef QUICKSESSION_H
#define QUICKSESSION_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdbool.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/md5.h>
#include <wolfssl/error-ssl.h>

/* ****************************************************************************************************************** */
/* CONSTANT DEFINITIONS */

#define DTLS_SESSION_MAJOR_MAX_SIZE 	( ( int )MAX_EXPORT_BUFFER + MD5_DIGEST_SIZE + ( int )AES_BLOCK_SIZE)	// Major Session is encrypted by AES, leave extra space for AES block alignment
#define DTLS_SESSION_MINOR_MAX_SIZE 	(MAX_EXPORT_STATE_BUFFER)

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
WOLFSSL_API int32_t wolfSSL_suspend_session( WOLFSSL *ssl, void *mem, int32_t sz );
WOLFSSL_API WOLFSSL *wolfSSL_restore_session( WOLFSSL_CTX *ctx, void *mem, int32_t sz );
WOLFSSL_API int32_t wolfSSL_export_session( WOLFSSL *, void *, int32_t );
WOLFSSL_API int32_t wolfSSL_export_session_state( WOLFSSL *, void *, int32_t );
WOLFSSL_API WOLFSSL *wolfSSL_restore_session_with_window( WOLFSSL_CTX *ctx, uint8_t *session, int32_t sessionSize, uint8_t *window, int32_t windowSize );
WOLFSSL_API WOLFSSL *wolfSSL_restore_session(WOLFSSL_CTX *, void *, int32_t);
int32_t     wolfSSL_get_suspend_memsize( WOLFSSL *ssl );
bool        QUICK_sessionEncrypted( void );
#endif
