# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

# Create benchmark executable

add_executable(benchmark
	benchmark.cpp)

target_link_libraries(benchmark PRIVATE tools)
target_compile_features(benchmark PRIVATE cxx_std_14)

# Create frequency executable

add_executable(frequency
	frequency.cpp)

target_link_libraries(frequency PRIVATE tools)
target_compile_features(frequency PRIVATE cxx_std_14)

# Create generate executable

add_executable(generate
	generate.cpp)

target_link_libraries(generate PRIVATE tools)
target_compile_features(generate PRIVATE cxx_std_14)

# Create matrices executable

add_executable(matrices
	matrices.cpp)

target_link_libraries(matrices PRIVATE ${PROJECT_NAME})
target_compile_features(matrices PRIVATE cxx_std_14)

# Create optimise executable

add_executable(optimise
	optimise.cpp)

target_link_libraries(optimise PRIVATE tools)
target_compile_features(optimise PRIVATE cxx_std_14)

# Create plot executable

add_executable(plot
	plot.cpp)

target_link_libraries(plot PRIVATE tools)
target_compile_features(plot PRIVATE cxx_std_14)

# Create trace executable

add_executable(trace
	trace.cpp)

target_link_libraries(trace PRIVATE tools)
target_compile_features(trace PRIVATE cxx_std_14)
