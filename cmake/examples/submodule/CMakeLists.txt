# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

cmake_minimum_required(VERSION 3.16)

# Create the project and set the version

project(example-submodule VERSION 0.0.0 LANGUAGES CXX)

# Add executables

add_executable(${PROJECT_NAME} main.cpp)

# Load external dependencies

add_subdirectory(../../.. openqmc EXCLUDE_FROM_ALL)

# Add dependencies

target_link_libraries(${PROJECT_NAME} PRIVATE OpenQMC::OpenQMC)
