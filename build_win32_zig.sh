#!/bin/sh

zig cc -target x86_64-windows -O2 src/*.c src/common/*.c src/editor/*.c ext/stb_*.c ext/glad/src/*.c ext/glfw/src/*.c -Iext/cglm/include -Iext/glad/include -Iext/glfw/include -Isrc -Iext -DGLFW_BUILD_WIN32 -D_GLFW_WIN32 -lgdi32 -ogarage.exe

