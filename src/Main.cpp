#include <Windows.h>

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

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

#include "jcwk/MathLib.cpp"

#pragma pack(push, 1)
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

struct BSPDirEntry {
    u32 offset;
    u32 length;
};

struct BSPEntity {
    char className[255];
    Vec3 origin;
    int angle;
    int spawnflags;
};

struct BSPHeader {
    char sig[4];
    u32 version;
    BSPDirEntry entities;
    BSPDirEntry textures;
    BSPDirEntry planes;
    BSPDirEntry nodes;
    BSPDirEntry leafs;
    BSPDirEntry leafFaces;
    BSPDirEntry leafBrushes;
    BSPDirEntry models;
    BSPDirEntry brushes;
    BSPDirEntry brushSides;
    BSPDirEntry vertices;
    BSPDirEntry meshVerts;
    BSPDirEntry effects; 
    BSPDirEntry faces;
    BSPDirEntry lightMaps;
    BSPDirEntry lightVols;
    BSPDirEntry visData;
};

struct BSPTexture {
    char name[64];
    u32 flags;
    u32 contents;
};

struct BSPVertex {
    Vec3 position;
    TexCoord texCoord[2];
    Vec3 normal;
    Color color;
};

struct BSPFace {
    u32 texture;
    i32 effect;
    u32 type;
    u32 vertex;
    u32 vertexCount;
    u32 meshVert;
    u32 meshVertCount;
    u32 lightMap;
    u32 lightMapStart[2];
    u32 lightMapSize[2];
    Vec3 lightMapWorldOrigin;
    Vec3 lightMapWorldDirs[2];
    Vec3 normal;
    u32 size[2];
};

struct EOCD {
    char sig[4];
    u16 disk;
    u16 cdrStartDisk;
    u16 cdrsOnDisk;
    u16 cdrCount;
    u32 cdrSize;
    u32 cdrOffset;
    u16 commentLength;
};

struct CDRecord {
    char sig[4];
    u16 createVersion;
    u16 requiredVersion;
    u16 flags;
    u16 method;
    u16 modTime;
    u16 modDate;
    u32 crc;
    u32 compressedSize;
    u32 uncompressedSize;
    u16 fnameLength;
    u16 extraFieldLength;
    u16 fileCommentLength;
    u16 startDisk;
    u16 internalFileAttributes;
    u32 externalFileAttributes;
    u32 localFileHeaderOffset;
};

struct LocalFileHeader {
    char sig[4];
    u16 requiredVersion;
    u16 flags;
    u16 method;
    u16 modTime;
    u16 modDate;
    u32 crc;
    u32 compressedSize;
    u32 uncompressedSize;
    u16 fnameLength;
    u16 extraFieldLength;
};

struct Uniforms {
    float proj[16];
    Vec4 eye;
    Quaternion rotation;
};
#pragma pack(pop)

#define READ(buffer, type, offset) (type*)(buffer + offset)

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PNG
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "stb_image.h"

#include "puff.c"
#include "jcwk/FileSystem.cpp"
#include "jcwk/Win32/DirectInput.cpp"
#include "jcwk/Win32/Controller.cpp"
#include "jcwk/Win32/Mouse.cpp"
#include "jcwk/Vulkan.cpp"
#include <vulkan/vulkan_win32.h>

const float DELTA_MOVE_PER_S = 10.f;
const float MOUSE_SENSITIVITY = 0.1f;
const float JOYSTICK_SENSITIVITY = 5;
bool keyboard[VK_OEM_CLEAR] = {};

LRESULT
WindowProc(
    HWND    window,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) PostQuitMessage(0);
            else keyboard[(uint16_t)wParam] = true;
            break;
        case WM_KEYUP:
            keyboard[(uint16_t)wParam] = false;
            break;
    }
    return DefWindowProc(window, message, wParam, lParam);
}

