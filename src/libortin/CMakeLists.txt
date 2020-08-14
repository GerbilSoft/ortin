# libortin: IS-NITRO management library
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(libortin LANGUAGES CXX)

# Sources.
SET(libortin_SRCS
	ISNitro.cpp
	)
# Headers.
SET(libortin_H
	ISNitro.hpp
	byteorder.h
	byteswap.h
	nitro-usb-cmds.h
	)

######################
# Build the library. #
######################

ADD_LIBRARY(ortin STATIC
	${libortin_SRCS} ${libortin_H}
	)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(ortin
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# libortin
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# libortin
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

# Exclude from ALL builds.
SET_TARGET_PROPERTIES(ortin PROPERTIES EXCLUDE_FROM_ALL TRUE)
# Make sure git_version.h is created before compiling this target.
IF(TARGET git_version)
	ADD_DEPENDENCIES(ortin git_version)
ENDIF(TARGET git_version)

# LibUSB is required.
FIND_PACKAGE(LibUSB 1.0 REQUIRED)
TARGET_LINK_LIBRARIES(ortin PUBLIC LibUSB::libusb)

# Unix: Add -fpic/-fPIC in order to use this static library in plugins.
IF(UNIX AND NOT APPLE)
	SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -fpic -fPIC")
	SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -fpic -fPIC")
ENDIF(UNIX AND NOT APPLE)
