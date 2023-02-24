project(wow_ab)

add_executable(orchestrator orchestrator.cpp)
add_executable(participant participant.cpp)
install(TARGETS orchestrator participant DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

