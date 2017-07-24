//
//  LevelSetSet.h
//  Vortex2D
//

#include "VariationalHelpers.h"

#include <Vortex2D/Renderer/Shapes.h>
#include <Vortex2D/Engine/LevelSet.h>

using namespace Vortex2D::Renderer;
using namespace Vortex2D::Fluid;

extern Device* device;

/*
void PrintLevelSet(int size, float (*phi)(const Vec2f&))
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            Vec2f pos(i,j);
            pos += Vec2f(1.0f);
            pos /= (float)size;
            std::cout << "(" << size * phi(pos) << ")";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void PrintLevelSet(Array2f& array)
{
    for (int j = 0; j < array.nj; j++)
    {
        for (int i = 0; i < array.ni; i++)
        {
            std::cout << "(" << array(i, j) << ")";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void PrintLevelSet(Buffer& buffer)
{
    Reader reader(buffer);
    reader.Read();

    for (int j = 0; j < buffer.Height() / 2; j++)
    {
        for (int i = 0; i < buffer.Width() / 2; i++)
        {
            float value = 0.0f;
            value += reader.GetFloat(i * 2, j * 2);
            value += reader.GetFloat(i * 2 + 1, j * 2);
            value += reader.GetFloat(i * 2, j * 2 + 1);
            value += reader.GetFloat(i * 2 + 1, j * 2 + 1);
            value *= 0.25f * 0.5f;

            std::cout << "(" << value << ")";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
*/

void CheckDifference(Texture& texture, float (*phi)(const Vec2f&))
{
    std::vector<float> pixels(texture.GetWidth() * texture.GetHeight());
    texture.CopyTo(pixels.data(), 4);

    for (uint32_t j = 0; j < texture.GetHeight() / 2; j++)
    {
        for (uint32_t i = 0; i < texture.GetWidth() / 2; i++)
        {
            Vec2f pos(i,j);
            pos += Vec2f(1.0f);
            pos /= (float)texture.GetWidth();
            float value = texture.GetWidth() * phi(pos);

            float readerValue = 0.0f;
            readerValue += pixels[i * 2 + (j * 2) * texture.GetWidth()];
            readerValue += pixels[i * 2 + 1 + (j * 2) * texture.GetWidth()];
            readerValue += pixels[i * 2 + (j * 2 + 1) * texture.GetWidth()];
            readerValue += pixels[i * 2 + 1 + (j * 2 + 1) * texture.GetWidth()];
            readerValue *= 0.25f * 0.5f;

            float diff = std::abs(value - readerValue);

            EXPECT_LT(diff, 0.8f); // almost sqrt(0.5)
        }
    }
}

TEST(DISABLED_LevelSetTests, SimpleCircle)
{
    glm::vec2 size(20.0f);
    glm::vec2 doubleSize(glm::vec2(2.0f) * size);

    LevelSet levelSet(*device, doubleSize);

    Ellipse circle(*device, glm::vec2{rad0} * doubleSize, glm::vec4(0.5f));
    circle.Position = glm::vec2(c0[0], c0[1]) * doubleSize;

    circle.Initialize({levelSet});
    circle.Update(levelSet.Orth, glm::mat4());

    levelSet.Record([&](vk::CommandBuffer commandBuffer)
    {
        Clear(20, 20, glm::vec4(-0.5f)).Draw(commandBuffer);
        circle.Draw(commandBuffer, {levelSet});
    });

    levelSet.Redistance(500);

    CheckDifference(levelSet, boundary_phi);
}

/*
TEST(LevelSetTests, ComplexCircles)
{
    glm::vec2 size(50.0f);
    glm::vec2 doubleSize(glm::vec2(2.0f) * size);

    LevelSet levelSet(doubleSize);

    Ellipse circle0(glm::vec2{rad0} * doubleSize);
    Ellipse circle1(glm::vec2{rad1} * doubleSize);
    Ellipse circle2(glm::vec2{rad2} * doubleSize);
    Ellipse circle3(glm::vec2{rad3} * doubleSize);

    circle0.Colour = glm::vec4(1.0);
    circle1.Colour = circle2.Colour = circle3.Colour = glm::vec4(-1.0f);

    circle0.Position = glm::vec2(c0[0], c0[1]) * doubleSize;
    circle1.Position = glm::vec2(c1[0], c1[1]) * doubleSize;
    circle2.Position = glm::vec2(c2[0], c2[1]) * doubleSize;
    circle3.Position = glm::vec2(c3[0], c3[1]) * doubleSize;

    levelSet.Clear(glm::vec4(-1.0));
    levelSet.Render(circle0);
    levelSet.Render(circle1);
    levelSet.Render(circle2);
    levelSet.Render(circle3);
    levelSet.Redistance(500);

    CheckDifference(size.x, levelSet, complex_boundary_phi);
}

TEST(LevelSetTests, Extrapolate)
{
    glm::vec2 size(50);

    Buffer solidPhi(glm::vec2(2)*size, 1);

    std::vector<float> solidData(4 * size.x * size.y, 1.0);
    DrawSquare(100, 100, solidData, glm::vec2(10.0f), glm::vec2(20.0f), -1.0f);
    Writer(solidPhi).Write(solidData);

    LevelSet liquidPhi(size);

    std::vector<float> liquidData(4 * size.x * size.y, 0.0);
    Writer(liquidPhi).Write(liquidData);

    liquidPhi.Extrapolate(solidPhi);

    Reader reader(liquidPhi);
    reader.Read();

    for (int i = 0; i < 9; i++)
    {
        EXPECT_FLOAT_EQ(-1.0f, reader.GetFloat(i + 5, 5));
        EXPECT_FLOAT_EQ(-1.0f, reader.GetFloat(5, i + 5));
        EXPECT_FLOAT_EQ(-1.0f, reader.GetFloat(i + 5, 10));
        EXPECT_FLOAT_EQ(-1.0f, reader.GetFloat(10, i + 5));
    }
}
*/