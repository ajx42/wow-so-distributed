cmake_minimum_required(VERSION 3.25.2)
project(wowFSToplevel LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# allow including wowRPC C++ files from across the project
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/wowRPC")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/wowUtils")

set(WOW_FS_EXEC "wowfs")
set(WOW_RPC_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/wowRPC")
set(WOW_FS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/wowFS")

# include generated source files (proto)
include_directories("${WOW_RPC_BINARY_DIR}")

add_subdirectory(wowRPC "${WOW_RPC_BINARY_DIR}")
message(STATUS "Using WowRPC via add_subdirectory.")

add_subdirectory(wowFS "${WOW_FS_BINARY_DIR}")
message(STATUS "Using UnreliableFS via add_subdirectory.") 
