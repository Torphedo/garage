cmake_minimum_required(VERSION 3.18)
project(garage C)

set(CMAKE_C_STANDARD 11)
# Emit a compile_commands.json for use with clangd in LSPs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

add_subdirectory(ext/glfw)

include_directories("src" "ext/glfw/include" "ext/glad/include" "ext/cglm/include" "ext" "ext/physfs/src")

# Internal build tool to generate headers for GLSL shaders
add_executable(incbin "ext/incbin.c")

# Build PhysicsFS without docs, to reduce build time.
set(PHYSFS_BUILD_DOCS FALSE CACHE BOOL "Build doxygen based documentation")
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
    src/common/list.c
)

add_executable(garage
    src/editor/camera.c
    src/editor/render_garage.c
    src/editor/render_debug.c
    src/editor/render_text.c
    src/editor/render_user.c
    src/editor/editor.c
    src/editor/vehicle_edit.c

    # Adding this to the source lists forces the custom command to run every
    # build
    ${CMAKE_CURRENT_BINARY_DIR}/generate_datafile

    src/main.c
    src/physfs_bundling.c
    src/vehicle.c
    src/parts.c
    src/stfs.c

    ext/glad/src/glad.c
    ext/stb_dxt.c
    ext/stb_truetype.c
)

target_link_libraries(garage PRIVATE glfw common physfs-static)

if (NOT MSVC)
    # Generate a data.zip with our assets
    add_custom_command(
        # This output file will never exist, which makes CMake run this command on
        # every build.
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generate_datafile
        # Setting the working dir stops it from making ".." the archive root.
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND "cmake"
        ARGS -E tar c ${CMAKE_CURRENT_BINARY_DIR}/data.zip --format=zip ${CMAKE_SOURCE_DIR}/bin/ ${CMAKE_SOURCE_DIR}/src/editor/shader/
    )
else()
    add_custom_command(
        # This output file will never exist, which makes CMake run this command on
        # every build.
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generate_datafile ${CMAKE_CURRENT_BINARY_DIR}/data_source.c
        # Setting the working dir stops it from making ".." the archive root... this is really stupid.
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND "cmake"
        ARGS -E tar c ${CMAKE_CURRENT_BINARY_DIR}/data.zip --format=zip ${CMAKE_SOURCE_DIR}/bin/ ${CMAKE_SOURCE_DIR}/src/editor/shader/

        # On MSVC we need to use incbin to generate a .c file because they don't have an inline assembler.
        COMMAND incbin
        ARGS src/physfs_bundling.c -Ssnakecase -o ${CMAKE_CURRENT_BINARY_DIR}/data_source.c -I${CMAKE_CURRENT_BINARY_DIR} -I.
    )
    # Add the generated source file to the sources list
    set_property(TARGET garage APPEND PROPERTY SOURCES ${CMAKE_CURRENT_BINARY_DIR}/data_source.c)
endif()

# List of all test files
set(test_sources
    test/test_stfs.c
)

add_executable(test
    src/stfs.c
    ${test_sources}
    test/main.c
)
target_link_libraries(test PRIVATE common)

