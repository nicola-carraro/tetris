static BaseString baseMakeString(char *text, uint64_t size) {
    BaseString result = {0};

    result.text = text;
    result.size = size;

    return result;
}

static bool baseArenaInit(BaseArena *arena, uint64_t size) {
    BASE_ASSERT(size % BASE_ALIGNEMENT == 0);
    bool result = false;

    void *buffer = platformAllocate(size);

    if (buffer) {
        BASE_ASSERT(((int64_t)buffer) % BASE_ALIGNEMENT == 0);
        ASAN_POISON_MEMORY_REGION(buffer, size);
        arena->capacity = size;
        arena->buffer = buffer;
        result = true;
    }

    return result;
}

static void baseArenaPushSize(BaseArena *arena, uint64_t size, void **result) {
    BASE_ASSERT(result);

    if (size > 0) {
        uint64_t padding = BASE_ALIGNEMENT * 4;
        uint64_t newUsed = arena->used + size + padding;

        if (newUsed % BASE_ALIGNEMENT != 0) {
            newUsed = ((newUsed / BASE_ALIGNEMENT) + 1) * BASE_ALIGNEMENT;
        }

        BASE_ASSERT(newUsed <= arena->capacity);

        *result = ((uint8_t *) arena->buffer) + arena->used;
        BASE_ASSERT(((int64_t)result) % BASE_ALIGNEMENT == 0);

        ASAN_UNPOISON_MEMORY_REGION(result, size);
        arena->used = newUsed;
    }
}

static BaseColor baseMakeColor(float r, float g, float b, float a) {
    BaseColor result = {r, g, b, a};

    return result;
}
