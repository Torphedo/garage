cmake_minimum_required(VERSION 3.16)
project(garage C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(ext/glfw)
add_subdirectory(ext/glad)

include_directories("src" "ext/glfw/include" "ext/glad/include" "ext/cglm/include" "ext")

# Internal build tool to generate headers for GLSL shaders
add_executable(txt2h "ext/txt2h.c")

set(GLSL_SOURCES
    src/gui/shader/fragment.glsl
    src/gui/shader/vertex.glsl
    src/gui/shader/text.vert
    src/gui/shader/text.frag
)

# Autogenerate string constant headers for each GLSL source file using txt2h.c
set(glsl_headers)
foreach(glsl_src ${GLSL_SOURCES})
    set(file_h "${glsl_src}.h") # Append ".h" to filename
    list(APPEND glsl_headers ${file_h}) # Add to list of generated headers

    add_custom_command(
        OUTPUT  ${CMAKE_SOURCE_DIR}/${file_h}
        COMMAND txt2h ${glsl_src} ${file_h}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endforeach()

add_library(common STATIC
    src/common/int.c
    src/common/file.c
    src/common/vector.c
    src/common/logging.c
    src/common/sha1.c
    src/common/image.c
)

add_executable(garage
    src/gui/gl_debug.c
    src/gui/gl_setup.c
    src/gui/input.c
    src/gui/camera.c
    src/gui/shader.c
    src/gui/render_garage.c
    src/gui/render_debug.c
    src/gui/render_text.c
    src/gui/gui_common.c
    src/gui/primitives.c

    # Adding the GLSL headers here auto-generates them during the build
    ${glsl_headers}

    src/main.c
    src/model.c
    src/vehicle.c
    src/parts.c
    src/stfs.c

    ext/stb_dxt.c
    ext/stb_truetype.c
)

target_link_libraries(garage PRIVATE glfw glad common)

# List of all test files
set(test_sources
    test/test_struct_size.c
    test/test_stfs.c
)

add_executable(test
    src/stfs.c
    ${test_sources}
    test/main.c
)
target_link_libraries(test PRIVATE common)

file(COPY "bin/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)

