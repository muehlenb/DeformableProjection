// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include <glad/glad.h>
#include <vector>

#include "ShadowTile.h"

class UIRenderer {
public:
    virtual void render(float mouseX, float mouseY, int mousePressedState) = 0;
    virtual unsigned int getTexture() = 0;
    virtual std::vector<ShadowTile>& getShadowTiles() = 0;

    void bindTexture(unsigned int textureSlot){
        glActiveTexture(GL_TEXTURE0 + textureSlot);
        glBindTexture(GL_TEXTURE_2D, getTexture());
    }

    void bindShadowTileTexture(unsigned int textureSlot){
        // TODO: Construct and cache shadow tiles.
    }
};
