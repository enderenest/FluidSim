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

void ComputeShader::dispatch(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ) const 
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

void ComputeShader::checkCompileErrors(GLuint object, const std::string& type, const char* filename = nullptr) const
{
	GLint success;
	glGetShaderiv(object,
		type[0] == 'P' ? GL_LINK_STATUS : GL_COMPILE_STATUS,
		&success);
	if (!success) {
		char infoLog[1024];
		if (type[0] == 'P') glGetProgramInfoLog(object, 1024, NULL, infoLog);
		else              glGetShaderInfoLog(object, 1024, NULL, infoLog);

		std::cerr
			<< "ERROR::" << type
			<< (filename ? std::string(" [") + filename + "]" : std::string())
			<< "\n" << infoLog
			<< "\n -- --------------------------------------------------- --\n";
	}
}

