#include <gtest/gtest.h>
#include <Vortex2D/Renderer/Instance.h>
#include <Vortex2D/Renderer/Device.h>

Vortex2D::Renderer::Device* device;

int main(int argc, char **argv)
{
    std::vector<const char*> extensions;
    Vortex2D::Renderer::Instance instance_;
    instance_.Create("Tests", extensions, true);
    Vortex2D::Renderer::Device device_(instance_.GetPhysicalDevice());

    device = &device_;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
