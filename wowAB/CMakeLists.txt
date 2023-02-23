project(wow_ab)

add_executable(sink sink.cpp)
add_executable(orchestrator orchestrator.cpp)
add_executable(participantA participant.cpp)
add_executable(participantB participantB.cpp)
install(TARGETS orchestrator DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

