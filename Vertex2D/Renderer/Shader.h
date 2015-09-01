//
//  Shader.h
//  Vortex
//
//  Created by Maximilian Maldacker on 06/04/2014.
//
//

#ifndef __Vortex__Shader__
#define __Vortex__Shader__

#include <string>
#include "Common.h"
#include "Uniform.h"

namespace Renderer
{

class Program;

class Shader
{
public:
    Shader & Source(const std::string & source);
    Shader & Compile();
    virtual ~Shader();

    friend class Program;

    static const GLuint TexCoords = 1;
    static const GLuint Position = 2;
    static const GLuint Colour = 3;
protected:
    Shader(GLuint shader);
    GLuint mShader;

private:
    static const char * PositionName;
    static const char * TexCoordsName;
    static const char * ColourName;
};

struct VertexShader : Shader
{
    VertexShader();
};

struct FragmentShader : Shader
{
    FragmentShader();
};

template<typename T>
class Uniform
{
public:
    Uniform(Program & program, const std::string & name);
    Uniform(GLuint location = -1);
    Uniform(const Uniform &);
    void SetLocation(Program & program, const std::string & name);
    void Set(T value);

    friend class Program;
private:
    GLuint mLocation;
};

class Program
{
public:
    Program();
    Program(const std::string & vertex, const std::string & fragment);
    ~Program();

    Program(Program &&);
    Program & operator=(Program &&);
    
    Program & AttachShader(const Shader & shader);
    Program & Link();
    Program & Use();
    static void Unuse();

    template<typename T>
    Program & Set(const std::string & name, T value)
    {
        GLint location = glGetUniformLocation(mProgram, name.c_str());
        assert(location >= 0);
        glUniform<T>(location, value);

        return *this;
    }

    Program & SetMVP(const glm::mat4 & mvp);

    static Program & TexturePositionProgram();
    static Program & PositionProgram();
    static Program & ColourTexturePositionProgram();
    static Program & ColourPositionProgram();

    template<typename T>
    friend class Uniform;
private:
    GLuint mProgram;
    Uniform<glm::mat4> mMVP;

    static int CurrentProgram;
};

template<typename T>
Uniform<T>::Uniform(Program & program, const std::string & name)
{
    SetLocation(program, name);
}

template<typename T>
Uniform<T>::Uniform(GLuint location) : mLocation(location)
{
    
}

template<typename T>
Uniform<T>::Uniform(const Uniform & uniform) : mLocation(uniform.mLocation)
{
    
}

template<typename T>
void Uniform<T>::SetLocation(Program & program, const std::string & name)
{
    program.Use();
    mLocation = glGetUniformLocation(program.mProgram, name.c_str());
    assert(mLocation != -1);
    program.Unuse();
}

template<typename T>
void Uniform<T>::Set(T value)
{
    glUniform<T>(mLocation, value);
}

}

#endif /* defined(__Vortex__Shader__) */