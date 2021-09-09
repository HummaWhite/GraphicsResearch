#pragma once

#include "../Common/GL/Includer.h"

class FSR :
	public EngineBase
{
public:
	FSR();
	~FSR();

private:
	void init();
	void renderLoop();
	void terminate();

	void processKey(int key, int scancode, int action, int mode);
	void processCursor(float posX, float posY);
	void processScroll(float offsetX, float offsetY);
	void processResize(int width, int height);

	void renderFrame();
	void reset();

	void setupGUI();
	void renderGUI();

private:
	Buffer screenVB;
	VertexArray screenVA;

	Shader easuShader, rcasShader, screenShader;
	Texture easuTex, rcasTex;
	Texture image;

	bool verticalSync = false;

	int workgroupSizeX = 32;
	int workgroupSizeY = 48;

	float sharpness = 0.0f;
	bool removeNoise = true;
};
