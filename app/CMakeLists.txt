# Define the minimum CMAKE version required:
cmake_minimum_required(VERSION 3.11)

project(DeformableProjection C CXX)

set(USE_IMGUI_GLAD OFF)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Set C++ stardard:
set(CMAKE_CXX_STANDARD 17)

# Enable automatic include of generated files (like paths.h):
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(USE_IMGUI_GLAD OFF)

add_subdirectory(lib/imgui-docking-1.91.4/)
add_subdirectory(lib/glad/)

find_package(OpenCV CONFIG REQUIRED)
find_package(PCL REQUIRED)

# Define all header files we want to compile:
set(HEADERS
    src/ContextManager.h
    src/Data.h

    src/controller/calibration/CalibrationManager.h
    src/controller/calibration/CalibrationState.h
    src/controller/calibration/Calibrator.h
    src/controller/calibration/CheckerboardCollector.h
    src/controller/calibration/DepthReconstructor.h
    src/controller/calibration/IntensityFinder.h
    src/controller/calibration/MarkerAligner.h

    src/processing/blendpcr/CameraPasses.h
    src/processing/blendpcr/Rectification.h
    src/processing/blendpcr/BlendPCRRenderer.h
    src/processing/blendpcr/ShadowAvoidance.h

    src/processing/devices/RGBDCamera.h
    src/processing/devices/RGBDCameraManager.h
    src/processing/devices/orbbec/OrbbecCamera.h
    src/processing/devices/orbbec/OrbbecCameraProvider.h

    src/processing/uirenderer/UIRenderer.h
    src/processing/uirenderer/StaticImageUIRenderer.h

    src/processing/OrganizedPointCloud.h

    src/simulation/util/DebugDraw.h
    src/simulation/util/NoiseTexture2D.h
    src/simulation/util/PostProcessing.h

    src/simulation/scene/Camera.h
    src/simulation/scene/SceneComponent.h
    src/simulation/scene/SceneComposite.h
    src/simulation/scene/SceneData.h
    src/simulation/scene/Scene.h

    src/simulation/scene/components/ProjectionSurface.h

    src/simulation/scene/components/CoordinateSystem.h
    src/simulation/scene/components/Image2D.h
    src/simulation/scene/components/PointLight.h
    src/simulation/scene/components/Projector.h
    src/simulation/scene/components/RectifiedProjection.h
    src/simulation/scene/components/VirtualRGBDCamera.h

    src/ui/GUI.h
    src/ui/PipelineVisualization.h

    src/Semaphore.h

    src/gl/Shader.h
    src/gl/Texture2D.h
    src/gl/TextureFBO.h
    src/gl/Mesh.h
    src/gl/GLRenderable.h

    src/gl/primitive/Triangle.h
    src/gl/primitive/Vertex.h
    src/gl/primitive/TexCoord.h

    src/gl/objects/GLCoordinateSystem.h

    src/math/Vec4.h
    src/math/Mat4.h

    src/stb_image/stb_image.h
    src/tiny_obj_loader/tiny_obj_loader.h
)

# Define all source files we want to compile:
set(SOURCES 
    src/main.cpp

    src/Data.cpp

    src/processing/OrganizedPointCloud.cpp
    src/processing/devices/RGBDCamera.cpp
    src/processing/devices/RGBDCameraManager.cpp
    src/processing/devices/orbbec/OrbbecCameraProvider.cpp

    src/processing/blendpcr/CameraPasses.cpp
    src/processing/blendpcr/ShadowAvoidance.cpp

    src/simulation/util/NoiseTexture2D.cpp

    src/simulation/scene/Scene.cpp
    src/simulation/scene/components/Projector.cpp
    src/simulation/scene/components/RectifiedProjection.cpp

    src/simulation/scene/components/ProjectionSurface.cpp

    src/ui/PipelineVisualization.cpp

    src/gl/Shader.cpp
    src/gl/Texture2D.cpp
    src/gl/TextureFBO.cpp
    src/gl/Mesh.cpp

    src/gl/objects/GLCoordinateSystem.cpp
)

# Define shader & resources which should be listed in IDE:
set(RESOURCES
    shader/postShader.vert
    shader/postShader.frag
    shader/projectorScreen.vert
    shader/projectorScreen.frag
    shader/rgbdFstPass.vert
    shader/rgbdFstPass.frag
    shader/rgbdSndPass.vert
    shader/rgbdSndPass.frag
    shader/projectionSurface.vert
    shader/projectionSurface.frag
    shader/simpleLighting.vert
    shader/simpleLighting.frag
    shader/unshaded.vert
    shader/unshaded.frag
)

# Define executables with source files and resources:
add_executable(DeformableProjection ${SOURCES} ${HEADERS} ${RESOURCES})

target_compile_definitions(DeformableProjection PUBLIC -DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# OpenCV
target_link_libraries(DeformableProjection PUBLIC opencv_ml opencv_dnn opencv_calib3d opencv_flann opencv_highgui)

# PCL
target_link_libraries(DeformableProjection PRIVATE ${PCL_LIBRARIES})
target_include_directories(DeformableProjection PRIVATE ${PCL_INCLUDE_DIRS})

target_compile_definitions(DeformableProjection PUBLIC USE_ORBBEC)
set(OrbbecSDK_DIR lib/orbbecsdk-2.4.11/lib)
find_package(OrbbecSDK REQUIRED)
target_include_directories(DeformableProjection PUBLIC ${OrbbecSDK_INCLUDE_DIRS})
target_link_libraries(DeformableProjection PUBLIC ob::OrbbecSDK)

# Glad and ImGui:
target_link_libraries(DeformableProjection PUBLIC imgui glad)


# OpenMP
target_link_libraries(DeformableProjection PUBLIC OpenMP::OpenMP_CXX)
