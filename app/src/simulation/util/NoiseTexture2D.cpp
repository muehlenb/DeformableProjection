#include "src/simulation/util/NoiseTexture2D.h"

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

std::vector<unsigned char> NoiseTexture2D::generateNoiseDataRED(int width, int height) {
    std::vector<unsigned char> noiseData(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char noiseValue = static_cast<unsigned char>(rand() % 256); // Random value between 0 and 255
            int index = y * width + x;
            noiseData[index] = noiseValue;
        }
    }

    return noiseData;
}

NoiseTexture2D::NoiseTexture2D(unsigned int width, unsigned int height)
    :numOfCopies(new int(1)) {

    // Generate the noise data:
    const std::vector<unsigned char>& noiseData = generateNoiseDataRED(width, height);

    // Generate a texture name:
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Define how the texture should behave when exceeding coordinates of [0,1]:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Define how the texture should be interpolated when it is rendered smaller
    // or bigger that the original image size on the screen:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Allocate the actual space in the GPU where the pixels of the texture are stored
    // according to the given type and size:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, noiseData.data());

    // Deactivate the texture:
    glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * This is an explicit copy constructor which is automatically
 * called every time an existing Texture is assigned to a variable
 * via the = Operator. Not important for CG1 and C++ specific,
 * but it is needed here to avoid problems because of early
 * deletion of the texture (on GPU) when the destructor is called.
 */
NoiseTexture2D::NoiseTexture2D(const NoiseTexture2D& t) {
    texture = t.texture;
    numOfCopies = t.numOfCopies;
    ++(*numOfCopies);
}

NoiseTexture2D::~NoiseTexture2D() {
    // We only want the texture to be destroyed
    // when the last copy of the texture is deleted:
    if (--(*numOfCopies) > 0)
        return;

    // If there is a compiled & linked shaderProgram, delete it from the GPU:
    if (texture != 0)
        glDeleteTextures(1, &texture);

    // To avoid reusage after deletion:
    texture = 0;

    // Delete also the numOfCopies variable:
    delete numOfCopies;
}

void NoiseTexture2D::bind(int textureSlot) {
    glActiveTexture(GL_TEXTURE0 + textureSlot);

    // Activates the texture for the following rendering
    glBindTexture(GL_TEXTURE_2D, texture);
}
