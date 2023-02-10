set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/wowfs/cmake;${CMAKE_MODULE_PATH}")

set(CLIENT_EXEC "${WOW_FS_EXEC}")

add_subdirectory(unreliablefs)

option(ENABLE_ASAN "Enable AddressSanitizer, a fast memory error detector
                    based on compiler instrumentation" OFF)
if(ENABLE_ASAN)
    target_compile_options(${CLIENT_EXEC} PUBLIC -fsanitize=address)
    target_link_options(${CLIENT_EXEC} PUBLIC -fsanitize=address)
endif()

option(ENABLE_UBSAN "Enable UBSanitizer, undefined behaviour
                     detector based on compiler instrumentation" OFF)
if(ENABLE_UBSAN)
    target_compile_options(${CLIENT_EXEC} PUBLIC -fsanitize=undefined)
    target_link_options(${CLIENT_EXEC} PUBLIC -fsanitize=undefined)
endif()

add_custom_target(check DEPENDS check-mandoc check-rockspec)

set(ROCKSPEC "unreliablefs-scm-1.rockspec")
add_custom_target(check-rockspec DEPENDS ${ROCKSPEC})
add_custom_command(TARGET check-rockspec
                   COMMAND luarocks --local build ${PROJECT_SOURCE_DIR}/${ROCKSPEC}
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                   COMMENT "Build Lua rockspec")

set(MANUAL_PAGES "unreliablefs/unreliablefs.1;unreliablefs/unreliablefs.conf.5")
add_custom_target(check-mandoc DEPENDS ${MANUAL_PAGES})
add_custom_command(TARGET check-mandoc
                   COMMAND mandoc -T lint -W warning,stop ${MANUAL_PAGES}
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                   COMMENT "Run mandoc static analysis")
