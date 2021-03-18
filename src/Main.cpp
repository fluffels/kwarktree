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

typedef float f32;
typedef double f64;

#pragma pack(push, 1)
struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
};

struct TexCoord {
    f32 s;
    f32 t;
};

struct Color {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};
#pragma pack(pop)

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

#define WARN(...) LOG("WARN", __VA_ARGS__);

#define ERR(...) LOG("ERROR", __VA_ARGS__);

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

#define READ(buffer, type, offset) (type*)(buffer + offset)

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "jcwk/FileSystem.cpp"
#include "jcwk/Vulkan.cpp"

#include "BSP.cpp"
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

    // TODO: This malloc is relatively expensive, as is the read following it.
    // It might be worth checking to see if mmap (or the Windows equivalent)
    // is faster since the program is reading relatively small chunks of data
    // in a random pattern instead of sequentially going through the whole
    // ~400 MB file.
    auto buffer = (char*)malloc(stat.st_size);
    INFO("Data allocated");

    auto bytesRead = fread(buffer, 1, stat.st_size, pakFile);
    LERROR(bytesRead != stat.st_size);
    INFO("PAK file read");

    u8* bspBytes = loadFileFromPAK(buffer, stat.st_size, "maps/q3dm17.bsp");
    free(buffer);
    INFO("BSP file unpacked");

    parseBSP(bspBytes);
    INFO("BSP file parsed");

    free(bspBytes);
    INFO("Memory freed");

    return 0;
}
