// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/simulation/scene/SceneComponent.h"

/**
 * A composite of the scene which consists of multiple components.
 *
 * This implements the 'composite' of the composite design pattern.
 */

class SceneComposite : public SceneComponent, public std::enable_shared_from_this<SceneComposite>
{
public:
    /**
     * Calls the prepare method of all components and giving the
     * concatenated transformation matrix.
     */
    void prepare(SceneData& data, const Mat4f parentModel) override {
        for(const std::shared_ptr<SceneComponent>& comp : components){
            comp->prepare(data, parentModel * transformation);
        }
    }

    /**
     * Rendering all components by calling their render method and
     * giving the concatenated transformation matrix.
     */
    void render(SceneData& data, const Mat4f parentModel) override {
        for(const std::shared_ptr<SceneComponent>& comp : components){
            comp->render(data, parentModel * transformation);
        }
    }

    /**
     * Adds a component to this composite.
     */
    virtual void add(const std::shared_ptr<SceneComponent>& component){
        components.push_back(component);
        //component->setParent(shared_from_this());
    }

    /**
     * Removes the given component from this composite.
     */
    virtual void remove(const std::shared_ptr<SceneComponent>& component){
        const auto result = std::find(components.begin(), components.end(), component);
        if(result != components.end()){
            components.erase(result);
            //component->setParent(nullptr);
        }
    }

    /**
     * Removes all components from this Composite
     */
    virtual void clear(){
        //for(std::shared_ptr<SceneComponent> comp : components){
            //comp->setParent(nullptr);
        //}
        components.clear();
    }

    /**
     * Returns a copy of shared pointers to all children.
     */
    virtual std::vector<std::shared_ptr<SceneComponent>> children() {
        return components;
    }

private:
    std::vector<std::shared_ptr<SceneComponent>> components;
};
