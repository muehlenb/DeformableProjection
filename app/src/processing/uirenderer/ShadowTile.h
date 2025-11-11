#pragma once

struct ShadowTile {
    int x;
    int y;
    int width;
    int height;

    float minDistanceP1[3];
    float minDistanceP2[3];
    float minDistanceP3[3];

    float minDistance[3]; // Per Projector

    int pixels[3];
    int majorProjector = -1;

    float projectorWeight[3] = {0.0, 0.0, 0.0};

    bool useFallback = false;

    ShadowTile(int x, int y, int width, int height)
        : x(x/2)
        , y(y/2)
        , width(width/2)
        , height(height/2){}
};
