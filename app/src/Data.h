#pragma once

#include <map>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

#include "src/math/Vec4.h"
#include "src/math/Mat4.h"

#include "src/processing/devices/RGBDCameraManager.h"

// Forward declaration to avoid including the full GLFW header:
struct GLFWmonitor;

/* Forward declaration to avoid cycle includes */
class Projector;
class VirtualRGBDCamera;
class Camera;

/**
 * Our global application data.
 *
 * Data can be obtained via `Data::instance.NAME` everywhere.
 */
struct Data {
    /** Camera Manager, contains current point clouds and associated cameras */
    RGBDCameraManager cameraManager;

    /** Projectors */
    std::vector<std::shared_ptr<Projector>> projectors;

    /** Number of Projectors that should be calibrated */
    int projectorNumToCalibrate = 2;

    /** Calibration Passes */
    int calibrationPasses = 1;

    /** Is Calibration mode activated? */
    bool isCalibrating = false;

    std::string calibrationStateInfo = "";

    /** Projector Min Brightness */
    float projectorMinBrightness = 0.02f;

    /** Projector Brightness */
    float projectorIntensity = 5.f;

    /** Map of virtual projector to a monitor (real projector) */
    std::map<std::shared_ptr<Projector>, GLFWmonitor*> projectorMonitorMap;

    /** Just for loading and saving the config file */
    std::map<int, int> projectorIdMonitorIdMap;

    /** Selected projector for image view */
    int selectedProjector = -1;

    bool colorDistanceToggle = false;

    /** RGBD cameras */
    std::vector<std::shared_ptr<VirtualRGBDCamera>> rgbdCameras;

    /** Virtual camera */
    std::shared_ptr<Camera> camera;

    /** Measures Frame Timing */
    float timingFrame = 0.0f;

    /** Width of Viewport (3D Render) */
    int display_w = 0;

    /** Height of Viewport (3D Render) */
    int display_h = 0;

    /** Menu Bar width */
    int menuWidth = 320;

    /** Exposure */
    float exposure = 0.5f;

    /** Saturation (post processing) */
    float saturation = 2.8f;

    /** Contrast (post processing) */
    float contrast = 1.7f;

    /** Gamma (post processing) */
    float gamma = 2.4f;

    /** Simulate RGBD noise */
    bool simulateRGBDNoise = false;

    /** UI selection of camera */
    int selectedRGBDCamera = -1;

    Mat4f virtualDisplayTransform;
    Vec4f virtualDisplaySpectator = Vec4f(0.4f, 2.0f, 0.0f);
    float virtualDisplayHeight = 1.1f;

    /** Should rectification target follow camera? */
    bool rectifyFollowCam = false;

    /** How much the target pos should follow around y axis rotation */
    float rectifyYRotationFactor = 1.f;

    /** */
    bool projectOnlyProjectorIDColor = false;

    /** Use tile based shadow avoidance */
    bool tileBasedShadowAvoidance = false;

    /** Ignores shadow avoidance (despite it's calculated) */
    bool ignoreShadowAvoidance = false;

    /** Render raw point cloud */
    bool renderRawPointCloud = false;

    /** Precision of rectification */
    int rectificationMeshStride = 1;

    /** Projector Image Resolution */
    int projectorImageWidth = 3840;
    int projectorImageHeight = 2160;

    /** Runtime */
    float runtime = 0;

    /** Should the projection plane be shown? */
    bool showProjectionPlane = true;

    /** Should the projection plane be enabled? */
    bool isProjectionPlaneInScene = true;

    /** Bump Intensity */
    float projectionPlaneBumpIntensity = 0.0;

    /** Simulate folds */
    bool projectionPlaneSimulateFolds = false;

    /** Simulate wind */
    bool projectionPlaneSimulateWind = false;

    /** Operating Table Position Offset */
    Vec4f projectionPlanePositionOffset;

private:
    Data() {}

public:
    static Data instance;
};