CDRecord*
findFileInPAK(
    char* pakBytes,
    EOCD& eocd,
    const char* path
) {
    auto pathLength = strlen(path);
    u16 index = 0;
    char* ptr = pakBytes + eocd.cdrOffset;
    CDRecord* record;
    while (index < eocd.cdrCount) {
        record = (CDRecord*)ptr;
        char* fname = ptr + sizeof(CDRecord);
        auto maxLen = pathLength <= record->fnameLength
            ? pathLength
            : record->fnameLength;
        if ((record->fnameLength >= pathLength) &&
            (strncmp(path, fname, maxLen) == 0))
            return record;
        ptr += sizeof(CDRecord);
        ptr += record->fnameLength;
        ptr += record->extraFieldLength;
        ptr += record->fileCommentLength;
        index++;
    }
    return nullptr;
}

u8*
unpackFile(
    char* pakBytes,
    CDRecord* record,
    u32* uncompressedLength = NULL
) {
    auto localHeader = (LocalFileHeader*)(pakBytes + record->localFileHeaderOffset);
    
    unsigned long uncompressedLen = localHeader->uncompressedSize;
    auto result = (u8*)malloc(uncompressedLen);
    unsigned long compressedLen = localHeader->compressedSize;
    u8* compressedBytes = (u8*)(pakBytes +
        record->localFileHeaderOffset +
        sizeof(LocalFileHeader) +
        localHeader->fnameLength +
        localHeader->extraFieldLength);

    puff(
        result, &uncompressedLen,
        compressedBytes, &compressedLen
    );

    if (uncompressedLength) {
        *uncompressedLength = uncompressedLen;
    }

    return result;
}

