# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

cmake_minimum_required(VERSION 3.16)

# Create the project and set the version

project(example-include VERSION 0.0.0 LANGUAGES CXX)

# Add executables

add_executable(${PROJECT_NAME} main.cpp)

# Enable C++14 support

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)

# Add dependencies

target_include_directories(${PROJECT_NAME} PRIVATE ../../../include)
