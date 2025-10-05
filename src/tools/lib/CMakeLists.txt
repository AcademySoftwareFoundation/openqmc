# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Add external dependencies

set(TBB_TEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
	oneTBB
	GIT_REPOSITORY https://github.com/oneapi-src/oneTBB
	GIT_TAG        9afd759 # 2021.12.0
	EXCLUDE_FROM_ALL
	FIND_PACKAGE_ARGS QUIET CONFIG NAMES TBB)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm
	GIT_TAG        bf71a83 # 0.9.9.8
	EXCLUDE_FROM_ALL
	FIND_PACKAGE_ARGS QUIET CONFIG)

FetchContent_MakeAvailable(oneTBB glm)

mark_as_advanced(oneTBB_DIR)
mark_as_advanced(glm_DIR)
mark_as_advanced(CMAKE_C_OSX_DEPLOYMENT_TARGET_FLAG)
mark_as_advanced(FETCHCONTENT_SOURCE_DIR_GLM)
mark_as_advanced(FETCHCONTENT_SOURCE_DIR_ONETBB)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_GLM)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_ONETBB)
mark_as_advanced(SDE_EXE)
mark_as_advanced(TBB4PY_BUILD)
mark_as_advanced(TBBMALLOC_BUILD)
mark_as_advanced(TBBMALLOC_PROXY_BUILD)
mark_as_advanced(TBB_BUILD)
mark_as_advanced(TBB_CPF)
mark_as_advanced(TBB_DISABLE_HWLOC_AUTOMATIC_SEARCH)
mark_as_advanced(TBB_ENABLE_IPO)
mark_as_advanced(TBB_EXAMPLES)
mark_as_advanced(TBB_FIND_PACKAGE)
mark_as_advanced(TBB_FUZZ_TESTING)
mark_as_advanced(TBB_INSTALL)
mark_as_advanced(TBB_INSTALL_VARS)
mark_as_advanced(TBB_NO_APPCONTAINER)
mark_as_advanced(TBB_SANITIZE)
mark_as_advanced(TBB_STRICT)
mark_as_advanced(TBB_TEST)
mark_as_advanced(TBB_TEST_SPEC)
mark_as_advanced(TBB_VALGRIND_MEMCHECK)
mark_as_advanced(TBB_WINDOWS_DRIVER)

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
	else()
		target_compile_options(${TARGET} PRIVATE ${OPENQMC_CXX_FLAGS})
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

target_compile_definitions(tools PRIVATE _USE_MATH_DEFINES)
target_include_directories(tools INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_architecture(tools)
