#pragma pack(push, 1)
struct BSPDirEntry {
    u32 offset;
    u32 length;
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
#pragma pack(pop)

void
parseBSP(u8* bytes) {
    auto& bspHeader = *READ(bytes, BSPHeader, 0);

    if (strncmp(bspHeader.sig, "IBSP", 4) != 0) {
        FATAL("not a valid IBSP file");
    }

    auto vertexCount = bspHeader.vertices.length / sizeof(BSPVertex);
    auto vertices = (BSPVertex*)(bytes + bspHeader.vertices.offset);

    auto meshVertexCount = bspHeader.meshVerts.length / sizeof(u32);
    auto meshVertices = (u32*)(bytes + bspHeader.meshVerts.offset);

    auto faceCount = bspHeader.faces.length / sizeof(BSPFace);
    auto faceVertices = (BSPFace*)(bytes + bspHeader.faces.offset);
}
