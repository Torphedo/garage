cmake_minimum_required(VERSION 3.18)
project(garage C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(ext/glfw)

include_directories("src" "ext/glfw/include" "ext/glad/include" "ext/cglm/include" "ext" "ext/physfs/src")

# Internal build tool to generate headers for GLSL shaders
add_executable(txt2h "ext/txt2h.c")

set(GLSL_SOURCES
    src/editor/shader/vcolor.frag
    src/editor/shader/vcolor.vert
    src/editor/shader/text.vert
    src/editor/shader/text.frag
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

# Build PhysicsFS without docs, to reduce build time.
set(PHYSFS_BUILD_DOCS FALSE CACHE BOOL "Build doxygen based documentation" )
set(PHYSFS_BUILD_SHARED FALSE CACHE BOOL "Build shared library")
set(PHYSFS_BUILD_TEST FALSE CACHE BOOL "Build stdio test program")
set(PHYSFS_DISABLE_INSTALL FALSE CACHE BOOL "Disable installing PhysFS")

# Disable everything but zip and 7z support. This saves 12KiB (ish).
set(PHYSFS_ARCHIVE_GRP FALSE CACHE BOOL "GRP support")
set(PHYSFS_ARCHIVE_WAD FALSE CACHE BOOL "WAD support")
set(PHYSFS_ARCHIVE_CSM FALSE CACHE BOOL "CSM support")
set(PHYSFS_ARCHIVE_HOG FALSE CACHE BOOL "HOG support")
set(PHYSFS_ARCHIVE_MVL FALSE CACHE BOOL "MVL support")
set(PHYSFS_ARCHIVE_QPAK FALSE CACHE BOOL "QPAK support")
set(PHYSFS_ARCHIVE_SLB FALSE CACHE BOOL "SLB support")
set(PHYSFS_ARCHIVE_VDF FALSE CACHE BOOL "VDF support")
set(PHYSFS_ARCHIVE_ISO9660 FALSE CACHE BOOL "ISO9660 support")
# Fix "uninstall" target name conflict with GLFW
set(PHYSFS_TARGETNAME_UNINSTALL "physfs_uninstall" CACHE STRING "Name of 'uninstall' build target")
add_subdirectory(ext/physfs)

add_library(common STATIC
    src/common/int.c
    src/common/file.c
    src/common/vector.c
    src/common/logging.c
    src/common/sha1.c
    src/common/image.c
    src/common/gl_debug.c
    src/common/gl_setup.c
    src/common/input.c
    src/common/shader.c
    src/common/primitives.c
    src/common/model.c
    src/common/path.c
)

add_executable(garage
    src/editor/camera.c
    src/editor/render_garage.c
    src/editor/render_debug.c
    src/editor/render_text.c
    src/editor/render_user.c
    src/editor/gui_common.c
    src/editor/vehicle_edit.c

    # Adding the GLSL headers here auto-generates them during the build
    ${glsl_headers}

    src/main.c
    src/vehicle.c
    src/parts.c
    src/stfs.c

    ext/glad/src/glad.c
    ext/stb_dxt.c
    ext/stb_truetype.c
)

target_link_libraries(garage PRIVATE glfw common physfs-static)

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

