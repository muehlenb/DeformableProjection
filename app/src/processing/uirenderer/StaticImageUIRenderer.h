// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include "src/processing/uirenderer/UIRenderer.h"

#include "src/gl/Texture2D.h"

class StaticImageUIRenderer : public UIRenderer {
    /** The texture the projector should project */
    Texture2D texture;

    unsigned int shadowTileTextureID;

    std::vector<ShadowTile> shadowTiles;

public:
    StaticImageUIRenderer()
        : texture(CMAKE_SOURCE_DIR "/textures/ctImage.png"){

        defineShadowTiles();
        createStaticSegmentTexture();
    }

    void defineShadowTiles(){
        shadowTiles.push_back(ShadowTile(180, 1080 - (1040), 1180-180, 1040-40));
        shadowTiles.push_back(ShadowTile(1200, 1080 - (380), 1740-1200, 380-74));
        shadowTiles.push_back(ShadowTile(1200, 1080 - ( 900), 1740-1200, 900-420));
        shadowTiles.push_back(ShadowTile(1200, 1080 - (1060), 1740-1200, 1060-900));
    }

    void createStaticSegmentTexture(){
        const int texWidth = 960;
        const int texHeight = 540;

        unsigned char* textureData = new unsigned char[texWidth * texHeight];

        for (int y = 0; y < texHeight; ++y) {
            for (int x = 0; x < texWidth; ++x) {
                textureData[y * texWidth + x] = 0;

                int id = 1;
                for(ShadowTile& tile : shadowTiles){
                    if(x >= tile.x && x <= tile.x + tile.width && y >= tile.y && y <= tile.y + tile.height){
                        textureData[y * texWidth + x] = id;
                        break;
                    }
                    ++id;
                }
            }
        }

        glGenTextures(1, &shadowTileTextureID);
        glBindTexture(GL_TEXTURE_2D, shadowTileTextureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, texWidth, texHeight, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, textureData);

        delete[] textureData;
    }


    virtual void render(float mouseX, float mouseY, int mousePressed) override{

    }

    virtual unsigned int getTexture() override {
        return texture.texture;
    }

    /*
    virtual void bindShadowTileTexture(unsigned int textureSlot) override {
        glActiveTexture(GL_TEXTURE0 + textureSlot);
        glBindTexture(GL_TEXTURE_2D, shadowTileTextureID);
    }*/

    virtual std::vector<ShadowTile>& getShadowTiles() override {
        return shadowTiles;
    }
};
