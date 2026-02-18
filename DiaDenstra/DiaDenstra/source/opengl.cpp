// Template, IGAD version 3
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2023


#include "opengl.h"

typedef unsigned int uint;


#define GLFW_USE_CHDIR 0
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "screen.h"
#include "spritetransform.h"
#include "sprite.h"
#include "common.h"

#include <gtc/matrix_transform.hpp>
#include <string>
#include <iostream>
#include <fstream>



extern bool IGP_detected;


// OpenGL helper functions
void _CheckGL( const char* f, int l )
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		const char* errStr = "UNKNOWN ERROR";
		if (error == 0x500) errStr = "INVALID ENUM";
		else if (error == 0x502) errStr = "INVALID OPERATION";
		else if (error == 0x501) errStr = "INVALID VALUE";
		else if (error == 0x506) errStr = "INVALID FRAMEBUFFER OPERATION";
		FATALERROR( "GL error %d: %s at %s:%d\n", error, errStr, f, l );
	}
}

GLuint CreateVBO( const GLfloat* data, const uint size )
{
	GLuint id;
	glGenBuffers( 1, &id );
	glBindBuffer( GL_ARRAY_BUFFER, id );
	glBufferData( GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW );
	CheckGL();
	return id;
}

void BindVBO( const uint idx, const uint N, const GLuint id )
{
	glEnableVertexAttribArray( idx );
	glBindBuffer( GL_ARRAY_BUFFER, id );
	glVertexAttribPointer( idx, N, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	CheckGL();
}

void CheckShader( GLuint shader )
{
	char buffer[1024];
	memset( buffer, 0, sizeof( buffer ) );
	GLsizei length = 0;
	glGetShaderInfoLog( shader, sizeof( buffer ), &length, buffer );
	CheckGL();
	FATALERROR_IF( length > 0 && strstr( buffer, "ERROR" ), "Shader compile error:\n%s", buffer );
}

void CheckProgram( GLuint id )
{
	char buffer[1024];
	memset( buffer, 0, sizeof( buffer ) );
	GLsizei length = 0;
	glGetProgramInfoLog( id, sizeof( buffer ), &length, buffer );
	CheckGL();
	FATALERROR_IF( length > 0, "Shader link error:\n%s", buffer );
}

void DrawQuad(Shader* shader)
{
	static GLuint vao = 0;
	if (!vao)
	{
		// generate buffers
		static const GLfloat verts[] = { 
			0, SCRHEIGHT, 0, 
			SCRWIDTH, SCRHEIGHT, 0,
			0, 0, 0, 
			SCRWIDTH, SCRHEIGHT, 0, 
			0, 0, 0, 
			SCRWIDTH, 0, 0 };
		static const GLfloat uvdata[] = { 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1 };
		static const GLfloat verts_igp[] = { 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0 };
		static const GLfloat uvdata_igp[] = { -1, -1, 1, -1, -1, 1, 1, -1, -1, 1, 1, 1 };
		GLuint vertexBuffer, UVBuffer;
		if (IGP_detected)
		{
			// not sure why it is needed - without it IGPs tile and clamp the output.
			vertexBuffer = CreateVBO( verts_igp, sizeof( verts_igp ) );
			UVBuffer = CreateVBO( uvdata_igp, sizeof( uvdata_igp ) );
		}
		else
		{
			vertexBuffer = CreateVBO( verts, sizeof( verts ) );
			UVBuffer = CreateVBO( uvdata, sizeof( uvdata ) );
		}
		glGenVertexArrays( 1, &vao );
		glBindVertexArray( vao );
		BindVBO( 0, 3, vertexBuffer );
		BindVBO( 1, 2, UVBuffer );
		glBindVertexArray( 0 );
		CheckGL();
	}
	glm::mat4 model = glm::mat4(1);
	shader->SetInputMatrix("model", model);
	glBindVertexArray( vao );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
	glBindVertexArray( 0 );
}

void DrawQuadSprite(Shader* shader, Stransform& transform, GLTexture* texture, spriteStr* Sprite)
{
	static GLuint vao = 0;
	if (!vao)
	{
		// generate buffers
		static const GLfloat verts[] = { 
			-1, 1, 0, 
			1, 1, 0, 
			-1, -1, 0, 
			1, 1, 0, 
			-1, -1, 0, 
			1, -1, 0 };
		static const GLfloat uvdata[] = { 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1 };
		static const GLfloat verts_igp[] = { 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0 };
		static const GLfloat uvdata_igp[] = { -1, -1, 1, -1, -1, 1, 1, -1, -1, 1, 1, 1 };
		if (IGP_detected)
		{
			// not sure why it is needed - without it IGPs tile and clamp the output.
			shader->setVBO(0, CreateVBO(verts_igp, sizeof(verts_igp)));
			shader->setVBO(1, CreateVBO(uvdata_igp, sizeof(uvdata_igp)));
		}
		else
		{
			shader->setVBO(0, CreateVBO(verts, sizeof(verts)));
			shader->setVBO(1, CreateVBO(uvdata, sizeof(uvdata)));
		}
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		BindVBO(0, 3, shader->getVBO(0));
		BindVBO(1, 2, shader->getVBO(1));
		glBindVertexArray(0);
		CheckGL();
	}

	//Translate position
	glm::mat4 model = glm::mat4(1);

	glm::vec3 pos = glm::vec3(transform.getTranslation() - glm::vec2(Sprite->getAfterWidthDif(), Sprite->getAfterHeightDif()), 0.0f);
	model = glm::translate(model, pos);

	//Check for pivot
	if (transform.getRotationPivot() != glm::vec2(0.5f, 0.5f))
	{
		glm::vec2 pivot = transform.getRotationPivot();
		if (transform.getScale().y < 0)
		{
			pivot.y = pivot.y * -1 + 1;
		}
		if (transform.getScale().x < 0)
		{
			pivot.x = pivot.x * -1 + 1;
		}

		//Move based on pivot
		{
			glm::vec2 newPos = pivot;
			newPos = -newPos + 0.5f;
			newPos *= glm::vec2(Sprite->getAfterWidth(), Sprite->getAfterHeight());
			model = glm::translate(model, glm::vec3(newPos, 0.0f));
		}

		pivot *= glm::vec2(Sprite->getAfterWidth(), Sprite->getAfterHeight());

		pivot -= 0.5f * glm::vec2(Sprite->getAfterWidth(), Sprite->getAfterHeight());

		//Take scaling into account 
		//pivot *= glm::vec2(transform.getScale().x < 0 ? -1.0f : 1.0f, transform.getScale().y < 0 ? -1.0f : 1.0f);



		model = glm::translate(model, glm::vec3(pivot, 0.0f)); //Move
		model = glm::rotate(model, transform.getRotation(), glm::vec3(0, 0, 1)); //Rotate
		model = glm::translate(model, glm::vec3(-pivot, 0.0f)); //Move back
	}
	else
	{
		model = glm::rotate(model, transform.getRotation(), glm::vec3(0, 0, 1));
	}
	//----------------!!Scaling down everything by 0.5, this because the verts cover from -1 to 1 which is 2 times the scale!!-----------------------
	model = glm::scale(model, glm::vec3(texture->width * transform.getScale().x * 0.5f, texture->height * transform.getScale().y * 0.5f, 1.0f));

	shader->SetInputMatrix("model", model);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

// OpenGL texture wrapper class
GLTexture::GLTexture( uint w, uint h, uint type)
{
	width = w;
	height = h;
	glGenTextures( 1, &ID );
	glBindTexture( GL_TEXTURE_2D, ID );
	if (type == DEFAULT)
	{
		// regular texture
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	else if (type == INTTARGET)
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
	}
	else /* type == FLOAT */
	{
		// floating point texture
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0 );
	}
	glBindTexture( GL_TEXTURE_2D, 0 );
	CheckGL();
}

GLTexture::~GLTexture()
{
	glDeleteTextures( 1, &ID );
	CheckGL();
}

void GLTexture::Bind( const uint slot )
{
	glActiveTexture( GL_TEXTURE0 + slot );
	glBindTexture( GL_TEXTURE_2D, ID );
	CheckGL();
}

void GLTexture::CopyFrom( unsigned int* pixels )
{
	glBindTexture( GL_TEXTURE_2D, ID );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	CheckGL();
}

void GLTexture::CopyTo( screen* dst )
{
	glBindTexture( GL_TEXTURE_2D, ID );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst->pixels );
	CheckGL();
}