int
WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR commandLine,
    int showCommand
) {
    // NOTE: Initialize logging.
    {
        auto error = fopen_s(&logFile, "LOG", "w");
        if (error) return 1;

        QueryPerformanceCounter(&counterEpoch);
        QueryPerformanceFrequency(&counterFrequency);
    }

    // NOTE: Create window.
    HWND window = NULL;
    {
        WNDCLASSEX windowClassProperties = {};
        windowClassProperties.cbSize = sizeof(windowClassProperties);
        windowClassProperties.style = CS_HREDRAW | CS_VREDRAW;
        windowClassProperties.lpfnWndProc = WindowProc;
        windowClassProperties.hInstance = instance;
        windowClassProperties.lpszClassName = "MainWindowClass";
        ATOM windowClass = RegisterClassEx(&windowClassProperties);
        CHECK(windowClass, "Could not create window class");

        window = CreateWindowEx(
            0,
            "MainWindowClass",
            "Vk Mesh Shader Example",
            WS_POPUP | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            800,
            800,
            NULL,
            NULL,
            instance,
            NULL
        );
        CHECK(window, "Could not create window");

        SetWindowPos(
            window,
            HWND_TOP,
            0,
            0,
            GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN),
            SWP_FRAMECHANGED
        );
        ShowCursor(FALSE);

        INFO("Window created");
    }

    // Create Vulkan instance.
    Vulkan vk;
    vk.extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    createVKInstance(vk);
    INFO("Vulkan instance created");

    // Create Windows surface.
    {
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = instance;
        createInfo.hwnd = window;

        auto result = vkCreateWin32SurfaceKHR(
            vk.handle,
            &createInfo,
            nullptr,
            &vk.swap.surface
        );
        VKCHECK(result, "could not create win32 surface");
        INFO("Surface created");
    }

    // Initialize Vulkan.
    initVK(vk);
    INFO("Vulkan initialized");

    // Load PAK.
    char* pakBytes;
    EOCD* eocd;
    {
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
        pakBytes = (char*)malloc(stat.st_size);
        INFO("Data allocated");

        auto bytesRead = fread(pakBytes, 1, stat.st_size, pakFile);
        LERROR(bytesRead != stat.st_size);
        INFO("PAK file read");

        if (strncmp(pakBytes, "PK", 2)) {
            FATAL("not a zip file");
        }

        if (pakBytes[2] != 0x03) {
            FATAL("wrong zip version: %d", pakBytes[2]);
        }

        auto c = pakBytes + bytesRead - 1;
        while ((c[0] != 'P') &&
            (c[1] != 'K') &&
            (c[2] != 5) &&
            (c[3] != 6) &&
            (c > pakBytes + 4)) {
            c--;
        }

        auto offset = c - bytesRead;
        if (offset == 0) {
            FATAL("invalid zip, no EOCD");
        }

        eocd = READ(c, EOCD, 0);
    }

    // Load map.
    u8* bspBytes;
    {
        CDRecord* record = findFileInPAK(pakBytes, *eocd, "maps/q3dm17.bsp");
        bspBytes = unpackFile(pakBytes, record);
        INFO("BSP file unpacked");
    }

    // Parse BSP.
    u32* indices = NULL;
    u32 indexCount = 0;
    BSPVertex* vertices = NULL;
    u32 vertexCount = 0;
    BSPEntity* entities = NULL;
    {
        auto& bspHeader = *READ(bspBytes, BSPHeader, 0);

        if (strncmp(bspHeader.sig, "IBSP", 4) != 0) {
            FATAL("not a valid IBSP file");
        }

        // Parse entitites.
        enum KEY {
            CLASS_NAME,
            ORIGIN,
            ANGLE,
            SPAWNFLAGS,
            UNKNOWN
        };
        KEY key = UNKNOWN;
        enum STATE {
            OUTSIDE_ENTITY,
            INSIDE_ENTITY,
            INSIDE_STRING,
        };
        STATE state = OUTSIDE_ENTITY;
        BSPEntity entity;

        u8* ePos = bspBytes + bspHeader.entities.offset;
        char buffer[255];
        memset(buffer, '\0', 255);
        char* bPos = buffer;
        while(ePos < bspBytes + bspHeader.entities.offset + bspHeader.entities.length) {
            char c = *ePos;
            if (state == OUTSIDE_ENTITY) {
                if (c == '{') {
                    state = INSIDE_ENTITY;
                    entity = {};
                }
                ePos++;
            } else if (state == INSIDE_ENTITY) {
                if (c == '"') {
                    bPos = buffer;
                    state = INSIDE_STRING;
                } else if (*ePos == '}') {
                    state = OUTSIDE_ENTITY;
                    arrput(entities, entity);
                }
                ePos++;
            } else if (state == INSIDE_STRING) {
                if (*ePos == '"') {
                    state = INSIDE_ENTITY;
                    *bPos = '\0';
                    if (key == CLASS_NAME) {
                        strncpy_s(entity.className, buffer, 255);
                    } else if (key == ORIGIN) {
                        char *s = strstr(buffer, " ");
                        *s = '\0';
                        entity.origin.x = (float)atoi(buffer);

                        char *n = s + 1;
                        s = strstr(n, " ");
                        *s = '\0';
                        entity.origin.z = (float)-atoi(n);

                        n = s + 1;
                        entity.origin.y = (float)-atoi(n);
                    } else if (key == ANGLE) {
                        entity.angle = atoi(buffer);
                    } else if (key == SPAWNFLAGS) {
                        entity.spawnflags = atoi(buffer);
                    }
                    if (strcmp("classname", buffer) == 0) key = CLASS_NAME;
                    else if (strcmp("origin", buffer) == 0) key = ORIGIN;
                    else if (strcmp("angle", buffer) == 0) key = ANGLE;
                    else if (strcmp("spawnflags", buffer) == 0) key = SPAWNFLAGS;
                    else key = UNKNOWN;
                } else {
                    *bPos = c;
                    bPos++;
                }
                ePos++;
            }
        }
        INFO("Entities parsed");

        // Parse textures.
        auto textureCount = bspHeader.textures.length / sizeof(BSPTexture);
        auto textures = (BSPTexture*)(bspBytes + bspHeader.textures.offset);
        for (int i = 0; i < textureCount; i++) {
            auto& texture = textures[i];
            if (strcmp(texture.name, "noshader\0") == 0) {
                continue;
            }
            auto* record = findFileInPAK(pakBytes, *eocd, texture.name);
            if (record == nullptr) {
                INFO("could not find file: '%s'", texture.name);
                continue;
            }
            u32 tgaLen = 0;
            auto tgaBytes = unpackFile(pakBytes, record, &tgaLen);
            int x, y, n;
            u8* data = stbi_load_from_memory(
                tgaBytes, tgaLen, &x, &y, &n, 4
            );
            if (data == nullptr) {
                ERR("could not load texture: '%s'", texture.name);
                continue;
            } else {
                VulkanSampler sampler = {};
                uploadTexture(
                    vk.device,
                    vk.memories,
                    vk.queue,
                    vk.queueFamily,
                    vk.cmdPoolTransient,
                    x,
                    y,
                    data,
                    x * y * 4,
                    sampler
                );
            }

            free(data);
            free(tgaBytes);
        }

        // Parse vertices.
        vertexCount = bspHeader.vertices.length / sizeof(BSPVertex);
        vertices = (BSPVertex*)(bspBytes + bspHeader.vertices.offset);

        auto meshVertexCount = bspHeader.meshVerts.length / sizeof(u32);
        auto meshVertices = (u32*)(bspBytes + bspHeader.meshVerts.offset);

        auto faceCount = bspHeader.faces.length / sizeof(BSPFace);
        auto faceVertices = (BSPFace*)(bspBytes + bspHeader.faces.offset);

        for (int faceIdx = 0; faceIdx < faceCount; faceIdx++) {
            auto& face = faceVertices[faceIdx];
            if (face.type != 1) {
                continue;
            }
            for (int i = 0; i < face.meshVertCount; i++) {
                auto meshVertIdx = face.meshVert + i;
                auto meshVert = meshVertices[meshVertIdx];
                auto idx = face.vertex + meshVert;
                arrput(indices, idx);
                indexCount++;
            }
        }

        INFO("BSP file parsed");
    }

    // Record command buffers.
    VkCommandBuffer* cmds = NULL;
    {
        VulkanPipeline pipeline;
        initVKPipeline(
            vk,
            "default",
            pipeline
        );

        VulkanMesh mesh = {};
        uploadMesh(
            vk.device,
            vk.memories,
            vk.queueFamily,
            vertices,
            vertexCount*sizeof(BSPVertex),
            indices,
            indexCount*sizeof(u32),
            mesh
        );

        updateUniformBuffer(
            vk.device,
            pipeline.descriptorSet,
            0,
            vk.uniforms.handle
        );

        u32 framebufferCount = vk.swap.images.size();
        arrsetlen(cmds, framebufferCount);
        createCommandBuffers(vk.device, vk.cmdPool, framebufferCount, cmds);
        for (size_t swapIdx = 0; swapIdx < framebufferCount; swapIdx++) {
            auto& cmd = cmds[swapIdx];
            beginFrameCommandBuffer(cmd);

            VkClearValue colorClear;
            colorClear.color = {};
            VkClearValue depthClear;
            depthClear.depthStencil = { 1.f, 0 };
            VkClearValue clears[] = { colorClear, depthClear };

            VkRenderPassBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.clearValueCount = 2;
            beginInfo.pClearValues = clears;
            beginInfo.framebuffer = vk.swap.framebuffers[swapIdx];
            beginInfo.renderArea.extent = vk.swap.extent;
            beginInfo.renderArea.offset = {0, 0};
            beginInfo.renderPass = vk.renderPass;

            vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.handle
            );
            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout,
                0,
                1,
                &pipeline.descriptorSet,
                0,
                nullptr
            );
            VkDeviceSize offsets[] = {0};

            vkCmdBindVertexBuffers(
                cmd,
                0, 1,
                &mesh.vBuff.handle,
                offsets
            );
            vkCmdBindIndexBuffer(
                cmd,
                mesh.iBuff.handle,
                0,
                VK_INDEX_TYPE_UINT32
            );
            Vec3 white = {1, 1, 1};
            vkCmdPushConstants(
                cmd,
                pipeline.layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(white),
                &white
            );
            vkCmdDrawIndexed(
                cmd,
                indexCount,
                1, 0, 0, 0
            );

            vkCmdEndRenderPass(cmd);

            VKCHECK(vkEndCommandBuffer(cmd));
        }
    }
    free(pakBytes);
    free(bspBytes);
    arrfree(indices);

    // Set up state.
    Uniforms uniforms = {};
    {
        matrixInit(uniforms.proj);
        matrixProjection(
            vk.swap.extent.width,
            vk.swap.extent.height,
            toRadians(45),
            10.f,
            .1f,
            uniforms.proj
        );

        quaternionInit(uniforms.rotation);

        // Find player spawn.
        for (int i = 0; i < arrlen(entities); i++) {
            auto& entity = entities[i];
            if (strcmp(entity.className, "info_player_deathmatch") == 0) {
                uniforms.eye.x = entity.origin.x;
                uniforms.eye.y = entity.origin.y;
                uniforms.eye.z = entity.origin.z;
                rotateQuaternionY(-(float)entity.angle, uniforms.rotation);
                break;
            }
        }
    }

    // Initialize DirectInput.
    DirectInput directInput(instance);
    auto mouse = directInput.mouse;
    auto controller = directInput.controller;

    // Main loop.
    LARGE_INTEGER frameStart = {};
    LARGE_INTEGER frameEnd = {};
    i64 frameDelta = 0;
    BOOL done = false;
    int errorCode = 0;
    while (!done) {
        QueryPerformanceCounter(&frameStart);

        MSG msg;
        BOOL messageAvailable; 
        do {
            messageAvailable = PeekMessage(
                &msg,
                (HWND)nullptr,
                0, 0,
                PM_REMOVE
            );
            TranslateMessage(&msg); 
            if (msg.message == WM_QUIT) {
                done = true;
                errorCode = (int)msg.wParam;
            }
            DispatchMessage(&msg); 
        } while(!done && messageAvailable);

        if (done) {
            break;
        }

        // Render frame.
        updateUniforms(vk, &uniforms, sizeof(uniforms));
        present(vk, cmds, 1);

        // Frame rate independent movement stuff.
        QueryPerformanceCounter(&frameEnd);
        float frameTime = (frameEnd.QuadPart - frameStart.QuadPart) /
            (float)counterFrequency.QuadPart;
        float moveDelta = DELTA_MOVE_PER_S * frameTime;

        // Keyboard.
        if (keyboard['W']) {
            moveAlongQuaternion(moveDelta, uniforms.rotation, uniforms.eye);
        }
        if (keyboard['S']) {
            moveAlongQuaternion(-moveDelta, uniforms.rotation, uniforms.eye);
        }
        if (keyboard['A']) {
            movePerpendicularToQuaternion(-moveDelta, uniforms.rotation, uniforms.eye);
        }
        if (keyboard['D']) {
            movePerpendicularToQuaternion(moveDelta, uniforms.rotation, uniforms.eye);
        }

        // Mouse.
        Vec2i mouseDelta = mouse->getDelta();
        auto mouseDeltaX = mouseDelta.x * MOUSE_SENSITIVITY;
        auto mouseDeltaY = mouseDelta.y * MOUSE_SENSITIVITY;
        rotateQuaternionX(mouseDeltaY, uniforms.rotation);
        rotateQuaternionY(-mouseDeltaX, uniforms.rotation);
    }
    arrfree(cmds);

    return errorCode;
}
