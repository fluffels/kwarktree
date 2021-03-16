#include "puff.c"

#define READ(buffer, type, offset) *(type*)(buffer + offset)

#pragma pack(push, 1)
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

struct Header {
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
#pragma pack(pop)

u8*
loadFileFromPAK(
    char* buffer,
    i64 bufferSize,
    const char* mapName
) {
    if (strncmp(buffer, "PK", 2)) {
        FATAL("not a zip file");
    }

    if (buffer[2] != 0x03) {
        FATAL("wrong zip version: %d", buffer[2]);
    }

    auto c = buffer + bufferSize - 1;
    while ((c[0] != 'P') &&
           (c[1] != 'K') &&
           (c[2] != 5) &&
           (c[3] != 6) &&
           (c > buffer + 4)) {
        c--;
    }

    auto offset = c - buffer;
    if (offset == 0) {
        FATAL("invalid zip, no EOCD");
    }

    auto eocd = READ(c, EOCD, 0);

    u16 index = 0;
    char* ptr = buffer + eocd.cdrOffset;
    CDRecord* record = nullptr;
    auto mapNameLength = strlen(mapName);
    while (index < eocd.cdrCount) {
        record = (CDRecord*)ptr;
        char* fname = ptr + sizeof(CDRecord);
        if ((record->fnameLength == mapNameLength) &&
            (strncmp(mapName, fname, record->fnameLength) == 0))
            break;
        ptr += sizeof(CDRecord);
        ptr += record->fnameLength;
        ptr += record->extraFieldLength;
        ptr += record->fileCommentLength;
        index++;
    }

    auto localHeader = (Header*)(buffer + record->localFileHeaderOffset);
    ptr = buffer +
        record->localFileHeaderOffset +
        sizeof(Header) +
        localHeader->fnameLength +
        localHeader->extraFieldLength;
    
    unsigned long destlen = localHeader->uncompressedSize;
    u8* dest = (u8*)malloc(destlen);
    unsigned long srclen = localHeader->compressedSize;
    u8* src = (u8*)ptr;

    puff(dest, &destlen, src, &srclen);

    return dest;
}
