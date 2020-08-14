/***************************************************************************
 * Ortin (IS-NITRO management) (libortin)                                  *
 * ndscrypt.hpp: Nintendo DS encryption.                                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ORTIN_LIBORTIN_NDSCRYPT_HPP__
#define __ORTIN_LIBORTIN_NDSCRYPT_HPP__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encrypt the ROM's Secure Area, if necessary.
 * @param pRom First 32 KB of the ROM image.
 * @param len Length of pRom.
 * @return 0 on success; non-zero on error.
 */
int ndscrypt_secure_area(uint8_t *pRom, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __ORTIN_LIBORTIN_NDSCRYPT_HPP__ */
