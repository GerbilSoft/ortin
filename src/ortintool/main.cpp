/***************************************************************************
 * Ortin (IS-NITRO management) (ortintool)                                 *
 * main.cpp: Command line interface.                                       *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.version.h"
#include "git.h"

// C includes.
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>

// C++ includes. (C namespace)
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <locale>

// libusb
#include <libusb.h>

// IS-NITRO
#include "ISNitro.hpp"

// Commands
#include "load-rom.hpp"
#include "avmode.hpp"

#include "tcharx.h"
#ifdef _MSC_VER
# define ORTIN_CDECL __cdecl
#else
# define ORTIN_CDECL
#endif

// FIXME: gcc doesn't support printf attributes for wide strings.
#if defined(__GNUC__) && !defined(_WIN32)
# define ATTR_PRINTF(fmt, args) __attribute__ ((format (printf, (fmt), (args))))
#else
# define ATTR_PRINTF(fmt, args)
#endif

/**
 * Print an error message.
 * @param argv0 Program name.
 * @param fmt Format string.
 * @param ... Arguments.
 */
static void ATTR_PRINTF(2, 3) print_error(const TCHAR *argv0, const TCHAR *fmt, ...)
{
	if (fmt != NULL) {
		va_list ap;
		va_start(ap, fmt);
		_ftprintf(stderr, _T("%s: "), argv0);
		_vftprintf(stderr, fmt, ap);
		va_end(ap);

		fputc('\n', stderr);
	}

	_ftprintf(stderr, _T("Try `%s` --help` for more information.\n"), argv0);
}

/**
 * Print program help.
 * @param argv0 Program name.
 */
static void print_help(const TCHAR *argv0)
{
	fputs("This program is licensed under the GNU GPL v2.\n"
		"For more information, visit: http://www.gnu.org/licenses/\n"
		"\n", stdout);

	fputs("Syntax: ", stdout);
	_fputts(argv0, stdout);
	fputs(" [options] [command]\n"
		"\n"
		"Supported commands:\n"
		"\n"
		"load filename.nds\n"
		"- Load a Nintendo DS ROM image. If the image has a decrypted secure area,\n"
		"  it will be re-encrypted on load.\n"
		"\n"
		"avmode av1 av2 [--bgcolor=COLOR] [--deflicker=DEFLICKER]\n"
		"- Set the AV mode settings. av1/av2 can be one of the following\n"
		"  primary mode characters:\n"
		"  - N: No image. Disables the output entirely.\n"
		"  - U: Upper screen image.\n"
		"  - L: Lower screen image.\n"
		"  - B: Both screen images, stacked on top of each other.\n"
		"  The following additional characters can be provided as modifiers:\n"
		"  - I: Use interlaced output.\n"
		"  - A: Do not use the correct aspect ratio.\n"
		"\n"
		"help\n"
		"- Display this help and exit.\n"
		"\n"
		"Options:\n"
		"\n"
		"  -b, --bgcolor=COLOR       Specify a custom background color. (24-bit hex)\n"
		"                            Example: FF8000 - default is black (000000)\n"
		"  -d, --deflicker=DEFLICKER Deflicker mode: none, normal, alternate.\n"
		"                            Default is none.\n"
		, stdout);
}

