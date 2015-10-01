//
//  Boundaries.cpp
//  Vertex2D
//
//  Created by Maximilian Maldacker on 01/09/2015.
//  Copyright (c) 2015 Maximilian Maldacker. All rights reserved.
//

#include "Boundaries.h"
#include "Disable.h"

namespace Fluid
{

Boundaries::Boundaries(Dimensions dimensions, int antialias)
    : mDimensions(dimensions)
    , mAntialias(antialias)
    , mBoundaries(glm::vec2(antialias)*dimensions.Size, 1)
    , mBoundariesVelocity(dimensions.Size, 2)
    , mWeights("TexturePosition.vsh", "Weights.fsh")
    , mHorizontal({dimensions.Size.x, 1.0f})
    , mVertical({1.0f, dimensions.Size.y})
{
    mBoundaries.clear();
    mBoundaries.linear();
    mBoundariesVelocity.clear();

    mVertical.Colour = {1.0f,1.0f,1.0f,1.0f};
    mHorizontal.Colour = {1.0f,1.0f,1.0f,1.0f};

    mWeights.Use().Set("u_texture", 0).Unuse();
}

void Boundaries::Render(const std::vector<Renderer::Drawable*> & objects)
{
    mObjects = objects;
    mBoundaries.begin({0.0f, 0.0f, 0.0f, 0.0f});
    Render(mBoundaries.Orth, mAntialias, mAntialias);
    mBoundaries.end();
}

void Boundaries::RenderMask(Buffer & mask)
{
    Renderer::Enable e(GL_STENCIL_TEST);
    Renderer::DisableColorMask c;

    glStencilFunc(GL_ALWAYS, 1, 0xFF); // write 1 in stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // replace value with above
    glStencilMask(0xFF); // enable stencil writing

    mask.begin();

    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT); // clear stencil buffer

    float scale = mask.size().x / mDimensions.Size.x;
    Render(mask.Orth, 1, scale);
    mask.end();

    glStencilMask(0x00); // disable stencil writing
}

void Boundaries::Render(const glm::mat4 & orth, int thickness, float scale)
{
    mHorizontal.Position = {0.0f, 0.0f};
    mHorizontal.Scale = {scale, thickness};
    mHorizontal.Render(orth);

    mHorizontal.Position = {0.0f, scale*mDimensions.Size.y-thickness};
    mHorizontal.Render(orth);

    mVertical.Position = {0.0f, 0.0f};
    mVertical.Scale = {thickness, scale};
    mVertical.Render(orth);

    mVertical.Position = {scale*mDimensions.Size.x-thickness, 0.0f};
    mVertical.Render(orth);

    auto scaled = glm::scale(orth, glm::vec3(scale, scale, 1.0f));
    for(auto object : mObjects)
    {
        object->Render(scaled*mDimensions.InvScale);
    }
}

void Boundaries::RenderVelocities(const std::vector<Renderer::Drawable*> & objects)
{
    mBoundariesVelocity.begin({0.0f, 0.0f, 0.0f, 0.0f});
    for(auto object : objects)
    {
        object->Render(mBoundariesVelocity.Orth*mDimensions.InvScale);
    }
    mBoundariesVelocity.end();
}

void Boundaries::Clear()
{
    mBoundaries.clear();
    mBoundariesVelocity.clear();
    mObjects.clear();
}

}