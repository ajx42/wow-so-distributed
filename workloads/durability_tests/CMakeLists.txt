project(wow_durability)

add_executable(reorder reorder.cc)
add_executable(checkpoint checkpoint.cpp)

install(TARGETS reorder checkpoint DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
