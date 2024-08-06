#pragma once

#if (defined(__APPLE__) && defined(__MACH__))
    #define PLATFORM_APPLE 1
#endif

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    #define PLATFORM_UNIX 1

    #include <sys/param.h>
    #ifdef BSD
        #define PLATFORM_BSD 1 
    #endif

    #include <unistd.h>
    #if defined(_POSIX_VERSION)
        #define PLATFORM_POSIX 1
    #endif
#elif defined (WIN32) || defined(_WIN32)
    #define PLATFORM_WINDOWS 1
#endif

#if defined(PLATFORM_WINDOWS)
    #define PLATFORM_DIRSEP ('\\')
#else
    #define PLATFORM_DIRSEP ('/')
#endif
