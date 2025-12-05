void ttsUpdate(Platform *platform, Atlas *atlas) {
    platformDrawTextureQuad(
        10.0f, 10.0f,   (float)atlas->width, (float)atlas->height,
        0.0f, 0.0f,
        (float)atlas->width, (float)atlas-> height,
        (float)atlas->width, (float)atlas-> height,
        0.0f, 0.0f, 1.0f, 1.0f,
        platform
    );

    platformDrawColorQuad(
        100.0f, 100.0f,
        100.0f, 50.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        platform
    );
}
