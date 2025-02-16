// Template, IGAD version 3
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2023

#pragma once

#include <algorithm>
#include <iostream>

// helper macro for line clipping
#define OUTCODE(x,y) (((x)<xmin)?1:(((x)>xmax)?2:0))+(((y)<ymin)?4:(((y)>ymax)?8:0))

// pixel operations

// ScaleColor: change the intensity of red, green and blue using a single
// fixed-point scale in the range 0..256, where 256 is 100%.
inline unsigned int ScaleColor(const unsigned int c, const unsigned int scale)
{
	const unsigned int rb = (((c & 0xff00ff) * scale) >> 8) & 0x00ff00ff;
	const unsigned int ag = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;
	return rb + ag;
}

// AddBlend: add together two colors, with clamping.
inline unsigned int AddBlend(const unsigned int c1, const unsigned int c2)
{
	const unsigned int r1 = (c1 >> 16) & 255, r2 = (c2 >> 16) & 255;
	const unsigned int g1 = (c1 >> 8) & 255, g2 = (c2 >> 8) & 255;
	const unsigned int b1 = c1 & 255, b2 = c2 & 255;
	const unsigned int r = std::min(255u, r1 + r2);
	const unsigned int g = std::min(255u, g1 + g2);
	const unsigned int b = std::min(255u, b1 + b2);
	return (r << 16) + (g << 8) + b;
}

// SubBlend: subtract a color from another color, with clamping.
inline unsigned int SubBlend(unsigned int a_Color1, unsigned int a_Color2)
{
	int red = (a_Color1 & 0xff0000) - (a_Color2 & 0xff0000);
	int green = (a_Color1 & 0x00ff00) - (a_Color2 & 0x00ff00);
	int blue = (a_Color1 & 0x0000ff) - (a_Color2 & 0x0000ff);
	if (red < 0) red = 0;
	if (green < 0) green = 0;
	if (blue < 0) blue = 0;
	return (unsigned int)(red + green + blue);
}

// 32-bit screen container
class screen
{
	enum { OWNER = 1 };
public:
	// constructor / destructor
	screen() = default;
	screen(int w, int h, unsigned int* buffer);
	screen(int w, int h);
	screen(const char* file);
	~screen();
	// operations
	void InitCharset();
	void SetChar(int c, const char* c1, const char* c2, const char* c3, const char* c4, const char* c5);
	void Print(const char* t, int x1, int y1, unsigned int c);
	void ReceivePrint(const char* t, unsigned int* buffer, int bufferWidth, unsigned int c);
	void Clear(unsigned int c);
	void Line(float x1, float y1, float x2, float y2, unsigned int c);
	void Plot(int x, int y, unsigned int c);
	void LoadFromFile(const char* file);
	void CopyTo(screen* dst, int x, int y);
	void Box(int x1, int y1, int x2, int y2, unsigned int color);
	void Bar(int x1, int y1, int x2, int y2, unsigned int color);
	void Circle(int x1, int y1, int radius, unsigned int color);
	// attributes
	unsigned int* pixels = 0;
	unsigned int* depthBuffer = 0;
	int width = 0, height = 0;
	bool ownBuffer = false;
	// static data for the hardcoded font
	static inline char font[51][5][6];
	static inline int transl[256];
	static inline bool fontInitialized = false;
};

