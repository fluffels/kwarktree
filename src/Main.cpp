#include <Windows.h>

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

LARGE_INTEGER counterEpoch;
LARGE_INTEGER counterFrequency;
FILE* logFile;

float GetElapsed() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    auto result =
        (t.QuadPart - counterEpoch.QuadPart)
        / (float)counterFrequency.QuadPart;
    return result;
}

#define TIME()\
    fprintf(logFile, "[%f]", GetElapsed());

#define LOC()\
    fprintf(logFile, "[%s:%d]", __FILE__, __LINE__);

#define LOG(level, ...)\
    LOC()\
    TIME()\
    fprintf(logFile, "[%s] ", level);\
    fprintf(logFile, __VA_ARGS__); \
    fprintf(logFile, "\n");

#define FATAL(...)\
    LOG("FATAL", __VA_ARGS__);\
    exit(1);

#define INFO(...)\
    LOG("INFO", __VA_ARGS__)

#define CHECK(x, ...)\
    if (!x) { FATAL(__VA_ARGS__) }

#define LERROR(x) \
    if (x) { \
        char buffer[1024]; \
        strerror_s(buffer, errno); \
        FATAL(buffer); \
    }

#include "PAK.cpp"

int
WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR commandLine,
    int showCommand
) {
    auto error = fopen_s(&logFile, "LOG", "w");
    if (error) return 1;

    QueryPerformanceCounter(&counterEpoch);
    QueryPerformanceFrequency(&counterFrequency);

    auto fname = "pak0.pk3";

    struct _stat stat = {};
    LERROR(_stat(fname, &stat))

    FILE* pakFile;
    LERROR(fopen_s(&pakFile, fname, "rb"));
    INFO("File opened");

    auto buffer = (char*)malloc(stat.st_size);
    INFO("Data allocated");

    auto bytesRead = fread(buffer, 1, stat.st_size, pakFile);
    LERROR(bytesRead != stat.st_size);
    INFO("PAK file read");

    u8* bspBytes = loadFileFromPAK(buffer, stat.st_size, "maps/q3dm17.bsp");
    free(buffer);
    INFO("BSP file unpacked");

    free(bspBytes);
    INFO("Memory freed");

    return 0;
}
