//
//  Reduce.cpp
//  Vortex2D
//

#include "Reduce.h"

namespace Vortex2D { namespace Fluid {

namespace
{

const char * SumFrag = GLSL(
   out vec4 colour_out;

   uniform sampler2D u_texture;

   void main()
   {
       ivec2 pos = 2 * ivec2(gl_FragCoord.xy - 0.5);

       float x = 0.0;
       x += texelFetch(u_texture, pos + ivec2(0,0), 0).x;
       x += texelFetch(u_texture, pos + ivec2(1,0), 0).x;
       x += texelFetch(u_texture, pos + ivec2(0,1), 0).x;
       x += texelFetch(u_texture, pos + ivec2(1,1), 0).x;

       colour_out = vec4(x, 0.0, 0.0, 0.0);
   }
);

const char * MaxFrag = GLSL(
  out vec4 colour_out;

  uniform sampler2D u_texture;

  void main()
  {
      ivec2 pos = 2 * ivec2(gl_FragCoord.xy - 0.5);

      vec4 value;
      value.x = abs(texelFetch(u_texture, pos + ivec2(0,0), 0).x);
      value.y = abs(texelFetch(u_texture, pos + ivec2(1,0), 0).x);
      value.z = abs(texelFetch(u_texture, pos + ivec2(0,1), 0).x);
      value.w = abs(texelFetch(u_texture, pos + ivec2(1,1), 0).x);

      colour_out = vec4(max(max(value.x, value.y), max(value.z, value.w)), 0.0, 0.0, 0.0);
  }
);

const char * MultiplyFrag = GLSL(
   in vec2 v_texCoord;
   out vec4 colour_out;

   uniform sampler2D u_x;
   uniform sampler2D u_y;

   void main()
   {
       float x = texture(u_x, v_texCoord).x;
       float y = texture(u_y, v_texCoord).x;

       colour_out = vec4(x * y, 0.0, 0.0, 0.0);
   }
);

}

Reduce::Reduce(glm::vec2 size, const char* fragment)
    : reduce(Renderer::Shader::PositionVert, fragment)
{
    while(size.x > 1.0f && size.y > 1.0f)
    {
        size = glm::ceil(size/glm::vec2(2.0f));
        s.emplace_back(size, 1);
        s.back().ClampToBorder();
    }

    reduce.Use().Set("u_texture", 0).Unuse();
}

Renderer::OperatorContext Reduce::operator()(Renderer::Buffer& buffer)
{
    s[0] = reduce(buffer);

    for (std::size_t i = 1; i < s.size(); i++)
    {
        s[i] = reduce(s[i-1]);
    }

    return reduce(s.back());
}

ReduceSum::ReduceSum(const glm::vec2& size)
    : Reduce(size, SumFrag)
    , mReduce(size, 1)
    , mMultiply(Renderer::Shader::TexturePositionVert, MultiplyFrag)
{
    mMultiply.Use().Set("u_x", 0).Set("u_y", 1).Unuse();
    mReduce.ClampToBorder();
}

Renderer::OperatorContext ReduceSum::operator()(Renderer::Buffer& input1, Renderer::Buffer& input2)
{
    mReduce = mMultiply(input1, input2);
    return Reduce::operator()(mReduce);
}

ReduceMax::ReduceMax(const glm::vec2& size)
    : Reduce(size, MaxFrag)
{

}

}}
