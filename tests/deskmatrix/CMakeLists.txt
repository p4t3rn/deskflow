# SPDX-FileCopyrightText: (C) 2026 DeskMatrix contributors
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.24)
project(DeskMatrixStatusTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.7 REQUIRED COMPONENTS Core Test)
enable_testing()
add_executable(ManagedStatusTests ManagedStatusTests.cpp)
target_include_directories(ManagedStatusTests PRIVATE ../../src/lib)
target_link_libraries(ManagedStatusTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME ManagedStatusTests COMMAND ManagedStatusTests)
