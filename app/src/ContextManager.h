// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Yaroslav Purgin

#pragma once

#include "src/gl/Mesh.h"
#include "src/gl/Shader.h"

#include <GLFW/glfw3.h>

#include "src/simulation/scene/components/Projector.h"

#include "src/Data.h"

class ContextManager {
private:
    /**
     * Callback function for key events. Closes the window if the ESC key is pressed.
     */
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        // TODO: only for debug, remove later
        if (action == GLFW_PRESS && key == GLFW_KEY_F8)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

public:
    /**
     * Initializes the main window, sets up OpenGL, ImGui, and related resources.
     * Call this as first thing in a new application.
     */
    static GLFWwindow* initialize() {
        // Setup window:
        glfwSetErrorCallback([](int error, const char* description) {
            fprintf(stderr, "Glfw Error %d: %s\n", error, description);
        });

        if (!glfwInit())
            return nullptr;

        // Decide GL+GLSL versions:
        const char* glsl_version = "#version 330 core";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        // Start the window maximized:
        //glfwWindowHint(GLFW_MAXIMIZED, true);

         // Get video mode of the primary monitor (for window size and positioning):
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* primaryMode = glfwGetVideoMode(primaryMonitor);
        if (!primaryMode) {
            return nullptr;
        }

        // TODO: move this and maybe GLFW_MAXIMIZED to Data
        int windowWidth = 1280;
        int windowHeight = 800;

        // Create window with graphics context:
        GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Shadow-free, Dynamic, Derformance Projection", nullptr, nullptr);
        if (!window) {
            // Return nullptr instead of continuing to work with it:
            return nullptr;
        }

        glfwSetKeyCallback(window, key_callback);

        // Center on main screen:
        glfwSetWindowPos(window,
            (primaryMode->width - windowWidth) / 2,
            (primaryMode->height - windowHeight) / 2
        );

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        // Initialize GLAD functions:
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return nullptr;
        }

        // Setup Dear ImGui context:
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi Viewports

        // Setup Dear ImGui style:
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends:
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Load Roboto Font:
        io.Fonts->AddFontFromFileTTF(CMAKE_SOURCE_DIR "/lib/imgui-docking-1.91.4/misc/fonts/Roboto-Medium.ttf", 16.0f);

        // Enable depth test:
        glEnable(GL_DEPTH_TEST);

        // Ensure that both sides of the triangle are rendered:
        glDisable(GL_CULL_FACE);

        return window;
    }

    /**
     * Renders the content for all projector windows.
     */
    static void renderProjectorWindows() {
        int i = 1;
        for (const auto& [projector, monitor] : Data::instance.projectorMonitorMap) {
            // No monitor selected for the projector:
            if (!monitor) {
                continue;
            }

            // Get the global window position from monitor:
            int posX, posY;
            glfwGetMonitorWorkarea(monitor, &posX, &posY, nullptr, nullptr);

            // Get the monitor's video mode for fullscreen:
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            ImVec2 windowSize = ImVec2(static_cast<float>(mode->width), static_cast<float>(mode->height));

            ImGui::SetNextWindowPos(ImVec2(posX, posY));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
            ImGui::SetNextWindowSize(windowSize);
            std::string monitorNameWithIndex = std::to_string(i++) + ": " + std::string(glfwGetMonitorName(monitor));
            ImGui::Begin(monitorNameWithIndex.c_str(), nullptr,
                ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);

            // Display the image:
            GLuint textureID = projector->getTexture().texture;
            ImGui::Image(textureID, windowSize, ImVec2(0, 1), ImVec2(1, 0));

            ImGui::End();

            ImGui::PopStyleVar(2);

            i++;
        }
    }
};
