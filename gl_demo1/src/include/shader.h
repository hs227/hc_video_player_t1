#ifndef SHADER_H
#define SHADER_H

#include"glad.h"
#include<GLM/glm.hpp>
#include<glm/gtc/type_ptr.hpp>

#include<fstream>
#include<iostream>
#include<sstream>

class Shader
{
public:
    unsigned int ID;
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        std::string   realVertexPath = std::string(R"(shader/)").append(vertexPath);
        std::string   realFragmentPath = std::string(R"(shader/)").append(fragmentPath);
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::string   vShaderSource;
        std::string   fShaderSource;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            std::stringstream vFileStream, fFileStream;
            vShaderFile.open(realVertexPath);
            fShaderFile.open(realFragmentPath);
            vFileStream << vShaderFile.rdbuf();
            fFileStream << fShaderFile.rdbuf();
            vShaderFile.close();
            fShaderFile.close();
            vShaderSource = vFileStream.str();
            fShaderSource = fFileStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }

        const char* vShaderCode = vShaderSource.c_str();
        const char* fShaderCode = fShaderSource.c_str();

        std::cout << vShaderCode << std::endl;
        std::cout << "--------------------------" << std::endl;
        std::cout << fShaderCode << std::endl;

        unsigned int vertexShader, fragmentShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vShaderCode, NULL);
        glCompileShader(vertexShader);
        checkCompileErrors(vertexShader, "VERTEX");
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
        glCompileShader(fragmentShader);
        checkCompileErrors(fragmentShader, "FRAGMENT");
        this->ID = glCreateProgram();
        glAttachShader(this->ID, vertexShader);
        glAttachShader(this->ID, fragmentShader);
        glLinkProgram(this->ID);
        checkCompileErrors(this->ID, "PROGRAM");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void use(void)
    {
        glUseProgram(this->ID);
    }

    void SetBool(const char* name, bool value) const
    {
        glUniform1i(glGetUniformLocation(this->ID, name), value);
    }

    void SetInt(const char* name, int value) const
    {
        glUniform1i(glGetUniformLocation(this->ID, name), value);
    }

    void SetFloat(const char* name, float value) const
    {
        glUniform1f(glGetUniformLocation(this->ID, name), value);
    }

    void SetVec2(const char* name, glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(this->ID, name),1,&value[0]);
    }

    void SetVec2(const char* name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(this->ID, name),x,y);
    }

    void SetVec3(const char* name, glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(this->ID, name), 1, &value[0]);
    }

    void SetVec3(const char* name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(this->ID, name), x, y, z);
    }

    void SetVec4(const char* name, glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(this->ID, name),1, &value[0]);
    }

    void SetVec4(const char* name, float v0, float v1, float v2, float v3) const
    {
        glUniform4f(glGetUniformLocation(this->ID, name), v0, v1, v2, v3);
    }

    void SetMat2(const char* name, glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(this->ID, name), 1, GL_FALSE, &mat[0][0]);
        //glUniformMatrix2fv(glGetUniformLocation(this->ID, name), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void SetMat3(const char* name, glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(this->ID, name), 1, GL_FALSE, &mat[0][0]);
        //glUniformMatrix3fv(glGetUniformLocation(this->ID, name), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void SetMat4(const char* name, glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(this->ID, name), 1, GL_FALSE, &mat[0][0]);
        //glUniformMatrix4fv(glGetUniformLocation(this->ID, name), 1, GL_FALSE, glm::value_ptr(mat));
    }


private:
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};


#endif // SHADER_H
