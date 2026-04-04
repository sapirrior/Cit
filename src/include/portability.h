#ifndef PORTABILITY_H
#define PORTABILITY_H

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #include <process.h>
    #include <windows.h>
    
    #define mkdir(path, mode) _mkdir(path)
    #define getcwd _getcwd
    #define S_IRWXU 0
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <dirent.h>
#endif

#endif // PORTABILITY_H
