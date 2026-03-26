cmake_minimum_required(VERSION 3.14)
project(DistSysEnv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Catch2 REQUIRED)

add_library(distsysenv_core
  src/core/context/context.cpp
  src/core/message/message.cpp
  src/core/node_id/node_id.cpp
  src/core/node_id/node_id_manager.cpp
  src/core/event/event.cpp
  src/core/event/event_manager.cpp
)
target_include_directories(distsysenv_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_core
  tests/test_core.cpp
)
target_link_libraries(test_core PRIVATE distsysenv_core Catch2::Catch2WithMain)
target_include_directories(test_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
