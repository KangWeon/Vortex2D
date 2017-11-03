//
//  Jacobi.h
//  Vortex2D
//

#ifndef Vortex2D_Jacobi_h
#define Vortex2D_Jacobi_h

#include <Vortex2D/Engine/LinearSolver/LinearSolver.h>
#include <Vortex2D/Engine/LinearSolver/Preconditioner.h>
#include <Vortex2D/Engine/LinearSolver/Reduce.h>
#include <Vortex2D/Renderer/Work.h>
#include <Vortex2D/Renderer/CommandBuffer.h>
#include <Vortex2D/Renderer/Pipeline.h>

namespace Vortex2D { namespace Fluid {

/**
 * @brief An iterative jacobi linear solver.
 */
class Jacobi : public Preconditioner
{
public:
    Jacobi(const Renderer::Device& device, const glm::ivec2& size);

    void Init(Renderer::GenericBuffer& d,
              Renderer::GenericBuffer& l,
              Renderer::GenericBuffer& b,
              Renderer::GenericBuffer& pressure) override;

    void Record(vk::CommandBuffer commandBuffer) override;

    void Record(vk::CommandBuffer commandBuffer, int iterations);
    void SetW(float w);
    void SetPreconditionerIterations(int iterations);

private:
    float mW;
    int mPreconditionerIterations;

    Renderer::GenericBuffer* mPressure;
    Renderer::Buffer<float> mBackPressure;

    Renderer::Work mJacobi;
    Renderer::Work::Bound mJacobiFrontBound;
    Renderer::Work::Bound mJacobiBackBound;
};

}}

#endif
