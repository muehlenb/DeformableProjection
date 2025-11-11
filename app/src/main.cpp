// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Yaroslav Purgin, Andre Mühlenbrock (muehlenb@uni-bremen.de)

// Include the GUI:
#include "src/ui/GUI.h"

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

// Include GLFW/OpenGL:
#include <GLFW/glfw3.h>

// Include IO utilities:
#include <iostream>

// Include Camera:
#include "src/simulation/scene/Camera.h"

// Scene:
#include "src/simulation/scene/Scene.h"
#include "src/simulation/scene/components/RectifiedProjection.h"

//#include "src/processing/uirenderer/StroopUIRenderer.h"

// Include Mat4f class:
#include "src/math/Mat4.h"

// Include post-processing class:
#include "src/simulation/util/PostProcessing.h"

#include "src/processing/blendpcr/BlendPCRRenderer.h"

#include <implot.h>

#include "src/controller/calibration/CalibrationManager.h"

#include "src/Data.h"
#include "src/simulation/util/DebugDraw.h"

#include "ContextManager.h"

// Function declaration. This function will handle calculating the frame time and passing it to data:
void calculateTime(const double& startTime, const double& prevTime);

/*
void glErrorDebugCallback(const char *name, void *funcptr, int len_args, ...) {
    GLenum error_code;
    error_code = glad_glGetError();

    if (error_code != GL_NO_ERROR) {
        fprintf(stderr, "GL Warning %d in %s\n", error_code, name);
    }
}*/

/**
 * Initializes the application including the GUI.
 */
