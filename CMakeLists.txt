cmake_minimum_required(VERSION 3.16)
project(garage C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(ext/glfw)
add_subdirectory(ext/glad)

# Internal build tool to generate headers for GLSL shaders
add_executable(txt2h "ext/txt2h.c")

set(GLSL_SOURCES
    src/gui/shader/fragment.glsl
    src/gui/shader/vertex.glsl
)

# Autogenerate string constant headers for each GLSL source file using txt2h.c
set(glsl_headers)
foreach(glsl_src ${GLSL_SOURCES})
    # Replace the ".glsl" with ".h" and put the string in ${file_h}
    get_filename_component(file_h ${glsl_src} NAME_WE)
    set(file_h "${file_h}.h")
    list(APPEND glsl_headers ${file_h}) # Add to list of generated headers

    add_custom_command(
        OUTPUT  ${CMAKE_SOURCE_DIR}/${file_h}
        COMMAND txt2h ${glsl_src}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endforeach()

add_executable(garage
    src/common/int.c
    src/common/file.c
    src/common/vector.c
    src/common/logging.c
    src/common/sha1.c

    src/gui/gl_debug.c
    src/gui/gl_setup.c
    src/gui/input.c
    src/gui/camera.c
    src/gui/shader.c
    src/gui/render_garage.c
    src/gui/gui_common.c
    src/gui/primitives.c
    src/gui/render_debug.c

    # Adding the GLSL headers here auto-generates them during the build
    ${glsl_headers}

    src/main.c
    src/vehicle.c
    src/part_ids.c
    src/stfs.c
)

target_include_directories(garage PRIVATE "src" "ext/glfw/include" "ext/glad/include" "ext/cglm/include")
target_link_libraries(garage PRIVATE glfw glad)
