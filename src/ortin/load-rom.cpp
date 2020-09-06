/***************************************************************************
 * Ortin (IS-NITRO management) (ortin CLI)                                 *
 * load-rom.cpp: 'load' command.                                           *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "load-rom.hpp"
#include "ISNitro.hpp"
#include "ndscrypt.hpp"

// C includes. (C++ namespace)
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>

/**
 * Load a Nintendo DS ROM image.
 * @param nitro IS-NITRO object.
 * @param filename ROM image filename.
 * @return 0 on success; non-zero on error.
 */
int load_nds_rom(ISNitro *nitro, const TCHAR *filename)
{
	errno = 0;
	FILE *f = _tfopen(filename, "rb");
	if (!f) {
		int err = errno;
		if (err == 0)
			err = EIO;
		fprintf(stderr, "*** ERROR opening '%s': %s\n", filename, strerror(err));
		return err;
	}

	fseeko(f, 0, SEEK_END);
	off64_t fileSize = ftello(f);
	rewind(f);
	if (fileSize > 256*1024*1024) {
		fprintf(stderr, "*** ERROR: ROM image '%s' is larger than 256 MB.\n", filename);
		fclose(f);
		return ENOMEM;
	}

	static const size_t BUF_SIZE = 1048576U;
	uint8_t *const buf1mb = static_cast<uint8_t*>(malloc(BUF_SIZE));

	// Reset the IS-NITRO while loading a ROM image.
	nitro->fullReset();
	nitro->ndsReset(true);
	nitro->setSlotPower(1, false);

	// Load 1 MB at a time.
	uint32_t address = 0;
	bool firstMB = true;
	while (fileSize > 0) {
		uint32_t curlen = std::min(fileSize, (off64_t)BUF_SIZE);
		errno = 0;
		size_t size = fread(buf1mb, 1, curlen, f);
		if ((off64_t)size != curlen) {
			// Short read...
			int err = errno;
			if (err == 0)
				err = EIO;
			fprintf(stderr, "*** ERROR: Short read.\n");
			// Remove IS-NITRO from reset anyway.
			nitro->ndsReset(false);
			return err;
		}
		fileSize -= curlen;

		if (firstMB) {
			// We may need to encrypt the secure area.
			ndscrypt_encrypt_secure_area(buf1mb, curlen);
			firstMB = false;
		}

		if (curlen % 2 != 0) {
			// Round it up to a multiple of two bytes.
			buf1mb[curlen] = 0xFF;
			curlen++;
		}
		// Write to the emulation memory.
		nitro->writeEmulationMemory(1, address, buf1mb, curlen);
		address += curlen;
	}

	// Install the debugger ROM.
	nitro->installDebuggerROM();

	// ROM image loaded!
	// Slot power must be turned on in order to access save memory.
	nitro->setSlotPower(1, true);
	nitro->ndsReset(false);

	// Wait for the debugger ROM to initialize.
	int ret = nitro->waitForDebuggerROM();
	if (ret < 0)
		return ret;

	// LibISNitroEmulator sends cmd174 to both CPUs here.
	ret = nitro->sendCpuCMD174(NITRO_CPU_ARM9);
	if (ret < 0)
		return ret;
	ret = nitro->sendCpuCMD174(NITRO_CPU_ARM7);
	if (ret < 0)
		return ret;

	// Start the ARM9 and ARM7 CPUs.
	// (Official debugger ROM requires this; NitroDriver's ROM does not.)
	nitro->continueProcessor(0);
	nitro->continueProcessor(1);
	return 0;
}
