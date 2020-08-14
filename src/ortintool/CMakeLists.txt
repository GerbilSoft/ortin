# Ortin command line tool
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(ortintool LANGUAGES C CXX)

# Sources.
SET(ortintool_SRCS
	main.cpp
	)
# Headers.
SET(ortintool_H
	)

#########################
# Build the executable. #
#########################

ADD_EXECUTABLE(ortintool
	${ortintool_SRCS} ${ortintool_H}
	)
SET_TARGET_PROPERTIES(ortintool PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(ortintool)
SET_WINDOWS_SUBSYSTEM(ortintool CONSOLE)
SET_WINDOWS_NO_MANIFEST(ortintool)

# FIXME: setargv isn't necessarily sorted...
# TODO: Implement our own setargv function?
# TODO: Unicode support.
SET_WINDOWS_ENTRYPOINT(ortintool main ON)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(ortintool
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# ortintool
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# ortintool
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

TARGET_LINK_LIBRARIES(ortintool PRIVATE ortin)
# TODO: getopt_msvc
#IF(MSVC)
#	TARGET_LINK_LIBRARIES(ortin PRIVATE getopt_msvc)
#ENDIF(MSVC)

# CMake-3.7.2 doesn't add include paths to windres.
IF(MINGW)
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_SOURCE_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_CURRENT_BINARY_DIR}/..\"")
	SET(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I \"${CMAKE_BINARY_DIR}\"")
ENDIF(MINGW)

#################
# Installation. #
#################

INCLUDE(DirInstallPaths)

# NOTE: Don't install libraries.
# That installs the import library, which isn't used
# for shell extensions.
INSTALL(TARGETS ortintool
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME ortintool PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
