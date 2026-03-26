static BaseString baseMakeString(char *text, uint64_t size) {
    BaseString result = {0};

    result.text = text;
    result.size = size;

    return result;
}

static bool baseArenaInit(
    BaseArena *arena, uint64_t size
) {
    bool result = false;

    void *buffer = platformAllocate(size);

    if (buffer) {
        arena->capacity = size;
        arena->buffer = buffer;
        result = true;
    }

    return result;
}

static void *baseArenaPushSize(BaseArena *arena, uint64_t size) {
    uint64_t newUsed = arena->used + size;

    uint64_t alignement = 8;

    if (newUsed % alignement != 0) {
        newUsed = ((newUsed / alignement) + 1) * alignement;
    }

    BASE_ASSERT(newUsed <= arena->capacity);

    void *result = ((uint8_t *) arena->buffer) + arena->used;

    arena->used = newUsed;

    return result;
}

static BaseColor baseMakeColor(float r, float g, float b, float a) {
    BaseColor result = {r, g, b, a};

    return result;
}