int ORTIN_CDECL _tmain(int argc, TCHAR *argv[])
{
	// Set the C and C++ locales.
	std::locale::global(std::locale(""));

	puts("Ortin Tool v" VERSION_STRING "\n"
		"Copyright (c) 2020 by David Korth.\n"
		"This program is NOT licensed or endorsed by Nintendo Co, Ltd."
	);
#ifdef RP_GIT_VERSION
	puts(RP_GIT_VERSION);
# ifdef RP_GIT_DESCRIBE
	puts(RP_GIT_DESCRIBE);
# endif
#endif
	putchar('\n');

	// avmode options.
	uint32_t bg_color = 0;
	NitroAVDeflicker_e deflicker = NITRO_AV_DEFLICKER_DISABLED;

	// TODO: Allow customization once we figure out how to get
	// rotation set up properly.
	NitroAVRotation_e rotation = NITRO_AV_ROTATION_NONE;

	while (true) {
		static const struct option long_options[] = {
			{_T("bgcolor"),		required_argument,	0, _T('b')},
			{_T("deflicker"),	required_argument,	0, _T('d')},
			{_T("help"),		no_argument,		0, _T('h')},

			{NULL, 0, 0, 0}
		};

		int c = getopt_long(argc, argv, _T("b:d:h"), long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case _T('b'): {
				// Background color.
				if (!optarg || optarg[0] == '\0') {
					// NULL?
					print_error(argv[0], _T("no background color specified"));
					return EXIT_FAILURE;
				}

				char *endptr = nullptr;
				bg_color = strtoul(optarg, &endptr, 16);
				if (*endptr != '\0') {
					print_error(argv[0], _T("background color is invalid (should be 24-bit hex)"));
					return EXIT_FAILURE;
				}
				break;
			}

			case _T('d'):
				// Deflicker.
				if (!optarg || optarg[0] == '\0') {
					// NULL?
					print_error(argv[0], _T("no deflicker mode specified"));
					return EXIT_FAILURE;
				}

				if (_tcsicmp(optarg, _T("none"))) {
					deflicker = NITRO_AV_DEFLICKER_DISABLED;
				} else if (_tcsicmp(optarg, _T("normal"))) {
					deflicker = NITRO_AV_DEFLICKER_NORMAL;
				} else if (_tcsicmp(optarg, _T("alternate")) ||
					   _tcsicmp(optarg, _T("alt")))
				{
					deflicker = NITRO_AV_DEFLICKER_ALTERNATE;
				} else {
					print_error(argv[0], _T("deflicker mode is invalid"));
					return EXIT_FAILURE;
				}
				break;

			case _T('h'):
				print_help(argv[0]);
				return EXIT_SUCCESS;

			case _T('?'):
			default:
				print_error(argv[0], NULL);
				return EXIT_FAILURE;
		}
	}

	// First argument after getopt-parsed arguments is set in optind.
	if (optind >= argc) {
		print_error(argv[0], _T("no parameters specified"));
		return EXIT_FAILURE;
	}

	int status = libusb_init(nullptr);
	if (status < 0) {
		fprintf(stderr, "*** ERROR: libusb_init() failed: %s\n", libusb_error_name(status));
		return EXIT_FAILURE;
	}

	ISNitro *nitro = new ISNitro();
	if (!nitro->isOpen()) {
		fprintf(stderr, "*** ERROR: Unable to open the IS-NITRO unit.\n");
		libusb_exit(nullptr);
		return EXIT_FAILURE;
	}

	// Check the specified command.
	// TODO: Better help if the command parameters are invalid.
	int ret = 0;
	if (!_tcscmp(argv[optind], _T("help"))) {
		// Display help.
		print_help(argv[0]);
		return EXIT_SUCCESS;
	} else if (!_tcscmp(argv[optind], _T("load"))) {
		// Load a ROM image.
		if (argc < optind+2) {
			print_error(argv[0], _T("Nintendo DS ROM image not specified"));
			ret = EXIT_FAILURE;
		} else {
			ret = load_nds_rom(nitro, argv[optind+1]);
		}
	} else if (!_tcscmp(argv[optind], _T("avmode"))) {
		// Set the AV mode.
		if (argc < optind+3) {
			print_error(argv[0], _T("AV mode parameters not specified"));
			ret = EXIT_FAILURE;
		} else {
			ret = set_av_mode(nitro, argv[optind+1], argv[optind+2], bg_color, deflicker, rotation);
		}
	}

	delete nitro;
	libusb_exit(nullptr);
	return ret;
}
