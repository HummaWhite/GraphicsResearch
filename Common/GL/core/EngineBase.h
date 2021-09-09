#pragma once

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <random>
#include <ctime>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../../ext/imguiIncluder.h"

#include "Model.h"
#include "Shader.h"
#include "Buffer.h"
#include "VertexArray.h"
#include "Renderer.h"
#include "Texture.h"
#include "Camera.h"
#include "Shape.h"
#include "VerticalSync.h"
#include "FrameBuffer.h"
#include "CheckError.h"
#include "Accelerator/BVH.h"
#include "Scene.h"
#include "EnvironmentMap.h"

class EngineBase
{
public:
	EngineBase();
	~EngineBase();

	int run();
	void setViewport(int x, int y, int width, int height);
	void resizeWindow(int width, int height);

	int windowWidth() const { return m_WindowWidth; }
	int windowHeight() const { return m_WindowHeight; }

	GLFWwindow* window() const { return m_Window; }
	bool shouldTerminate() const { return m_ShouldTerminate; }
	void setTerminateStatus(bool status) { m_ShouldTerminate = status; }
	void setRenderLoopMode(bool loop) { m_RenderLoop = loop; }

	virtual void processKey(int key, int scancode, int action, int mode) = 0;
	virtual void processCursor(float posX, float posY) = 0;
	virtual void processScroll(float offsetX, float offsetY) = 0;
	virtual void processResize(int width, int height) = 0;

	int getKeyStatus(int key) { return glfwGetKey(m_Window, key); }

	void swapBuffers();
	void display();
	void error(const char* errString);

private:
	void setupGL(int width, int height, bool border);

	virtual void init() = 0;
	virtual void renderLoop() = 0;
	virtual void terminate() = 0;

private:
	GLFWwindow* m_Window;
	bool m_ShouldTerminate;
	bool m_ErrorOccured;
	bool m_RenderLoop;
	int m_WindowWidth, m_WindowHeight;
};
