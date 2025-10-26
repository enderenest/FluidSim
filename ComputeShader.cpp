#include "ComputeShader.h"
#include <fstream>
#include <sstream>
#include <iostream>

ComputeShader::ComputeShader(const char* computeFile) 
{
	std::string code = loadShaderSource(computeFile);
	const char* src = code.c_str();

	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	checkCompileErrors(shader, "COMPUTE", computeFile);

	_id = glCreateProgram();
	glAttachShader(_id, shader);
	glLinkProgram(_id);
	checkCompileErrors(_id, "PROGRAM", computeFile);

	glDeleteShader(shader);
}

ComputeShader::~ComputeShader() 
{
	glDeleteProgram(_id);
}

void ComputeShader::use() const 
{
	glUseProgram(_id);
}

void ComputeShader::dispatch(int groupsX, int groupsY, int groupsZ) const 
{
	glDispatchCompute(groupsX, groupsY, groupsZ);
}

void ComputeShader::setInt(const char* name, int value) const 
{
	glUniform1i(glGetUniformLocation(_id, name), value);
}

void ComputeShader::setFloat(const char* name, float value) const
{
	glUniform1f(glGetUniformLocation(_id, name), value);
}

void ComputeShader::setUint(const char* name, const int value) const 
{
	glUniform1ui(glGetUniformLocation(_id, name), value);
}

int ComputeShader::getID() { return _id; }

void ComputeShader::wait() const 
{
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

std::string ComputeShader::loadShaderSource(const char* filePath) const {
	std::ifstream in(filePath);
	if (!in.is_open()) {
		std::cerr << "ERROR: Could not open compute shader file: "
			<< filePath << "\n";
		return "";
	}
	std::stringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

void ComputeShader::checkCompileErrors(GLuint object,
    const std::string& type,
    const char* filename /* no default here in the definition */) const
{
    GLint success = 0;

    if (!type.empty() && (type[0] == 'P' /* e.g., "PROGRAM" */)) {
        // PROGRAM path
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLen = 0;
            glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLen);
            std::string infoLog(logLen, '\0');
            glGetProgramInfoLog(object, logLen, nullptr, infoLog.data());
            std::cerr << "ERROR::" << type
                << (filename ? std::string(" [") + filename + "]" : std::string())
                << "\n" << infoLog
                << "\n -- --------------------------------------------------- --\n";
        }
    }
    else {
        // SHADER path
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint logLen = 0;
            glGetShaderiv(object, GL_INFO_LOG_LENGTH, &logLen);
            std::string infoLog(logLen, '\0');
            glGetShaderInfoLog(object, logLen, nullptr, infoLog.data());
            std::cerr << "ERROR::" << type
                << (filename ? std::string(" [") + filename + "]" : std::string())
                << "\n" << infoLog
                << "\n -- --------------------------------------------------- --\n";
        }
    }
}

