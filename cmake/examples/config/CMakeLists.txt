# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

cmake_minimum_required(VERSION 3.16)

# Create the project and set the version

project(example-config VERSION 0.0.0 LANGUAGES CXX)

# Add executables

add_executable(${PROJECT_NAME} main.cpp)

# Find external dependencies

find_package(OpenQMC CONFIG REQUIRED)

# Add dependencies

target_link_libraries(${PROJECT_NAME} PRIVATE OpenQMC::OpenQMC)
