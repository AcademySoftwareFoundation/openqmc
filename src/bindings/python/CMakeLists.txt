# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Find Python and nanobind
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# Add nanobind dependency using FetchContent (like GLM in tools)
FetchContent_Declare(
	nanobind
	GIT_REPOSITORY https://github.com/wjakob/nanobind
	GIT_TAG        v2.2.0
	EXCLUDE_FROM_ALL
	FIND_PACKAGE_ARGS QUIET CONFIG)

FetchContent_MakeAvailable(nanobind)

mark_as_advanced(nanobind_DIR)
mark_as_advanced(FETCHCONTENT_SOURCE_DIR_NANOBIND)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_NANOBIND)

# Create Python module with latest nanobind flags
nanobind_add_module(
	openqmc_python
	STABLE_ABI          # For Python 3.12+ compatibility
	NB_STATIC          # Static nanobind core
	LTO                # Link-time optimization
	wrapper.cpp        # Single consolidated wrapper
)

target_link_libraries(openqmc_python PRIVATE ${PROJECT_NAME})

# Install the module  
install(TARGETS openqmc_python 
	LIBRARY DESTINATION ${SKBUILD_PLATLIB_DIR}/openqmc)