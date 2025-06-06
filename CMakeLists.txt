# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(system-manager)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(LIBNM REQUIRED libnm)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0 gobject-2.0)

include_directories(
    ${DBUS_INCLUDE_DIRS}
    ${LIBNM_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    )
link_directories(
    ${DBUS_LIBRARY_DIRS}
    ${LIBNM_LIBRARY_DIRS}
    ${GLIB_LIBRARY_DIRS}
    )
add_definitions(
    ${LIBNM_CFLAGS_OTHER}
    ${GLIB_CFLAGS_OTHER}
)

include_directories(include)

file(GLOB_RECURSE SRC_FILES "src/*.c")
add_executable(sys-mgr ${SRC_FILES})
target_link_libraries(sys-mgr
    ${DBUS_LIBRARIES}
    ${LIBNM_LIBRARIES}
    ${GLIB_LIBRARIES}
)

file(GLOB_RECURSE UTILS_FILES "utils/*.c")
add_executable(sys-utils ${UTILS_FILES})
target_link_libraries(sys-utils ${DBUS_LIBRARIES})

