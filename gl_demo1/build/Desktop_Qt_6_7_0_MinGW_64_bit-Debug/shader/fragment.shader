#version 330 core
//纹理坐标
in vec2 TexCoord;
//片段颜色
out vec4 FragColor;
//2D 纹理
uniform sampler2D texture1;


//将纹理采样
void main() 
{
  FragColor =texture(texture1, TexCoord);
}
