/***************************************************************************
 * Ortin (IS-NITRO management) (ortin CLI)                                 *
 * avmode.hpp: 'avmode' command.                                           *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ORTIN_ORTIN_AVMODE_HPP__
#define __ORTIN_ORTIN_AVMODE_HPP__

#include "tcharx.h"
#include "nitro-usb-cmds.h"

class ISNitro;

/**
 * Set the IS-NITRO's AV mode.
 * @param nitro IS-NITRO object.
 * @param av1 AV1 mode string.
 * @param av2 AV2 mode string.
 * @param bg_color Background color.
 * @param deflicker Deflicker setting.
 * @param rotation Rotation setting.
 * @return 0 on success; non-zero on error.
 */
int set_av_mode(ISNitro *nitro, const TCHAR *av1, const TCHAR *av2,
	uint32_t bg_color, NitroAVDeflicker_e deflicker, NitroAVRotation_e rotation);

#endif /* __ORTIN_ORTIN_AVMODE_HPP__ */
