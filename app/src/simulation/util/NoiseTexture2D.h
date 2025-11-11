// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Yaroslav Purgin

// Include this file only once when compiling:
#pragma once

#include <vector>

/**
 * This class generates a white noise texture which will be stored on the GPU.
 */
class NoiseTexture2D {
private:

    /** Generates a vector of RED channel noise data */
    static std::vector<unsigned char> generateNoiseDataRED(int width, int height);

public:
    /** Stores the ID of the texture on the GPU */
    unsigned int texture = 0;
    
    // Copy counter (This is needed for correct creation and deletion
    // of shaderProgram on GPU, since C++ copies objects on reassignment,
    // see copy constructor. In general, I would recommend holding Mesh,
    // Shader, Texture2D, etc. objects by a shared_ptr instead of
    // doing this, because that ensures that exactly one explicitly created
    // mesh object exists, but we don't want to introduce 'Smart Pointers'
    // in Computergrafik 1 - just as a hint if you want to create your
    // own engine):
    int* numOfCopies;

    /**
     * Creates an empty texture of the given type and dimensions.
     */
    NoiseTexture2D(unsigned int width, unsigned int height);

    /**
     * Explicit copy constructor for reference counting (for correct
     * texture deletion on GPU).
     */
    NoiseTexture2D(const NoiseTexture2D& t);

    /**
     * Deletes the texture and ensures that the memory on the GPU
     * is cleaned up.
     */
    ~NoiseTexture2D();

    /**
     * Binds the texture so that it can be used in the shader.
     *
     * @textureSlot     Defines the slot where the texture should
     *                  be bound to.
     */
    void bind(int textureSlot = 0);
};
