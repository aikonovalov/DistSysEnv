cmake_minimum_required(VERSION 3.14)
project(DistSysEnv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_library(distsysenv
  src/utils/utils.h
  src/utils/message.cpp
  src/utils/message.h
  src/utils/node_id.cpp
  src/utils/node_id.h
  src/utils/node_id_manager.cpp
  src/utils/node_id_manager.h
  src/core/event.cpp
  src/core/event.h
  src/core/event_manager.cpp
  src/core/event_manager.h
  src/logger/logger.cpp
  src/logger/logger.h
  src/node/node.h
  src/node/context.cpp
  src/node/context.h
  src/network/network.cpp
  src/network/network.h
  src/network/network_settings.h
)
target_include_directories(distsysenv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(main_example
  examples/main_example.cpp
)
target_link_libraries(main_example PRIVATE distsysenv)
target_include_directories(main_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
