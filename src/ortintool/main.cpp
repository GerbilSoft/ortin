/***************************************************************************
 * Ortin (IS-NITRO management) (ortintool)                                 *
 * main.cpp: Command line interface.                                       *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// C includes.
#include <stdint.h>
#include <stdlib.h>

// C++ includes. (C namespace)
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>

// libusb
#include <libusb.h>

// IS-NITRO
#include "ISNitro.hpp"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "syntax: %s nds-rom.nds\n", argv[0]);
		fprintf(stderr, "NDS ROM must have S-Boxes and encrypted secure area.\n");
		return EXIT_FAILURE;
	}

	int status = libusb_init(nullptr);
	if (status < 0) {
		fprintf(stderr, "libusb_init() failed: %s\n", libusb_error_name(status));
		return EXIT_FAILURE;
	}

	ISNitro *nitro = new ISNitro();
	if (!nitro->isOpen()) {
		fprintf(stderr, "Unable to open the IS-NITRO unit.\n");
	}

	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "*** ERROR opening '%s': %s\n", argv[1], strerror(errno));
		libusb_exit(nullptr);
		return EXIT_FAILURE;
	}

	fseeko(f, 0, SEEK_END);
	off64_t fileSize = ftello(f);
	rewind(f);
	if (fileSize > 256*1024*1024) {
		fprintf(stderr, "*** ERROR: ROM image '%s' is larger than 256 MB.\n", argv[1]);
		fclose(f);
		libusb_exit(nullptr);
		return EXIT_FAILURE;
	}

	// Set AV1 to Both screens, interlaced.
	// Set AV2 to none.
	int ret = nitro->setAVMode(NITRO_AV_MODE_BOTH, NITRO_AV_MODE_OFF, true, false);

#if 0
	static const size_t BUF_SIZE = 1048576U;
	uint8_t *const buf1mb = static_cast<uint8_t*>(malloc(BUF_SIZE));

	// Reset the IS-NITRO while loading a ROM image.
	nitro->fullReset();
	nitro->ndsReset(true);
	nitro->setSlotPower(1, false);

	// Load 1 MB at a time.
	int ret = 0;
	uint32_t address = 0;
	while (fileSize > 0) {
		uint32_t curlen = std::min(fileSize, (off64_t)BUF_SIZE);
		size_t size = fread(buf1mb, 1, curlen, f);
		if ((off64_t)size != curlen) {
			// Short read...
			fprintf(stderr, "*** ERROR: Short read.\n");
			// Remove IS-NITRO from reset anyway.
			nitro->ndsReset(false);
			ret = EXIT_FAILURE;
			goto out;
		}
		fileSize -= curlen;

		if (curlen % 2 != 0) {
			// Round it up to a multiple of two bytes.
			buf1mb[curlen] = 0xFF;
			curlen++;
		}
		// Write to the emulation memory.
		nitro->writeEmulationMemory(1, address, buf1mb, curlen);
		address += curlen;
	}

	// ROM image loaded!
	// NOTE: Slot power seems to be for PC-side access to the real slot...
	//nitro->setSlotPower(1, true);
	nitro->ndsReset(false);
#endif

out:
	delete nitro;
	libusb_exit(nullptr);
	return ret;
}