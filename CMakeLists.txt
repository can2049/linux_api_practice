
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-unused-variable")


add_subdirectory(multiplex_io)
add_subdirectory(cpu)
add_subdirectory(network)

add_executable(watch_file_change watch_file_change.cc)
add_executable(inotify_demo inotify_demo.cc)
add_executable(use_pipe use_pipe.cc)
