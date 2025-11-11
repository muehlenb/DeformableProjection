#include "src/simulation/scene/components/ProjectionSurface.h"

std::shared_ptr<Mesh> ProjectionPlane::surfaceMesh = nullptr;
std::shared_ptr<Texture2D> ProjectionPlane::surfaceTexture = nullptr;
std::shared_ptr<Shader> ProjectionPlane::shader = nullptr;
int ProjectionPlane::instanceCounter = 0;
