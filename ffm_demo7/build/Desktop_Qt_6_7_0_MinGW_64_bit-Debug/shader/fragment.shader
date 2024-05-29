#version 330 core
//纹理坐标
in vec2 TexCoord;
//片段颜色
out vec4 FragColor;
//2D 纹理
//uniform sampler2D texture1;
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;


//将纹理采样
void main() 
{
    vec3 yuv;
    vec3 rgb;

    //YUV->RGB
    yuv.x=texture2D(tex_y,TexCoord).r;
    yuv.y=texture2D(tex_u,TexCoord).r-0.5;
    yuv.z=texture2D(tex_v,TexCoord).r-0.5;

    rgb = mat3(1.0, 1.0, 1.0,
        0.0, -0.39465, 2.03211,
        1.13983, -0.58060, 0.0) * yuv;

    FragColor =vec4(rgb,1.0);
}
