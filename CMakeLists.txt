cmake_minimum_required(VERSION 3.16)
project(garage C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(ext/glfw)
add_subdirectory(ext/glad)

add_executable(garage
    src/common/int.c
    src/common/file.c
    src/common/vector.c
    src/common/logging.c
    src/common/sha1.c

    src/main.c
    src/vehicle.c
    src/part_ids.c
    src/stfs.c
)

target_include_directories(garage PRIVATE "src" "ext/glfw/include" "ext/glad/include")
target_link_libraries(garage PRIVATE glfw glad)
