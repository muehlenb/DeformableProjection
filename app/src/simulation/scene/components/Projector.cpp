#include "src/simulation/scene/components/Projector.h"

std::shared_ptr<Mesh> Projector::projectorMesh = nullptr;
std::shared_ptr<Texture2D> Projector::textureActive = nullptr;
std::shared_ptr<Texture2D> Projector::textureInactive = nullptr;
std::shared_ptr<Shader> Projector::shader = nullptr;

int Projector::instanceCounter = 0;
