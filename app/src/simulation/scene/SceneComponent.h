// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/simulation/scene/SceneData.h"

/** Forward declaration of scene component class */
class SceneComposite;

/**
 * A component of the scene (abstract class).
 *
 * This implements the 'component' of the composite design pattern.
 */
class SceneComponent
{
public:
    /**
     * Prepares the object before for each frame (before the render method
     * is called).
     */
    virtual void prepare(SceneData& sceneData, Mat4f parentModel){};

    /**
     * Is called every time this component should be rendered.
     */
    virtual void render(SceneData& sceneData, Mat4f parentModel) = 0;

    /** Returns the type name of this class */
    virtual std::string type_name() { return typeid(*this).name(); }

    /** Returns the position vector */
    virtual Vec4f getPosition(){ return transformation.getPosition(); };

    /** Transformation Matrix of this component */
    Mat4f transformation;
};
