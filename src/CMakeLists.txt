# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Create project target

if(OPENQMC_ENABLE_BINARY)
	if(OPENQMC_SHARED_LIB)
		add_library(${PROJECT_NAME} SHARED)
	else()
		add_library(${PROJECT_NAME} STATIC)
	endif()

	if(OPENQMC_FORCE_PIC)
		set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
	endif()

	target_sources(${PROJECT_NAME} PRIVATE bntables.cpp)
	target_compile_options(${PROJECT_NAME} PRIVATE ${OPENQMC_CXX_FLAGS})
	target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
else()
	add_library(${PROJECT_NAME} INTERFACE)
endif()

if(${OPENQMC_ARCH_TYPE} STREQUAL Scalar)
	target_compile_definitions(${PROJECT_NAME} INTERFACE OQMC_FORCE_SCALAR)
endif()

if(${OPENQMC_ARCH_TYPE} STREQUAL GPU)
	target_compile_definitions(${PROJECT_NAME} INTERFACE OQMC_FORCE_SCALAR)
endif()

if(OPENQMC_ENABLE_BINARY)
	target_compile_definitions(${PROJECT_NAME} INTERFACE OQMC_ENABLE_BINARY)
endif()

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_14)

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

add_subdirectory(bindings)
