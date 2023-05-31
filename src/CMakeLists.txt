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
	target_include_directories(${PROJECT_NAME} PRIVATE
		${PROJECT_SOURCE_DIR}/include)
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

# Set common variables

set(EXTERN "${PROJECT_SOURCE_DIR}/extern")

# Add subdirectories

if(OPENQMC_BUILD_TOOLS)
	add_subdirectory(tools)
endif()

if(OPENQMC_BUILD_TESTING)
	add_subdirectory(tests)
endif()
