#pragma once

#include <iostream>
#include <cstdlib>

#include "Buffer2D.h"
#include "../Display/Color.h"
#include "../Math/Transform.h"

#include "../../Ext/stbIncluder.h"

class Texture:
	public Buffer2D<glm::vec3>
{
public:
	enum FilterType
	{
		NEAREST = 0,
		LINEAR
	};

public:
	void loadRGB24(const char *filePath);
	void loadFloat(const char *filePath);

	glm::vec4 get(float u, float v);
	glm::vec4 getSpherical(const glm::vec3 &uv);

	void setFilterType(FilterType type) { filterType = type; }

	int texWidth() const { return width; }
	int texHeight() const { return height; }

	void saveImage(std::string path);

public:
	FilterType filterType = FilterType::LINEAR;
};