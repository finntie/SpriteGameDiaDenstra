// Template, IGAD version 3
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2023

#pragma once

#include <glm.hpp>
#define NOMINMAX
#include <windows.h>
#include <glad.h>

// OpenGL texture wrapper
class screen;
struct Stransform;
struct spriteStr;
class GLTexture
{
public:
	enum { DEFAULT = 0, FLOAT = 1, INTTARGET = 2 };
	// constructor / destructor
	GLTexture(unsigned int width, unsigned int height, unsigned int type = DEFAULT );
	~GLTexture();
	// methods
	void Bind( const unsigned int slot = 0 );
	void CopyFrom( unsigned int* pixels);
	void CopyTo( screen* dst );
	unsigned int width = 0, height = 0;
public:
	// public data members
	GLuint ID = 0;
};

// template function access
GLTexture* GetRenderTarget();
bool WindowHasFocus();

// shader wrapper
class Shader
{
public:
	// constructor / destructor
	Shader( const char* vfile, const char* pfile, bool fromString );
	~Shader();
	// methods
	void Init( const char* vfile, const char* pfile );
	void Compile( const char* vtext, const char* ftext );
	void Bind();
	void SetInputTexture(unsigned int slot, const char* name, GLTexture* texture );
	void SetInputMatrix( const char* name, const glm::mat4& matrix );
	void SetFloat( const char* name, const float v );
	void SetInt( const char* name, const int v );
	void SetUInt( const char* name, const unsigned int v );
	void Unbind();
	GLuint getVBO(int idx) { if (idx == 0) return vertexBuffer; else return UVBuffer; }
	void setVBO(int idx, GLuint VBO) { if (idx == 0) vertexBuffer = VBO; else UVBuffer = VBO; }
private:
	// data members
	unsigned int vertex = 0;	// vertex shader identifier
	unsigned int pixel = 0;		// fragment shader identifier
	unsigned int ID = 0;		// shader program identifier
	GLuint vertexBuffer, UVBuffer; //VBOs
};

//Error function (ChatGPT)
void FatalError(const char* fmt, ...);

// generic error checking for OpenGL code
#define CheckGL() { _CheckGL( __FILE__, __LINE__ ); }

// fatal error reporting (with a pretty window)
#define FATALERROR( fmt, ... ) FatalError( "Error on line %d of %s: " fmt "\n", __LINE__, __FILE__, ##__VA_ARGS__ )
#define FATALERROR_IF( condition, fmt, ... ) do { if ( ( condition ) ) FATALERROR( fmt, ##__VA_ARGS__ ); } while ( 0 )
#define FATALERROR_IN( prefix, errstr, fmt, ... ) FatalError( prefix " returned error '%s' at %s:%d" fmt "\n", errstr, __FILE__, __LINE__, ##__VA_ARGS__ );
#define FATALERROR_IN_CALL( stmt, error_parser, fmt, ... ) do { auto ret = ( stmt ); if ( ret ) FATALERROR_IN( #stmt, error_parser( ret ), fmt, ##__VA_ARGS__ ) } while ( 0 )

// forward declarations of helper functions
void _CheckGL( const char* f, int l );
GLuint CreateVBO( const GLfloat* data, const unsigned int size );
void BindVBO( const unsigned int idx, const unsigned int N, const GLuint id );
void CheckShader( GLuint shader, const char* vshader, const char* fshader );
void CheckProgram( GLuint id, const char* vshader, const char* fshader );
void DrawQuad(Shader* shader);
void DrawQuadSprite(Shader* shader, Stransform& spriteTransform, GLTexture* Texture, spriteStr* Sprite);