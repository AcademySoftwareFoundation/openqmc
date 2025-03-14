# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Create benchmark executable

add_executable(benchmark
	benchmark.cpp)

target_compile_options(benchmark PRIVATE ${OPENQMC_CXX_FLAGS})
target_link_libraries(benchmark PRIVATE tools)

# Create frequency executable

add_executable(frequency
	frequency.cpp)

target_compile_options(frequency PRIVATE ${OPENQMC_CXX_FLAGS})
target_link_libraries(frequency PRIVATE tools)

# Create generate executable

add_executable(generate
	generate.cpp)

target_compile_options(generate PRIVATE ${OPENQMC_CXX_FLAGS})
target_link_libraries(generate PRIVATE tools)

# Create matrices executable

add_executable(matrices
	matrices.cpp)

target_compile_options(matrices PRIVATE ${OPENQMC_CXX_FLAGS})
target_link_libraries(matrices PRIVATE ${PROJECT_NAME})

# Create optimise executable

add_executable(optimise
	optimise.cpp)

target_compile_options(optimise PRIVATE ${OPENQMC_CXX_FLAGS})
target_link_libraries(optimise PRIVATE tools)

# Create plot executable

add_executable(plot
	plot.cpp)

target_compile_options(plot PRIVATE ${OPENQMC_CXX_FLAGS})
target_link_libraries(plot PRIVATE tools)

# Create trace executable

add_executable(trace
	trace.cpp)

target_compile_options(trace PRIVATE ${OPENQMC_CXX_FLAGS})
target_link_libraries(trace PRIVATE tools)
