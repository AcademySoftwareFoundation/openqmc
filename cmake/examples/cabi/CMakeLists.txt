# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

cmake_minimum_required(VERSION 3.16)

# Create the project and set the version

project(example-cabi VERSION 0.0.0 LANGUAGES C)

# Add executables

add_executable(${PROJECT_NAME} main.c)

# Enable C11 support

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)

# Find external dependencies

find_package(OpenQMC CONFIG REQUIRED)

# Add dependencies

target_link_libraries(${PROJECT_NAME} PRIVATE OpenQMC::CABI)
