void ttsUpdate(Platform *platform, Atlas *atlas) {
    static float x = 0.0f;
    platformDrawTextureQuad(
        10, 10.0f,   (float)atlas->width, (float)atlas->height,
        0.0f, 0.0f,
        (float)atlas->width, (float)atlas-> height,
        (float)atlas->width, (float)atlas-> height,
        0.0f, 0.0f, 1.0f, 1.0f,
        platform
    );

    platformDrawColorQuad(
        x, 100.0f,
        100.0f, 50.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        platform
    );

    x += 5.0f;
    if (x > 600.0f) {
        x = 0;
    }
}
