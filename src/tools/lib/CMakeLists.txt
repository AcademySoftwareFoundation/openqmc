# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Add dependencies

if(NOT OPENQMC_USE_SUBMODULES)
	find_package(TBB QUIET CONFIG)
	mark_as_advanced(TBB_DIR)
endif()

if(OPENQMC_USE_SUBMODULES OR NOT TBB_CONFIG)
	set(TBB_TEST OFF CACHE BOOL "" FORCE)

	add_subdirectory("${EXTERN}/oneTBB" TBB EXCLUDE_FROM_ALL)

	mark_as_advanced(FORCE CMAKE_C_OSX_DEPLOYMENT_TARGET_FLAG)
	mark_as_advanced(FORCE SDE_EXE)
	mark_as_advanced(FORCE TBB4PY_BUILD)
	mark_as_advanced(FORCE TBBMALLOC_BUILD)
	mark_as_advanced(FORCE TBBMALLOC_PROXY_BUILD)
	mark_as_advanced(FORCE TBB_BUILD)
	mark_as_advanced(FORCE TBB_CPF)
	mark_as_advanced(FORCE TBB_DISABLE_HWLOC_AUTOMATIC_SEARCH)
	mark_as_advanced(FORCE TBB_ENABLE_IPO)
	mark_as_advanced(FORCE TBB_EXAMPLES)
	mark_as_advanced(FORCE TBB_FIND_PACKAGE)
	mark_as_advanced(FORCE TBB_INSTALL_VARS)
	mark_as_advanced(FORCE TBB_NO_APPCONTAINER)
	mark_as_advanced(FORCE TBB_SANITIZE)
	mark_as_advanced(FORCE TBB_STRICT)
	mark_as_advanced(FORCE TBB_TEST)
	mark_as_advanced(FORCE TBB_TEST_SPEC)
	mark_as_advanced(FORCE TBB_VALGRIND_MEMCHECK)
	mark_as_advanced(FORCE TBB_WINDOWS_DRIVER)
	mark_as_advanced(FORCE TBB_DIR)
endif()

if(NOT OPENQMC_USE_SUBMODULES)
	find_package(glm QUIET CONFIG)
	mark_as_advanced(glm_DIR)
endif()

if(OPENQMC_USE_SUBMODULES OR NOT glm_CONFIG)
	add_subdirectory("${EXTERN}/glm" glm EXCLUDE_FROM_ALL)
endif()

# Function to configure target architecture

function(target_architecture TARGET)
	if(${OPENQMC_ARCH_TYPE} STREQUAL SSE)
		target_compile_options(${TARGET} PRIVATE -msse2)
	endif()

	if(${OPENQMC_ARCH_TYPE} STREQUAL AVX)
		target_compile_options(${TARGET} PRIVATE -mavx2)
	endif()

	if(${OPENQMC_ARCH_TYPE} STREQUAL GPU)
		target_architecture_gpu(${TARGET})
	endif()
endfunction()

function(target_architecture_gpu TARGET)
	get_target_property(TARGET_SOURCES ${TARGET} SOURCES)
	set_source_files_properties(${TARGET_SOURCES} PROPERTIES LANGUAGE CUDA)
	target_compile_options(${TARGET} PRIVATE --extended-lambda)
	set_target_properties(${TARGET} PROPERTIES CUDA_STANDARD 14)
	set_target_properties(${TARGET} PROPERTIES CUDA_STANDARD_REQUIRED ON)
endfunction()

# Create tools library

add_library(tools SHARED
	benchmark.cpp
	frequency.cpp
	generate.cpp
	optimise.cpp
	plot.cpp
	progress.cpp
	trace.cpp)

target_link_libraries(tools PRIVATE ${PROJECT_NAME}
	TBB::tbb
	glm::glm)

target_compile_features(tools PRIVATE cxx_std_14)
target_compile_definitions(tools PRIVATE _USE_MATH_DEFINES)
target_include_directories(tools INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_architecture(tools)
