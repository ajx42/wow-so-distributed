project(wow_ab)

add_executable(orchestrator orchestrator.cpp)
add_executable(checkpoint checkpoint.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(orchestrator PRIVATE Threads::Threads)

add_executable(participant participant.cpp)
install(TARGETS orchestrator participant DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

