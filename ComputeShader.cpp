#include "ComputeShader.h"
#include <fstream>
#include <sstream>
#include <iostream>

ComputeShader::ComputeShader(const char* computePath) 
{
	std::string code = loadShaderSource(computePath);
	const char* src = code.c_str();

	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	checkCompileErrors(shader, "COMPUTE");

	ID = glCreateProgram();
	glAttachShader(ID, shader);
	glLinkProgram(ID);
	checkCompileErrors(ID, "PROGRAM");

	glDeleteShader(shader);
}

ComputeShader::~ComputeShader() 
{
	glDeleteProgram(ID);
}

void ComputeShader::use() const 
{
	glUseProgram(ID);
}

void ComputeShader::dispatch(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ) const 
{
	glDispatchCompute(groupsX, groupsY, groupsZ);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ComputeShader::setInt(const char* name, int value) const 
{
	glUniform1i(glGetUniformLocation(ID, name), value);
}

void ComputeShader::wait() const 
{
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
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

void ComputeShader::checkCompileErrors(GLuint object, const std::string& type) const 
{
	GLint success;
	GLchar infoLog[1024];

	if (type == "COMPUTE") {
		glGetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(object, 1024, nullptr, infoLog);
			std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: "
				<< type << "\n"
				<< infoLog << "\n";
		}
	}
	else {  // PROGRAM
		glGetProgramiv(object, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(object, 1024, nullptr, infoLog);
			std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: "
				<< type << "\n"
				<< infoLog << "\n";
		}
	}
}

