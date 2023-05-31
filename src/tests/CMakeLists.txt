# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Add dependencies

if(NOT OPENQMC_USE_SUBMODULES)
	find_package(GTest QUIET CONFIG)
	mark_as_advanced(GTest_DIR)
endif()

if(OPENQMC_USE_SUBMODULES OR NOT GTest_CONFIG)
	set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
	set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
	set(GTEST_HAS_ABSL OFF CACHE BOOL "" FORCE)

	add_subdirectory("${EXTERN}/googletest" googletest EXCLUDE_FROM_ALL)

	mark_as_advanced(FORCE BUILD_GMOCK)
	mark_as_advanced(FORCE INSTALL_GTEST)
	mark_as_advanced(FORCE GTEST_HAS_ABSL)
endif()

if(NOT OPENQMC_USE_SUBMODULES)
	find_package(hypothesis QUIET CONFIG)
	mark_as_advanced(hypothesis_DIR)
endif()

if(OPENQMC_USE_SUBMODULES OR NOT hypothesis_CONFIG)
	add_subdirectory("${EXTERN}/hypothesis" hypothesis EXCLUDE_FROM_ALL)
endif()

# Create tests executable

add_executable(tests EXCLUDE_FROM_ALL
	arch.cpp
	bntables.cpp
	encode.cpp
	float.cpp
	gpu.cpp
	lattice.cpp
	latticebn.cpp
	lookup.cpp
	oqmc.cpp
	owen.cpp
	pcg.cpp
	pmj.cpp
	pmjbn.cpp
	permute.cpp
	rank1.cpp
	reverse.cpp
	rotate.cpp
	sampler.cpp
	sobol.cpp
	sobolbn.cpp
	state.cpp
	stochastic.cpp
	unused.cpp)

target_link_libraries(tests PRIVATE
	${PROJECT_NAME}
	GTest::gtest_main
	hypothesis::hypothesis)

target_compile_features(tests PRIVATE cxx_std_14)
target_compile_definitions(tests PRIVATE OQMC_FORCE_SCALAR)

# Register tests with CTest

include(GoogleTest)
gtest_discover_tests(tests)
