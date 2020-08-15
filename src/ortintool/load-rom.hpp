/***************************************************************************
 * Ortin (IS-NITRO management) (ortintool)                                 *
 * load-rom.hpp: 'load' command.                                           *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ORTIN_ORTINTOOL_LOAD_ROM_HPP__
#define __ORTIN_ORTINTOOL_LOAD_ROM_HPP__

#include "tcharx.h"

class ISNitro;

/**
 * Load a Nintendo DS ROM image.
 * @param nitro IS-NITRO object.
 * @param filename ROM image filename.
 * @return 0 on success; non-zero on error.
 */
int load_nds_rom(ISNitro *nitro, const TCHAR *filename);

#endif /* __ORTIN_ORTINTOOL_LOAD_ROM_HPP__ */
