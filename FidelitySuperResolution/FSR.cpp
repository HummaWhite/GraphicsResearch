#include "FSR.h"

#include <array>

using Vec2f = glm::vec2;
using Vec3f = glm::vec3;
using Vec4f = glm::vec4;
using Vec2i = glm::ivec2;

std::array<Vec4f, 4> EasuSetupConst(
	Vec2f inputViewport,
	Vec2f outputViewport)
{
	std::array<Vec4f, 4> ret;
	Vec2f inputSizeInv = Vec2f(1.0f) / inputViewport;
	ret[0] = Vec4f(inputViewport / outputViewport, inputViewport / outputViewport * 0.5f - 0.5f);
	ret[1] = Vec4f(inputSizeInv, inputSizeInv * Vec2f(1.0f, -1.0f));
	ret[2] = Vec4f(inputSizeInv * Vec2f(-1.0f, 2.0f), inputSizeInv * Vec2f(1.0f, 2.0f));
	ret[3] = Vec4f(inputSizeInv * Vec2f(0.0f, 4.0f), Vec2f(0.0f));
	return ret;
}

float RcasSetupConst(float sharpness)
{
	return std::exp2f(-sharpness);
}

FSR::FSR()
{
	glfwSetInputMode(this->window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

FSR::~FSR()
{
}

void FSR::init()
{
	screenVB.allocate(sizeof(SCREEN_COORD), SCREEN_COORD, 6);
	screenVA.addBuffer(screenVB, LAYOUT_POS2);

	screenShader.load("screenQuad.shader");
	easuShader.setComputeShaderSizeHint(workgroupSizeX, workgroupSizeY, 1);
	easuShader.setHeaderHint("#version 450 core");
	easuShader.load("EASU.shader");
	rcasShader.setComputeShaderSizeHint(workgroupSizeX, workgroupSizeY, 1);
	rcasShader.setHeaderHint("#version 450 core");
	rcasShader.load("RCAS.shader");

	image.loadSingle("res/image/6.png", GL_RGBA32F);

	const int Scale = 2;
	easuTex.generate2D(image.width() * Scale, image.height() * Scale, GL_RGBA32F);
	easuTex.setFilterAndWrapping(GL_LINEAR, GL_CLAMP_TO_EDGE);
	glBindImageTexture(0, easuTex.ID(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);

	rcasTex.generate2D(image.width() * Scale, image.height() * Scale, GL_RGBA32F);
	rcasTex.setFilterAndWrapping(GL_LINEAR, GL_CLAMP_TO_EDGE);
	glBindImageTexture(1, rcasTex.ID(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);

	easuShader.setTexture("inImage", image, 1);
	rcasShader.setTexture("inImage", easuTex, 2);
	screenShader.setTexture("frameBuffer", rcasTex, 0);

	Vec2f imgSize(image.width(), image.height());
	auto easuConst = EasuSetupConst(imgSize, imgSize * float(Scale));

	for (int i = 0; i < 4; i++)
		easuShader.setVec4(("con" + std::to_string(i)).c_str(), easuConst[i]);

	this->resizeWindow(image.width() * Scale, image.height() * Scale);

	this->setupGUI();
	this->reset();
}

void FSR::renderLoop()
{
	this->processKey(0, 0, 0, 0);
	VerticalSyncStatus(verticalSync);

	renderer.clear(0.0f, 0.0f, 0.0f);

	this->renderFrame();
	this->renderGUI();
	this->swapBuffers();
	this->display();
}

void FSR::terminate()
{
}

void FSR::processKey(int key, int scancode, int action, int mode)
{
	if (this->getKeyStatus(GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		this->setTerminateStatus(true);
	}
}

void FSR::processCursor(float posX, float posY)
{
}

void FSR::processScroll(float offsetX, float offsetY)
{
}

void FSR::processResize(int width, int height)
{
}

void FSR::renderFrame()
{
	int xNum = (this->windowWidth() + workgroupSizeX - 1) / workgroupSizeX;
	int yNum = (this->windowHeight() + workgroupSizeY - 1) / workgroupSizeY;

	easuShader.enable();
	glDispatchCompute(xNum, yNum, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	rcasShader.enable();
	glDispatchCompute(xNum, yNum, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	renderer.clear(0.0f, 0.0f, 0.0f);

	this->setViewport(0, 0, this->windowWidth(), this->windowHeight());
	renderer.draw(screenVA, screenShader);
}

void FSR::reset()
{
	rcasShader.set1f("sharpness", RcasSetupConst(sharpness));
	rcasShader.set1i("removeNoise", removeNoise);
}

void FSR::setupGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(this->window(), true);
	ImGui_ImplOpenGL3_Init("#version 450");
}

void FSR::renderGUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("FSR");
	{
		if (ImGui::SliderFloat("Sharpness", &sharpness, 0.0f, 1.0f) ||
			ImGui::Checkbox("Remove Noise", &removeNoise))
		{
			reset();
		}

		if (ImGui::Button("Save Image"))
		{
			int w = this->windowWidth();
			int h = this->windowHeight();
			uint8_t* buf = new uint8_t[w * h * 3];

			glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);

			std::string name = "screenshots/save" + std::to_string((uint64_t)time(0)) + ".png";
			stbi_write_png(name.c_str(), w, h, 3, buf + w * (h - 1) * 3, -w * 3);

			delete[] buf;
		}

		ImGui::Checkbox("Vertical Sync", &verticalSync);

		if (ImGui::Button("Exit"))
		{
			this->setTerminateStatus(true);
		}
	}
	ImGui::End();

	if (ImGui::Begin("Example: Simple overlay", nullptr,
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav))
	{
		ImGui::Text("Render Time: %.3f ms, FPS: %.3f", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}
	ImGui::End();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
