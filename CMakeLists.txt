# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

cmake_minimum_required(VERSION 3.16)

# Create the project and set the version

project(OpenQMC VERSION 0.1.0 LANGUAGES CXX)

# Prevent in source builds
# https://cliutils.gitlab.io/modern-cmake/chapters/basics/structure.html

file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)

if(EXISTS "${LOC_PATH}")
	message(FATAL_ERROR "You cannot build in a source directory. Please make a \
build subdirectory. Then remove CMakeCache.txt and CMakeFiles.")
endif()

# Set the options for the build
# https://blog.kitware.com/cmake-and-the-default-build-type/

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Options are Debug, Release, MinSizeRel or RelWithDebInfo." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
		Debug Release MinSizeRel RelWithDebInfo)
endif()

if(NOT OPENQMC_ARCH_TYPE)
	set(OPENQMC_ARCH_TYPE Scalar CACHE STRING
		"Options are Scalar, SSE, AVX, ARM or GPU." FORCE)
	set_property(CACHE OPENQMC_ARCH_TYPE PROPERTY STRINGS
		Scalar SSE AVX ARM GPU)
endif()

# Add other binary options

option(OPENQMC_BUILD_TOOLS "Build the command line tools.")
option(OPENQMC_BUILD_TESTING "Build the unit tests.")
option(OPENQMC_USE_SUBMODULES "Ignore installed dependencies.")
option(OPENQMC_ENABLE_BINARY "Build binary to reduce memory cost.")
option(OPENQMC_SHARED_LIB "Make a shared library, in place of static.")
option(OPENQMC_FORCE_PIC "Force PIC for static libraries.")

# Enable testing

if(OPENQMC_BUILD_TESTING)
	enable_testing()
endif()

# Enable GPU support
# https://cmake.org/cmake/help/latest/variable/CMAKE_CUDA_ARCHITECTURES.html

if(${OPENQMC_ARCH_TYPE} STREQUAL GPU)
	set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)

	enable_language(CUDA)

	if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
		set(CMAKE_CUDA_ARCHITECTURES OFF)
	endif()
endif()

# Clean up the config

mark_as_advanced(CMAKE_CUDA_ARCHITECTURES)
mark_as_advanced(CMAKE_EXECUTABLE_FORMAT)
mark_as_advanced(CMAKE_OSX_ARCHITECTURES)
mark_as_advanced(CMAKE_OSX_DEPLOYMENT_TARGET)
mark_as_advanced(CMAKE_OSX_SYSROOT)

# Add uninstall target
# https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#can-i-do-make-uninstall-with-cmake

if(NOT TARGET uninstall)
	configure_file(cmake/cmake_uninstall.cmake.in
		cmake/cmake_uninstall.cmake @ONLY)
	add_custom_target(uninstall COMMAND ${CMAKE_COMMAND}
		-P cmake/cmake_uninstall.cmake)
endif()

# Include install directory variables

include(GNUInstallDirs)

# Add subdirectories

add_subdirectory(cmake)
add_subdirectory(include)
add_subdirectory(src)
