/***************************************************************************
 * Ortin (IS-NITRO management) (ortintool)                                 *
 * avmode.hpp: 'avmode' command.                                           *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "avmode.hpp"
#include "ISNitro.hpp"

// C includes. (C++ namespace)
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>

/**
 * Parse an AV mode string.
 * @param av	[out] NitroAVModeScreen_t
 * @param str	[in] AV mode string.
 * @return 0 on success; non-zero on error.
 */
static int parseAVModeString(NitroAVModeScreen_t *av, const TCHAR *str)
{
	// TODO: Customizable spacing?
	av->spacing = 1;
	av->interlaced = false;
	av->aspect_ratio = true;

	if (!str || str[0] == '\0') {
		// Empty string. Assume no output.
		av->mode = NITRO_AV_MODE_OFF;
		return 0;
	}

	// First character should indicate None/Upper/Lower/Bottom.
	switch (_totupper(str[0])) {
		case 'N':
			av->mode = NITRO_AV_MODE_OFF;
			break;
		case 'U':
			av->mode = NITRO_AV_MODE_UPPER;
			break;
		case 'L':
			av->mode = NITRO_AV_MODE_LOWER;
			break;
		case 'B':
			av->mode = NITRO_AV_MODE_BOTH;
			break;
		default:
			return EINVAL;
	}

	// Remaining characters can specify aspect ratio and interlaced mode.
	for (str++; *str != '\0'; str++) {
		switch (_totupper(*str)) {
			case 'A':
				av->aspect_ratio = false;
				break;
			case 'I':
				av->interlaced = true;
				break;
			default:
				return EINVAL;
		}
	}

	return 0;
}

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
	uint32_t bg_color, NitroAVDeflicker_e deflicker, NitroAVRotation_e rotation)
{
	NitroAVModeSettings_t avSettings;
	memset(&avSettings, 0, sizeof(avSettings));

	// Parse av1/av2.
	int ret = parseAVModeString(&avSettings.av[0], av1);
	if (ret != 0) {
		_ftprintf(stderr, _T("*** ERROR: Invalid AV1 mode string: '%s'\n"), av1);
		return ret;
	}
	ret = parseAVModeString(&avSettings.av[1], av2);
	if (ret != 0) {
		_ftprintf(stderr, _T("*** ERROR: Invalid AV2 mode string: '%s'\n"), av2);
		return ret;
	}

	// Other settings.
	avSettings.bg_color = bg_color;
	avSettings.deflicker = deflicker;
	avSettings.rotation = rotation;

	// Apply the AV settings.
	ret = nitro->setAVModeSettings(&avSettings);
	if (ret < 0) {
		fprintf(stderr, "*** ERROR: Failed to set AV mode: %d\n", ret);
	}
	return ret;
}