// Shader class implementation
Shader::Shader( const char* vfile, const char* pfile, bool fromString )
{
	if (fromString)
	{
		Compile( vfile, pfile );
	}
	else
	{
		Init( vfile, pfile );
	}
}

Shader::~Shader()
{
	glDetachShader( ID, pixel );
	glDetachShader( ID, vertex );
	glDeleteShader( pixel );
	glDeleteShader( vertex );
	glDeleteProgram( ID );
	CheckGL();
}

std::string FileRead(const char* _File)
{
	std::ifstream s(_File);
	std::string str((std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());
	s.close();
	return str;
}

void Shader::Init( const char* vfile, const char* pfile )
{
	std::string vsText = FileRead( vfile );
	std::string fsText = FileRead( pfile );
	FATALERROR_IF( vsText.size() == 0, "File %s not found", vfile );
	FATALERROR_IF( fsText.size() == 0, "File %s not found", pfile );
	const char* vertexText = vsText.c_str();
	const char* fragmentText = fsText.c_str();
	Compile( vertexText, fragmentText );
}

void Shader::Compile( const char* vtext, const char* ftext )
{
	vertex = glCreateShader( GL_VERTEX_SHADER );
	pixel = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( vertex, 1, &vtext, 0 );
	glCompileShader( vertex );
	CheckShader( vertex );
	glShaderSource( pixel, 1, &ftext, 0 );
	glCompileShader( pixel );
	CheckShader( pixel );
	ID = glCreateProgram();
	glAttachShader( ID, vertex );
	glAttachShader( ID, pixel );
	glBindAttribLocation( ID, 0, "position" );
	glBindAttribLocation( ID, 1, "texCoord" );
	glLinkProgram( ID );
	CheckProgram( ID );
	CheckGL();
}

void Shader::Bind()
{
	glUseProgram( ID );
	CheckGL();
}

void Shader::Unbind()
{
	glUseProgram( 0 );
	CheckGL();
}

void Shader::SetInputTexture( uint slot, const char* name, GLTexture* texture )
{
	glActiveTexture( GL_TEXTURE0 + slot );
	glBindTexture( GL_TEXTURE_2D, texture->ID );
	glUniform1i( glGetUniformLocation( ID, name ), slot );
	CheckGL();
}

void Shader::SetInputMatrix( const char* name, const glm::mat4 & matrix)
{
	//const GLfloat* data = (const GLfloat*)&matrix;
	GLint location = glGetUniformLocation(ID, name);
	CheckGL();
	glUniformMatrix4fv( location, 1, GL_FALSE, &matrix[0][0]);
	CheckGL();
}

void Shader::SetFloat( const char* name, const float v )
{
	glUniform1f( glGetUniformLocation( ID, name ), v );
	CheckGL();
}

void Shader::SetInt( const char* name, const int v )
{
	glUniform1i( glGetUniformLocation( ID, name ), v );
	CheckGL();
}

void Shader::SetUInt( const char* name, const uint v )
{
	glUniform1ui( glGetUniformLocation( ID, name ), v );
	CheckGL();
}

void FatalError(const char* fmt, ...)
{

	va_list args;
	va_start(args, fmt);

	// Print the formatted error message
	fprintf(stderr, "FATAL ERROR: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");

	va_end(args);

	// Exit the program with failure code
	exit(EXIT_FAILURE);
}