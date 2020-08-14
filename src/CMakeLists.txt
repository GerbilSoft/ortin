CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(src)

# Check for Large File Support.
# NOTE: This should have been done in platform.cmake, but we're
# including it here because the off_t/off64_t sizes are written
# to config.libc.h.
INCLUDE(CheckLargeFileSupport)
CHECK_LARGE_FILE_SUPPORT()

# Write the libc configuration file.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.libc.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.libc.h")

# C++11 compatibility header.
# NOTE: This must be included regardless of C++11 support in the compiler.
# gcc-4.6 supports some C++11, but is missing explicit virtual overrides.
IF(MSVC)
	SET(RP_CONFIG_LIBC_H "-FI${CMAKE_CURRENT_BINARY_DIR}/config.libc.h")
	SET(RP_CXX11_COMPAT_H "-FI${CMAKE_CURRENT_SOURCE_DIR}/c++11-compat.h")
ELSE(MSVC)
	SET(RP_CONFIG_LIBC_H "-include ${CMAKE_CURRENT_BINARY_DIR}/config.libc.h")
	SET(RP_CXX11_COMPAT_H "-include ${CMAKE_CURRENT_SOURCE_DIR}/c++11-compat.h")
ENDIF(MSVC)

# Add the C++11 compatibility header to CFLAGS/CXXFLAGS.
SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} ${RP_CONFIG_LIBC_H} ${RP_CXX11_COMPAT_H}")
SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${RP_CONFIG_LIBC_H} ${RP_CXX11_COMPAT_H}")

# Check for hidden visibility flags.
INCLUDE(CheckHiddenVisibility)
CHECK_HIDDEN_VISIBILITY()

# Write the version number to config.version.h.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.version.h")

# Source Code subdirectories.
ADD_SUBDIRECTORY(libortin)
ADD_SUBDIRECTORY(ortintool)
