include(CheckFunctionExists)

set(UNRELIABLEFS_SRC conf.c
                     unreliablefs.c
                     unreliablefs_errinj.c
                     unreliablefs_ops.c)

set_source_files_properties(${UNRELIABLEFS_SRC} PROPERTIES LANGUAGE CXX )

find_package(FUSE 2.9 REQUIRED)

add_definitions(${FUSE_DEFINITIONS})
add_executable(${CLIENT_EXEC} ${UNRELIABLEFS_SRC})

target_link_libraries(${CLIENT_EXEC} fs_grpc_proto)
target_link_libraries(${CLIENT_EXEC} wowrpc)
target_include_directories(${CLIENT_EXEC} PRIVATE ${FUSE_INCLUDE_DIRS})

message("wowRPC generated binaries will be read from: ${WOW_RPC_BINARY_DIR}")

include_directories(${WOW_RPC_BINARY_DIR})
target_link_libraries(${CLIENT_EXEC} ${FUSE_LIBRARIES})

target_compile_options(${CLIENT_EXEC} PUBLIC -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wno-unused-parameter)
target_compile_options(${CLIENT_EXEC} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++11 -D_FILE_OFFSET_BITS=64>)

check_function_exists(fallocate HAVE_FALLOCATE)
check_function_exists(fallocate HAVE_FLOCK)
check_function_exists(utimensat HAVE_UTIMENSAT)
check_function_exists(setxattr HAVE_XATTR)
if (${HAVE_FALLOCATE})
    target_compile_definitions(${CLIENT_EXEC} PUBLIC HAVE_FALLOCATE)
endif ()
if (${HAVE_FLOCK})
    target_compile_definitions(${CLIENT_EXEC} PUBLIC HAVE_FLOCK)
endif ()
if (${HAVE_UTIMENSAT})
    target_compile_definitions(${CLIENT_EXEC} PUBLIC HAVE_UTIMENSAT)
endif ()
if (${HAVE_XATTR})
    target_compile_definitions(${CLIENT_EXEC} PUBLIC HAVE_XATTR)
endif ()

add_subdirectory(tests)

install(TARGETS ${CLIENT_EXEC} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES "unreliablefs.1" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
install(FILES "unreliablefs.conf.5" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man5)
