#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include<glad/glad.h>
#include <GLFW/glfw3.h>
#include<string>

class ComputeShader
{
public:
		ComputeShader(const char* computeFile);
		
		~ComputeShader();

		void use() const;

		void dispatch(unsigned int groupsX, unsigned int groupsY = 1, unsigned int groupsZ = 1) const;

		void setInt(const char* name, int value) const;

		void setFloat(const char* name, float value) const;

		void setUint(const char* name, const unsigned int value) const;

		void wait() const;

		unsigned int getID();


private:
		unsigned int _id;

		std::string loadShaderSource(const char* filePath) const;

		void checkCompileErrors(GLuint object, const std::string& type, const char* filename) const;
};

#endif
