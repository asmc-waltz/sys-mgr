# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(system-manager)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})
include_directories(include)

file(GLOB_RECURSE SRC_FILES "src/*.c")
add_executable(sys-mgr ${SRC_FILES})
target_link_libraries(sys-mgr ${DBUS_LIBRARIES})

file(GLOB_RECURSE UTILS_FILES "utils/*.c")
add_executable(sys-utils ${UTILS_FILES})
target_link_libraries(sys-utils ${DBUS_LIBRARIES})

