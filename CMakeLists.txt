cmake_minimum_required(VERSION 3.14)
project(DistSysEnv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Catch2 REQUIRED CONFIG)

add_library(distsysenv_core
  src/core/context/context.cpp
  src/core/message/message.cpp
  src/core/node_id/node_id.cpp
  src/core/node_id/node_id_manager.cpp
  src/core/event/event.cpp
  src/core/event/event_manager.cpp
)
target_include_directories(distsysenv_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(distsysenv_simulation
  src/simulation/context/context.cpp
  src/simulation/context/timer_book.cpp
  src/simulation/event/event.cpp
  src/simulation/network_node/network_node.cpp
  src/simulation/scenario/scenario.cpp
)
target_link_libraries(distsysenv_simulation PUBLIC distsysenv_core)
target_include_directories(distsysenv_simulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(distsysenv_raft
  raft/raft_node.cpp
  raft/raft_election.cpp
  raft/raft_replication.cpp
  raft/raft_state.cpp
  raft/raft_client.cpp
  raft/message_specs.cpp
)
target_link_libraries(distsysenv_raft PUBLIC distsysenv_simulation)
target_include_directories(distsysenv_raft PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(distsysenv_hybrid_raft
  hybrid_raft/hybrid_raft_node.cpp
  hybrid_raft/hybrid_raft_election.cpp
  hybrid_raft/hybrid_raft_replication.cpp
  hybrid_raft/hybrid_raft_state.cpp
  hybrid_raft/hybrid_raft_client.cpp
)
target_link_libraries(distsysenv_hybrid_raft PUBLIC distsysenv_raft)
target_include_directories(distsysenv_hybrid_raft PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_core
  tests/test_core.cpp
)
target_include_directories(test_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_core PRIVATE distsysenv_core Catch2::Catch2WithMain)

add_executable(test_simulation
  tests/test_simulation.cpp
)
target_include_directories(test_simulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_simulation PRIVATE distsysenv_simulation Catch2::Catch2WithMain)

add_executable(test_raft
  tests/test_raft.cpp
)
target_include_directories(test_raft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_raft PRIVATE distsysenv_hybrid_raft Catch2::Catch2WithMain)

add_executable(test_mc
  tests/mc/test_raft_monkey_chaos.cpp
)
target_include_directories(test_mc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_mc PRIVATE distsysenv_raft Catch2::Catch2WithMain)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND DEFINED catch2_LIB_DIRS_RELEASE)
  target_include_directories(test_core PRIVATE ${Catch2_INCLUDE_DIRS})
  target_include_directories(test_simulation PRIVATE ${Catch2_INCLUDE_DIRS})

  target_link_libraries(test_core PRIVATE
    "${catch2_LIB_DIRS_RELEASE}/libCatch2.a"
    "${catch2_LIB_DIRS_RELEASE}/libCatch2Main.a"
  )
  
  target_link_libraries(test_simulation PRIVATE
    "${catch2_LIB_DIRS_RELEASE}/libCatch2.a"
    "${catch2_LIB_DIRS_RELEASE}/libCatch2Main.a"
  )

  target_include_directories(test_raft PRIVATE ${Catch2_INCLUDE_DIRS})
  target_link_libraries(test_raft PRIVATE
    "${catch2_LIB_DIRS_RELEASE}/libCatch2.a"
    "${catch2_LIB_DIRS_RELEASE}/libCatch2Main.a"
  )

  target_include_directories(test_mc PRIVATE ${Catch2_INCLUDE_DIRS})
  target_link_libraries(test_mc PRIVATE
    "${catch2_LIB_DIRS_RELEASE}/libCatch2.a"
    "${catch2_LIB_DIRS_RELEASE}/libCatch2Main.a"
  )
endif()

add_executable(main_example
  examples/main_example.cpp
)
target_link_libraries(main_example PUBLIC distsysenv_simulation)
target_include_directories(main_example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(raft_example
  examples/raft_example.cpp
)
target_link_libraries(raft_example PRIVATE distsysenv_raft)
target_include_directories(raft_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
