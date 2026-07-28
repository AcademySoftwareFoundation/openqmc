# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Create project target

add_library(${PROJECT_NAME} INTERFACE)

if(${OPENQMC_ARCH_TYPE} STREQUAL Scalar)
	target_compile_definitions(${PROJECT_NAME} INTERFACE OQMC_FORCE_SCALAR)
endif()

if(${OPENQMC_ARCH_TYPE} STREQUAL GPU)
	target_compile_definitions(${PROJECT_NAME} INTERFACE OQMC_FORCE_SCALAR)
endif()

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

# Add include directories

target_include_directories(${PROJECT_NAME} INTERFACE
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# Alias target for down stream projects

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Setup target for install

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets)

# Setup fetch content for dependencies

if(OPENQMC_BUILD_TOOLS OR OPENQMC_BUILD_TESTING)
	if(OPENQMC_FORCE_DOWNLOAD)
		set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)
	endif()

	include(FetchContent)

	mark_as_advanced(FETCHCONTENT_BASE_DIR)
	mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
	mark_as_advanced(FETCHCONTENT_QUIET)
	mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
endif()

# Add subdirectories

if(OPENQMC_BUILD_TOOLS)
	add_subdirectory(tools)
endif()

if(OPENQMC_BUILD_TESTING)
	add_subdirectory(tests)
endif()