int main(int, char**)
{
    // Initialize the context manager and thus also the main window, OpenGL, ImGui, and related resources.
    GLFWwindow* mainWindow = ContextManager::initialize();
    if (!mainWindow) {
        return EXIT_FAILURE;
    }

    //glad_set_post_callback(glErrorDebugCallback);

    std::shared_ptr<Shader> unlitColorShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/unlitColor.vert", CMAKE_SOURCE_DIR "/shader/unlitColor.frag");
    std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(CMAKE_SOURCE_DIR "/models/unit-cube.obj");

    std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(CMAKE_SOURCE_DIR "/models/unit-sphere.obj");
    std::shared_ptr<Mesh> screenMesh = std::make_shared<Mesh>(CMAKE_SOURCE_DIR "/models/unit-screen.obj");

    Data::instance.camera = std::make_shared<Camera>();
    // The surgical scene:
    SurgicalScene scene;

    ImGuiContext* mainImGuiContext = ImGui::GetCurrentContext();

    ImGuiContext* uiRendererImGuiContext = ImGui::CreateContext();
    ImPlotContext* uiRendererImPlotContext = ImPlot::CreateContext();
    ImGui::SetCurrentContext(uiRendererImGuiContext);
    ImPlot::SetCurrentContext(uiRendererImPlotContext);

    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiIO& uiio = ImGui::GetIO();

    // Load Roboto Font:
    uiio.Fonts->AddFontFromFileTTF(CMAKE_SOURCE_DIR "/lib/imgui-docking-1.91.4/misc/fonts/Arial.ttf", 38.0f);

    // UI Renderer:
    std::shared_ptr<UIRenderer> uiRenderer = std::make_shared<StaticImageUIRenderer>();

    ImGui::SetCurrentContext(mainImGuiContext);

    // Rectification:
    std::shared_ptr<RectifiedProjection> rectifiedProjection = std::make_shared<RectifiedProjection>(uiRenderer);

    // Post-processing:
    PostProcessing pProcessing;

    // Ensure PCTextureProcessor is instanciated (this must be instanciated in the OpenGL thread,
    // since shaders are loaded. This function is also called NOT within the OpenGL thread
    // when new point clouds are available, so by calling it here, we ensure early correct instanciation:
    CameraPasses::getInstance();


    // Register CameraPasses as Callback, so that it can manage the point clouds:
    Data::instance.cameraManager.registerCallback([](int deviceIndex, std::shared_ptr<OrganizedPointCloud> pc){
        CameraPasses::getInstance().insertNewPointCloud(deviceIndex, pc);
    });

    BlendPCRRenderer blendPCRRenderer;

#ifdef USE_CUDA
    //pointCloudPipeline.addFilter(std::make_shared<TemporalNoiseFilter>());
    //pointCloudPipeline.addFilter(std::make_shared<SpatialHoleFiller>());
    //pointCloudPipeline.addFilter(std::make_shared<ErosionFilter>());
#endif

    CalibrationManager calibrationManager;
    std::shared_ptr<CoordinateSystem> coordinateSystem = std::make_shared<CoordinateSystem>();
    
    // Variables for frame time calculation:
    double startTime = glfwGetTime();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    Data::instance.cameraManager.load();

    // Main loop which is executed every frame until the window is closed:
    while (!glfwWindowShouldClose(mainWindow)) {
        double prevTime = glfwGetTime();

        auto start = high_resolution_clock::now();

        if(Data::instance.cameraManager.requiresSimulatedRGBDData()){
            std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds;
            for (const std::shared_ptr<VirtualRGBDCamera>& rgbdCam : Data::instance.rgbdCameras) {
                if (!rgbdCam->active)
                    continue;

                rgbdCam->renderRGBD(scene);
                pointClouds.push_back(rgbdCam->getOrganizedPointCloud());
            }

            for(int i=0; i < pointClouds.size(); ++i){
                Data::instance.cameraManager.pointCloudCallback(i, pointClouds[i]);
            }
        }
        //glFlush();

        // Render UI:
        ImGui::SetCurrentContext(uiRendererImGuiContext);
        ImPlot::SetCurrentContext(uiRendererImPlotContext);
        uiRenderer->render(0.f, 0.f, false);
        ImGui::SetCurrentContext(mainImGuiContext);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Set up the GL viewport:
        glClearColor(0.6f, 0.725f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Processes all glfw events:
        glfwPollEvents();

        // Get the size of the window:
        int display_w, display_h;
        glfwGetFramebufferSize(mainWindow, &display_w, &display_h);

        // Subtract right menu:
        display_w -= Data::instance.menuWidth;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create the GUI:
        Data::instance.display_w = display_w;
        Data::instance.display_h = display_h;
        GUI::drawGui();

        // Control the camera:
        Data::instance.camera->processImGuiInput();

        // Render the first pass (to the firstPassFBO):
        {
            // The scene data:
            SceneData sceneData(SD_MAIN);

            // If rectification should follow camera, update target pos:
            if (Data::instance.rectifyFollowCam) {
                Data::instance.virtualDisplaySpectator = Data::instance.camera->getView().inverse().getPosition();
            }

            sceneData.projection = Data::instance.camera->getProjection(display_w, display_h);
            sceneData.view = Data::instance.camera->getView();

            // Activate the firstPassFBO to render to its textures instead
            // of rendering directly to the screen:
            pProcessing.getFirstPassFBO()->bind(display_w, display_h);

            // Setup viewport and clear color & depth image:
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.6f, 0.725f, 0.8f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // GL Tick (e.g. uploading new point clouds, etc.):
            CameraPasses::getInstance().glTick();
            rectifiedProjection->shadowAvoidance->glTick();

            // Prepare scene data:
            scene.prepare(sceneData, Mat4f());

            // Render the scene:
            scene.render(sceneData, Mat4f());

            if(Data::instance.renderRawPointCloud){
                blendPCRRenderer.render(sceneData.projection, sceneData.view);
            }

            Vec4f targetPos = Data::instance.virtualDisplaySpectator;
            coordinateSystem->transformation = Mat4f::translation(targetPos.x, targetPos.y, targetPos.z);
            {
                Mat4f model = Data::instance.virtualDisplayTransform * Mat4f::translation(-0.5f, -0.5f, 0.0f);
                unlitColorShader->bind();
                unlitColorShader->setUniform("projection", sceneData.projection);
                unlitColorShader->setUniform("view", sceneData.view);
                unlitColorShader->setUniform("model", model);
                unlitColorShader->setUniform("color", Vec4f(0.f, 0.f, 1.f, 1.f));
                screenMesh->render();
            }

            // Align virtual cameras with real ones:
            /*
            int i=0;
            for(std::shared_ptr<VirtualRGBDCamera> camera : Data::instance.rgbdCameras){
                if(RGBDCamera::globalCameraMatrices.size() > i)
                    camera->transformation = Streamer::globalModelMatrices[i];

                i++;
            }*/
        }

        // Render the second pass (to the screen):
        pProcessing.render(display_w, display_h);

        // Tick the calibrationManager:
        calibrationManager.tick();

        ContextManager::renderProjectorWindows();

        //ImGui::ShowMetricsWindow();

        // Render the GUI and draw it to the screen:
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows.
        // Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere:
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backupCurrentContext);
        }

        calculateTime(startTime, prevTime);

        // Swap Buffers:
        glfwSwapBuffers(mainWindow);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(mainWindow);
    glfwTerminate();

    return EXIT_SUCCESS;
}

void calculateTime(const double& startTime, const double& prevTime) {
    //glFinish();
    double nowTime = glfwGetTime();
    const float frameDuration = static_cast<float>(nowTime - prevTime) * 1000.f; // in ms

    //std::cout << "TotalGL:" << frameDuration << std::endl;

    // UI value adjusts slowly over time for better readability:
    Data::instance.timingFrame = Data::instance.timingFrame * 0.9f + frameDuration * 0.1f;
    Data::instance.runtime = static_cast<float>(nowTime - startTime);
}
