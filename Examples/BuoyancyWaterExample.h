//
//  HydrostaticWaterExample.h
//  Vortex2D
//

#include <Vortex2D/Vortex2D.h>

#include "Runner.h"
#include "Rigidbody.h"

#include <functional>
#include <vector>
#include <memory>

extern glm::vec4 red;
extern glm::vec4 blue;

class HydrostaticWaterExample : public Runner
{
    const float gravityForce = 300.0f;

public:
    HydrostaticWaterExample(const Vortex2D::Renderer::Device& device,
                     const Vortex2D::Fluid::Dimensions& dimensions,
                     float dt)
        : delta(dt)
        , gravity(device, glm::vec2(1024.0f, 1024.0f))
        , world(device, dimensions, dt)
        , solidPhi(world.SolidDistanceField())
        , liquidPhi(world.LiquidDistanceField())
        , rWorld(b2Vec2(0.0f, gravityForce / box2dScale))
        , circle1(device, rWorld, b2_dynamicBody, world, Vortex2D::Fluid::RigidBody::Type::eStrong, 40.0f, 0.5f)
        , circle2(device, rWorld, b2_dynamicBody, world, Vortex2D::Fluid::RigidBody::Type::eStrong, 40.0f, 0.9f)
        , circle3(device, rWorld, b2_dynamicBody, world, Vortex2D::Fluid::RigidBody::Type::eStrong, 40.0f, 2.0f)
        , left(device, rWorld, b2_staticBody, world, Vortex2D::Fluid::RigidBody::Type::eStatic, {20.0f, 500.0f})
        , right(device, rWorld, b2_staticBody, world, Vortex2D::Fluid::RigidBody::Type::eStatic, {20.0f, 500.0f})
        , bottom(device, rWorld, b2_staticBody, world, Vortex2D::Fluid::RigidBody::Type::eStatic, {1000.0f, 20.0f})
    {
        liquidPhi.Scale = solidPhi.Scale = glm::vec2(dimensions.Scale);
        gravity.Colour = glm::vec4(0.0f, dt * gravityForce, 0.0f, 0.0f);

        solidPhi.Colour = red;
        liquidPhi.Colour = blue;
    }

    void Init(const Vortex2D::Renderer::Device& device,
              Vortex2D::Renderer::RenderTarget& renderTarget) override
    {
        // Add particles
        Vortex2D::Renderer::IntRectangle fluid(device, {900.0f, 300.0f});
        fluid.Position = {40.0f, 600.0f};
        fluid.Colour = glm::vec4(4);

        world.RecordParticleCount({fluid}).Submit().Wait();

        // Draw boundaries
        left.SetTransform({12.0f, 1000.0f}, 0.0f);
        left.Update();

        right.SetTransform({1000.0f, 1000.0f}, 0.0f);
        right.Update();

        bottom.SetTransform({12.0f, 1000.0f}, 0.0f);
        bottom.Update();

        // Add circles
        circle1.SetTransform({200.0f, 200.0f}, 0.0f);
        circle2.SetTransform({500.0f, 200.0f}, 0.0f);
        circle3.SetTransform({800.0f, 200.0f}, 0.0f);

        // Set gravity
        velocityRender = world.RecordVelocity({gravity});

        Vortex2D::Renderer::BlendState blendState;
        blendState.ColorBlend
                .setBlendEnable(true)
                .setAlphaBlendOp(vk::BlendOp::eAdd)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero);

        windowRender = renderTarget.Record({liquidPhi, solidPhi}, blendState);
    }

    void Step() override
    {
        circle1.Update();
        circle2.Update();
        circle3.Update();

        world.SubmitVelocity(velocityRender);
        world.Solve();

        const int velocityStep = 8;
        const int positionStep = 3;
        rWorld.Step(delta, velocityStep, positionStep);

        windowRender.Submit();
    }
private:
    float delta;
    Vortex2D::Renderer::Rectangle gravity;
    Vortex2D::Fluid::WaterWorld world;
    Vortex2D::Fluid::DistanceField solidPhi, liquidPhi;
    Vortex2D::Renderer::RenderCommand velocityRender, windowRender;

    b2World rWorld;

    CircleRigidbody circle1, circle2, circle3;
    BoxRigidbody left, right, bottom;
};
