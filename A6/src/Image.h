#pragma once
#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <string>
#include <vector>
#include "stb_image.h"
#include "glm/vec3.hpp"

class Image
{
public:
	Image(int width, int height);
	Image(const std::string& filename);
	virtual ~Image();
	void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
	void writeToFile(const std::string &filename);
	glm::vec3 getColorAt(double u, double v) const;
	int getWidth() const { return width; }
	int getHeight() const { return height; }

private:
	int width;
	int height;
	int comp;
	std::vector<unsigned char> pixels;
};

#endif
