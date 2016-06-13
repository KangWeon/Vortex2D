#include <GLFW/glfw3.h>

#include "Common.h"

#include "SmokeExample.h"
#include "ObstacleSmokeExample.h"
#include "WaterExample.h"
#include "VelocitySmokeExample.h"

#include <iostream>

std::unique_ptr<BaseExample> example;

void error_callback(int error, const char* description)
{
    std::cout << "GLFW erro " << error << " : " << description << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action != GLFW_PRESS) return;

    switch(key)
    {
        case GLFW_KEY_1:
            example.reset(new SmokeExample());
            break;
        case GLFW_KEY_2:
            example.reset(new ObstacleSmokeExample());
            break;
        case GLFW_KEY_3:
            example.reset(new VelocitySmokeExample());
            break;
        case GLFW_KEY_4:
            example.reset(new WaterExample());
            break;
        default:
            break;
    }

}

int main(int argc, const char * argv[])
{
    if(!glfwInit()) { std::cout << "Could not initialize GLFW" << std::endl; exit(EXIT_FAILURE); }

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glm::vec2 size = {500,500};

    GLFWwindow * window = glfwCreateWindow(size.x, size.y, "Vortex2D Examples", NULL, NULL);

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    glViewport(0, 0, size.x, size.y);
    glm::mat4 ortho = glm::ortho(0.0f, size.x, size.y, 0.0f);

    glfwSwapInterval(1);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto colour = glm::vec4{99.0f,96.0f,93.0f,255.0f}/glm::vec4(255.0f);
    glClearColor(colour.r, colour.g, colour.b, colour.a);

    Renderer::Disable d(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    example.reset(new SmokeExample());

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        example->frame();

        Renderer::Enable d(GL_BLEND);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        for(auto object : example->render())
        {
            object->Render(ortho);
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}