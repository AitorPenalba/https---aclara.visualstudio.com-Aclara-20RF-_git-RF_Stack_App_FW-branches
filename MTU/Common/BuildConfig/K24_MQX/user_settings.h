#include "CompileSwitch.h"

/* System Settings */
#define NO_MAIN_DRIVER              /*  */
#define BENCH_EMBEDDED              /*  */
#define SINGLE_THREADED             /* Since ONLY the NWK task will call library */
#define NO_FILESYSTEM               /* To use buffers instead of files - need to use xxx_buffers() instead of xxx_files() */
#define NO_WRITEV                   /*  */
#define WOLFSSL_USER_IO             /*  */
#define NO_DEV_RANDOM               /* RNG Gen Seed user function - this is redundant since Freescale MQX also defines it */
#define USE_CERT_BUFFERS_2048       /* Enables 2048-bit test certificate and key buffers */
#define WOLFSSL_USER_CURRTIME       /* Not used */
#define NO_WOLF_C99                 /* This must be defined in order to use the mqx.h gmtime_t() function */
#define WOLFSSL_DTLS_ALLOW_FUTURE   /* Allow time difference into future */

// Not need if defined FREESCALE_MQX
#define SIZEOF_LONG_LONG            8


/* Cipher Select */
#define WOLFSSL_DTLS                /* Enables DTLS */
#define HAVE_ECC                    /* Enables Elliptical Curve Cryptography support */
#define HAVE_AESCCM                 /* Enables AES-CCM support */


/* Crypto Settings */
// - Memory Optimization
// Not need if defined FREESCALE_MQX
#define USE_FAST_MATH			      /* Ming - added // need add tfm.c to the wolfSSL build instead of integer.c
                                       Switches the big integer library to a faster one that uses assembly if possible */
#define TFM_TIMING_RESISTANT        /* This will get rid of the large static arrays */
#define ECC_TIMING_RESISTANT  	   /* Deter against timing based, side-channel attacks for ECC */

#define ALT_ECC_SIZE			         /* Reduce ECC memory consumption. Instead of using stack for ECC points it will allocate from the heap. Must define USE_FAST_MATH */
#define WOLFSSL_SMALL_STACK 	      /* Increases use of dynamic memory, but can lead to slower performance */
#define SMALL_SESSION_CACHE         /* Reduce the default session cache from 33 sessions to 6 sessions and save approximately 2.5 kB */
#define FP_MAX_BITS (512)		      /* Fastmath max suport 512 bits integer or 256 bits x 256 bits multiplications. */
#define POSITIVE_EXP_ONLY           /* Reduces stack usage if only positive Floating point numbers */

// - Customization
#define HAVE_PK_CALLBACKS           /* Enable Public Key callbacks */
#define WOLFSSL_ALWAYS_VERIFY_CB    /*  */
#define PERSIST_SESSION_CACHE       /* Enable persistent Session Cache functions */
#define KEEP_PEER_CERT              /*  */
#define CUSTOM_RAND_GENERATE_SEED   DtlsGenerateSeed    /* User defined function for wc_GenerateSeed() */
#define WOLFSSL_SESSION_EXPORT      /* Enable use of DTLS session export and import */
#define WOLFSSL_SESSION_EXPORT_NOPEER  /* Disable export of Peer Session Info */

#if ( DTLS_DEBUG == 1 )             /* Only enable if application is turned on*/
#define DEBUG_WOLFSSL		         /* Enables debug print, requires MQX fio.h support for stderr */
#define WOLFSSL_DEBUG_MEMORY        /* Enables extra function and line number args for memory callbacks */
#else
#undef DEBUG_WOLFSSL
#undef WOLFSSL_DEBUG_MEMORY
#endif
#define USE_WOLFSSL_MEMORY          /*  */
#define NO_STDIO_FILESYSTEM         /*  */
#define WOLFSSL_EXTRA               /*  */

// #define OPENSSL_EXTRA_X509_SMALL

/* Exclude Building */
#define NO_WOLFSSL_SERVER           /* Removes calls specific to the server side (was improperly #define NO_WOLF_SERVER -save ~10kB code) */
#define NO_RSA                      /* Not using */
#define NO_DES3                     /* Not using */
#define NO_DSA                      /* Not using */
#define NO_MD4                      /* Not using */
#define NO_PSK                      /* Not using */
#define NO_PWDBASED                 /* Not using */
#define NO_RC4                      /* Not using */
#define NO_HC128                    /* Not using */
#define NO_OLD_TLS                  /* Not using */

/* What about:
NO_RABBIT
WC_NO_RSA_OAEP
*/
/* For Integrate to wolfSSL4.3.0 */
#define HAVE_IDEA
