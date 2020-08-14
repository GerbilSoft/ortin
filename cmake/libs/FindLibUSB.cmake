# Find LibUSB libraries and headers. (GTK+ 2.x version)
# If found, the following variables will be defined:
# - LibUSB_FOUND: System has LibUSB.
# - LibUSB_INCLUDE_DIRS: LibUSB include directories.
# - LibUSB_LIBRARIES: LibUSB libraries.
# - LibUSB_DEFINITIONS: Compiler switches required for using LibUSB.
# - LibUSB_EXTENSIONS_DIR: Extensions directory. (for installation)
#
# In addition, a target LibUSB::libusb will be created with all of
# these definitions.
#
# References:
# - https://cmake.org/Wiki/CMake:How_To_Find_Libraries
# - http://francesco-cek.com/cmake-and-gtk-3-the-easy-way/
#

# TODO: Replace package prefix with CMAKE_INSTALL_PREFIX.
INCLUDE(FindLibraryPkgConfig)
FIND_LIBRARY_PKG_CONFIG(LibUSB
	libusb-1.0	# pkgconfig
	libusb.h	# header
	usb-1.0		# library
	LibUSB::libusb	# imported target
	)
