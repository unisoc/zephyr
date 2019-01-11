/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal configuration for PBKDF2 for IEEE 802.11i.
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* System support */
#define MBEDTLS_PLATFORM_C

#if !defined(CONFIG_ARM)
#define MBEDTLS_HAVE_ASM
#endif

/* mbed TLS feature support */

/* mbed TLS modules */
#define MBEDTLS_MD_C
#define MBEDTLS_PKCS5_C
#define MBEDTLS_SHA1_C

#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */
