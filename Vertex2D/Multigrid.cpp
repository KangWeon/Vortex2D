//
//  Multigrid.cpp
//  Vertex2D
//
//  Created by Maximilian Maldacker on 10/09/2015.
//  Copyright (c) 2015 Maximilian Maldacker. All rights reserved.
//

#include "Multigrid.h"
#include "Disable.h"

namespace Fluid
{

Multigrid::Multigrid(const glm::vec2 & s, int iterations)
    : mDepths(0)
    , mCorrect("TexturePosition.vsh", "Correct.fsh")
    , mProlongate("TexturePosition.vsh", "Prolongate.fsh")
    , mResidual("Diff.vsh", "Residual.fsh")
    , mRestrict("TexturePosition.vsh", "Restrict.fsh")
{
    auto size = s;
    const float min_size = 4.0f;

    do
    {
        mXs.emplace_back(size, iterations, 1.0f);
        size = glm::ceil(size/glm::vec2(2.0f));
        mDepths++;
    }while(size.x > min_size && size.y > min_size);

    mCorrect.Use()
    .Set("u_texture", 0)
    .Set("u_residual", 1)
    .Unuse();

    mProlongate.Use()
    .Set("u_texture", 0)
    .Set("u_pressure", 1)
    .Unuse();

    mResidual.Use()
    .Set("u_texture", 0)
    .Set("u_weights", 1)
    .Unuse();

    mRestrict.Use()
    .Set("u_texture", 0)
    .Unuse();
}

Renderer::Reader Multigrid::GetPressureReader(int depth)
{
    return {mXs[depth].GetData().Pressure.Front};
}

Renderer::Reader Multigrid::GetWeightsReader(int depth)
{
    return {mXs[depth].GetData().Weights};
}

void Multigrid::Init(Boundaries & boundaries)
{
    for(int i = 0; i < mDepths ; i++)
    {
        mXs[i].Init(boundaries);
    }
}

LinearSolver::Data & Multigrid::GetData()
{
    return mXs[0].GetData();
}

void Multigrid::Solve()
{
    for(int i = 0 ; i < mDepths - 1 ; i++)
    {
        GaussSeidel(i, true);
        Residual(i);
        Restrict(i);
    }
    GaussSeidel(mDepths - 1, true);
    for(int i = mDepths - 2 ; i >= 0 ; --i)
    {
        Prolongate(i);
        Correct(i);
        GaussSeidel(i, false);
    }
}

void Multigrid::GaussSeidel(int depth, bool up)
{
    mXs[depth].Solve(up);
}

void Multigrid::Residual(int depth)
{
    auto & x = mXs[depth].GetData();

    Renderer::Enable e(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    x.Pressure.swap();
    mResidual.Use().Set("h", x.Quad.Size());
    mResidual.apply(x.Quad, x.Pressure,
                    x.Pressure.Back, x.Weights);
}

void Multigrid::Restrict(int depth)
{
    auto & x = mXs[depth].GetData();
    auto & r = mXs[depth+1].GetData();

    Renderer::Enable e(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    mRestrict.apply(r.Quad, r.Pressure,
                    x.Pressure.Front);
}

void Multigrid::Prolongate(int depth)
{
    auto & x = mXs[depth].GetData();
    auto & u = mXs[depth+1].GetData();

    Renderer::Enable e(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    mProlongate.apply(x.Quad, x.Pressure,
                      u.Pressure.Front,
                      x.Pressure.Back);
}

void Multigrid::Correct(int depth)
{
    auto & x = mXs[depth].GetData();

    Renderer::Enable e(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    x.Pressure.swap();
    mCorrect.apply(x.Quad, x.Pressure,
                   x.Pressure.Back);
}

}