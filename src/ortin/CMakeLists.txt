# Ortin command line tool
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
CMAKE_POLICY(SET CMP0048 NEW)
IF(POLICY CMP0063)
	# CMake 3.3: Enable symbol visibility presets for all
	# target types, including static libraries and executables.
	CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)
PROJECT(ortin LANGUAGES C CXX)

# Sources.
SET(ortin_SRCS
	main.cpp
	load-rom.cpp
	avmode.cpp
	)
# Headers.
SET(ortin_H
	load-rom.hpp
	avmode.hpp
	)

#########################
# Build the executable. #
#########################

ADD_EXECUTABLE(ortin
	${ortin_SRCS} ${ortin_H}
	)
SET_TARGET_PROPERTIES(ortin PROPERTIES PREFIX "")
DO_SPLIT_DEBUG(ortin)
SET_WINDOWS_SUBSYSTEM(ortin CONSOLE)
SET_WINDOWS_NO_MANIFEST(ortin)

# FIXME: setargv isn't necessarily sorted...
# TODO: Implement our own setargv function?
SET_WINDOWS_ENTRYPOINT(ortin wmain ON)

# Include paths:
# - Public: Current source and binary directories.
# - Private: Parent source and binary directories,
#            and top-level binary directory for git_version.h.
TARGET_INCLUDE_DIRECTORIES(ortin
	PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>		# ortin
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>		# ortin
	PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>	# src
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>			# build
	)

TARGET_LINK_LIBRARIES(ortin PRIVATE libortin)
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
INSTALL(TARGETS ortin
	RUNTIME DESTINATION "${DIR_INSTALL_EXE}"
	LIBRARY DESTINATION "${DIR_INSTALL_DLL}"
	#ARCHIVE DESTINATION "${DIR_INSTALL_LIB}"
	COMPONENT "program"
	)
IF(INSTALL_DEBUG)
	# FIXME: Generator expression $<TARGET_PROPERTY:${_target},PDB> didn't work with CPack-3.6.1.
	GET_TARGET_PROPERTY(DEBUG_FILENAME ortin PDB)
	INSTALL(FILES "${DEBUG_FILENAME}"
		DESTINATION "${DIR_INSTALL_EXE_DEBUG}"
		COMPONENT "debug"
		)
	UNSET(DEBUG_FILENAME)
ENDIF(INSTALL_DEBUG)
