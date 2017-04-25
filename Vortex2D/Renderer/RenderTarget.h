//
//  RenderTarget.h
//  Vortex2D
//

#ifndef RenderTarget_h
#define RenderTarget_h

#include <Vortex2D/Renderer/Common.h>
#include <Vortex2D/Renderer/Drawable.h>

namespace Vortex2D { namespace Renderer {

/**
 * @brief And interface to represent a target that can be rendered to.
 * This is implemented by the RenderWindow (in the examples) and the RenderTexture
 */
struct RenderTarget
{
    RenderTarget(float width, float height);

    virtual ~RenderTarget(){}

    /**
     * @brief Clear clears the whole target with the colour
     */
    virtual void Clear(const glm::vec4 & colour) = 0;

    /**
     * @brief Render the object to the target
     * @param object An object whose class implements Drawable
     * @param transform An optional aditional transformation matrix to be applied before rendering
     */
    virtual void Render(Drawable & object, const glm::mat4 & transform = glm::mat4()) = 0;

    /**
     * @brief Matrix holding an orthographic projection
     */
    glm::mat4 Orth;

    vk::Framebuffer Framebuffer;

    vk::RenderPass RenderPass;

    uint32_t Width, Height;
};

}}

#endif /* RenderTarget_h */
