#include "Common.h"
#include "ResourcePath.h"
#include "GridVisualiser.h"
#include "WindowRenderer.h"
#include "Engine.h"

#include <string>

void error_callback(int error, const char* description)
{
    std::cout << "GLFW erro " << error << " : " << description << std::endl;
}

int main(int argc, const char * argv[])
{
    if(!glfwInit()) { std::cout << "Could not initialize GLFW" << std::endl; exit(EXIT_FAILURE); }

    glfwSetErrorCallback(error_callback);

    int size = 30;
    int scale = 30;

    std::cout << "My resource path is " << getResourcePath() << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GridVisualiser grid({size, size}, scale);

    auto realSize = glm::vec2{size*scale,size*scale};
    WindowRenderer window(realSize, &grid);

    Fluid::Dimensions dimensions(realSize, scale);
    Renderer::Quad quad(dimensions.Size);
    Fluid::Boundaries boundaries(dimensions, 2);

    Renderer::Rectangle rect({4.0f, 4.0f});
    boundaries.Render({&rect});
    boundaries.RenderWeights(quad);

    while (!window.ShouldClose() && !grid.ShouldClose())
    {
        window.Render();
        grid.Render();

        glfwPollEvents();
    }

    glfwTerminate();
}
