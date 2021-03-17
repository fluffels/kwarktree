void
parseBSP(u8* bytes) {
    if (strncmp((char*)bytes, "IBSP", 4) != 0) {
        FATAL("not a valid IBSP file");
    }
}
