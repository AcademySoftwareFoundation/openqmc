# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Add external dependencies

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(GTEST_HAS_ABSL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest
	GIT_TAG        58d77fa # 1.12.1
	EXCLUDE_FROM_ALL
	FIND_PACKAGE_ARGS QUIET CONFIG NAMES GTest)

FetchContent_Declare(
	hypothesis
	GIT_REPOSITORY https://github.com/joshbainbridge/hypothesis
	GIT_TAG        cde392d # latest
	EXCLUDE_FROM_ALL
	FIND_PACKAGE_ARGS QUIET CONFIG)

FetchContent_MakeAvailable(googletest hypothesis)

mark_as_advanced(googletest_DIR)
mark_as_advanced(hypothesis_DIR)
mark_as_advanced(BUILD_GMOCK)
mark_as_advanced(FETCHCONTENT_SOURCE_DIR_GOOGLETEST)
mark_as_advanced(FETCHCONTENT_SOURCE_DIR_HYPOTHESIS)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_GOOGLETEST)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_HYPOTHESIS)
mark_as_advanced(GTEST_HAS_ABSL)
mark_as_advanced(INSTALL_GTEST)

# Create tests executable

add_executable(tests EXCLUDE_FROM_ALL
	arch.cpp
	bntables.cpp
	encode.cpp
	float.cpp
	gpu.cpp
	lookup.cpp
	oqmc.cpp
	pcg.cpp
	pmj.cpp
	pmjbn.cpp
	permute.cpp
	range.cpp
	reverse.cpp
	rotate.cpp
	sampler.cpp
	state.cpp
	stochastic.cpp
	unused.cpp)

target_link_libraries(tests PRIVATE
	${PROJECT_NAME}
	GTest::gtest_main
	hypothesis::hypothesis)

target_compile_options(tests PRIVATE ${OPENQMC_CXX_FLAGS})
target_compile_features(tests PRIVATE cxx_std_14)
target_compile_definitions(tests PRIVATE _USE_MATH_DEFINES)
target_compile_definitions(tests PRIVATE OQMC_FORCE_SCALAR)

# Register tests with CTest

include(GoogleTest)
gtest_discover_tests(tests)
