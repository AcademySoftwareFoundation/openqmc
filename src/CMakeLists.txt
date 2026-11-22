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

# Create C ABI target

if(OPENQMC_BUILD_CABI)
	if(OPENQMC_CABI_SHARED_LIB)
		add_library(${PROJECT_NAME}_CABI SHARED)

		# Export only the C ABI functions.
		target_compile_definitions(${PROJECT_NAME}_CABI PRIVATE OQMC_CABI_EXPORT_SYMBOLS)
	else()
		add_library(${PROJECT_NAME}_CABI STATIC)
	endif()

	# Hide all other symbols. The static library keeps them private to any
	# binary that links it, such as a plugin, so that separate copies can't
	# bind to one another.
	set_target_properties(${PROJECT_NAME}_CABI PROPERTIES CXX_VISIBILITY_PRESET hidden)

	if(OPENQMC_CABI_FORCE_PIC)
		set_target_properties(${PROJECT_NAME}_CABI PROPERTIES POSITION_INDEPENDENT_CODE ON)
	endif()

	target_sources(${PROJECT_NAME}_CABI PRIVATE oqmc_impl.cpp)
	target_compile_options(${PROJECT_NAME}_CABI PRIVATE ${OPENQMC_CXX_FLAGS})
	target_compile_definitions(${PROJECT_NAME}_CABI PRIVATE OQMC_CABI_SAMPLER_${OPENQMC_CABI_SAMPLER})
	target_link_libraries(${PROJECT_NAME}_CABI PRIVATE ${PROJECT_NAME})

	# Link no default libraries except libc.

	if(NOT WIN32)
		target_link_options(${PROJECT_NAME}_CABI PRIVATE -nodefaultlibs)
		target_link_libraries(${PROJECT_NAME}_CABI PRIVATE c)
	endif()

	# Add include directories

	target_include_directories(${PROJECT_NAME}_CABI INTERFACE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

	# Name the library liboqmc, and export it as OpenQMC::CABI.

	set_target_properties(${PROJECT_NAME}_CABI PROPERTIES OUTPUT_NAME oqmc EXPORT_NAME CABI)

	# Alias target for down stream projects

	add_library(${PROJECT_NAME}::CABI ALIAS ${PROJECT_NAME}_CABI)

	# Setup target for install

	install(TARGETS ${PROJECT_NAME}_CABI EXPORT ${PROJECT_NAME}Targets)
endif()

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
