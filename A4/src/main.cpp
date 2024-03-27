#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <random>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"



#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Texture.h"
#include "Material.h"
#include "Light.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

GLFWwindow* window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;

shared_ptr<Shape> teapot;
shared_ptr<Shape> shape;
shared_ptr<Shape> sphere;
shared_ptr<Shape> ground;
shared_ptr<Texture> groundTexture;
shared_ptr<Shape> frustum;
glm::mat3 T1(1.0f);


shared_ptr<Program> bPhShader;

vector<shared_ptr<Material>> materials;
vector<shared_ptr<Light>> lights;

//Vectors containing items for 100 objects
vector<int> objBunOrTea;
vector<float> objRotAngles;
vector<float> objPhaseShifts;



float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
int currMaterial = 0;
int currShader = 0;
int currLight = 0;

bool topDownViewActivated = false;



bool keyToggles[256] = { false }; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char* description)
{
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		case GLFW_KEY_W:
			camera->moveForward(0.1);
			break;
		case GLFW_KEY_S:
			camera->moveForward(-0.1);
			break;
		case GLFW_KEY_A:
			camera->moveRight(-0.1);
			break;
		case GLFW_KEY_D:
			camera->moveRight(0.1);
			break;

		case GLFW_KEY_Z:
			if (mods & GLFW_MOD_SHIFT) {
				camera->zoomOut(); //little z

			}
			else {
				camera->zoomIn(); //big z
			}
			break;
		case GLFW_KEY_T:
			topDownViewActivated = !topDownViewActivated;
		}

	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if (action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt);
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse);
	}
}

static void char_callback(GLFWwindow* window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char* filepath, GLFWwindow* w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if (rc) {
		cout << "Wrote to " << filepath << endl;
	}
	else {
		cout << "Couldn't write to " << filepath << endl;
	}
}

void checkShaderLinkStatus(GLuint shaderProgram)
{
	GLint status;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, buffer);
		cerr << "Shader link error: " << buffer << endl;
	}
}



// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initialize time.
	glfwSetTime(0.0);

	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	bPhShader = make_shared<Program>();
	bPhShader->setShaderNames(RESOURCE_DIR + "shaders_vert.glsl", RESOURCE_DIR + "blinnphong_frag.glsl");
	bPhShader->setVerbose(true);
	bPhShader->init();
	bPhShader->addAttribute("aPos");
	bPhShader->addAttribute("aNor");
	bPhShader->addUniform("MV");
	bPhShader->addUniform("P");
	bPhShader->addUniform("invTransformMV");
	bPhShader->addUniform("ka");
	bPhShader->addUniform("kd");
	bPhShader->addUniform("ks");
	bPhShader->addUniform("shininess");
	bPhShader->addUniform("lightPos");
	for (int i = 0; i < 2; ++i) {
		string uniformName = "lightEnabled[" + std::to_string(i) + "]";
		bPhShader->addUniform(uniformName);
	}
	bPhShader->addUniform("lightColor");
	bPhShader->addAttribute("aTex");
	bPhShader->addUniform("T1");
	bPhShader->addUniform("groundTexture");

	groundTexture = make_shared<Texture>();
	groundTexture->setFilename(RESOURCE_DIR + "GrassSamp1.jpg");
	groundTexture->init();
	groundTexture->setUnit(0);
	groundTexture->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	bPhShader->setVerbose(false);


	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation


	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();

	teapot = make_shared<Shape>();
	teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
	teapot->init();


	sphere = make_shared<Shape>();
	sphere->loadMesh(RESOURCE_DIR + "sphere.obj");
	sphere->init();

	ground = make_shared<Shape>();
	ground->loadMesh(RESOURCE_DIR + "square.obj");
	ground->init();

	frustum = make_shared<Shape>();
	frustum->loadMesh(RESOURCE_DIR + "frustum.obj");
	frustum->init();


	//Randomizing Objects (infinite error if done in render)
	std::random_device rd;
	std::mt19937 eng(rd());
	std::uniform_int_distribution<> distr(0, 1);
	objBunOrTea.resize(100);
	for (auto& choice : objBunOrTea) {
		choice = distr(eng);
	}

	//Randomizing Sizing
	std::uniform_real_distribution<> phaseDistr(0.0, 2 * M_PI);
	objPhaseShifts.resize(100);
	for (auto& phase : objPhaseShifts) {
		phase = phaseDistr(eng);
	}


	//Randomizing Color
	std::uniform_real_distribution<> distrMat(0.00, 100.0);
	for (int i = 0; i < 100; ++i) {
		float kd1 = static_cast<float>(distrMat(eng)) / 100.00;
		float kd2 = static_cast<float>(distrMat(eng)) / 100.00;
		float kd3 = static_cast<float>(distrMat(eng)) / 100.00;
		materials.push_back(make_shared<Material>(glm::vec3(0.2, 0.2, 0.2), glm::vec3(kd1, kd2, kd3), glm::vec3(1.0, 0.9, 1.0), 200.0f)); //Mat 1
	}

	//Randomizing Rotation
	std::uniform_real_distribution<> distrRot(0.0, 2 * M_PI);
	objRotAngles.resize(100);
	for (auto& angle : objRotAngles) {
		angle = distrRot(eng);
	}
	


	lights.push_back(make_shared<Light>(glm::vec3(10.0, 10.0, 10.0), glm::vec3(0.8, 0.8, 0.8)));
	lights.push_back(make_shared<Light>(glm::vec3(0.0, 1.0, -5.0), glm::vec3(0.8, 0.8, 0.8)));



	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	}
	else {
		glDisable(GL_CULL_FACE);
	}

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspectRatio = (float)width / (float)height;
	float scaleFactor = 1.0f / sqrt(aspectRatio);
	camera->setAspect(aspectRatio);

	double t = glfwGetTime();

	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();

	shared_ptr<Program> useProg = bPhShader;


	///HUD///////////////////////////////////////////////

	useProg->bind();
	glViewport(0, 0, width, height);
	glUniform3f(useProg->getUniform("ka"), 0.13, 0.13, 0.13);
	glUniform3f(useProg->getUniform("kd"), 0.9, 0.9, 0.9);
	glUniform3f(useProg->getUniform("ks"), 0.2, 0.2, 0.5);
	glUniform1f(useProg->getUniform("shininess"), 200);


	P->pushMatrix();
	glUniform1i(glGetUniformLocation(useProg->pid, "lightEnabled[0]"), 0);
	glUniform1i(glGetUniformLocation(useProg->pid, "lightEnabled[1]"), 1);

	glUniform3fv(glGetUniformLocation(useProg->pid, "lightPositions[0]"), 1, glm::value_ptr(lights[0]->position));
	glUniform3fv(glGetUniformLocation(useProg->pid, "lightColors[0]"), 1, glm::value_ptr(lights[0]->color));
	glUniform3fv(glGetUniformLocation(useProg->pid, "lightPositions[1]"), 1, glm::value_ptr(lights[1]->position));
	glUniform3fv(glGetUniformLocation(useProg->pid, "lightColors[1]"), 1, glm::value_ptr(lights[1]->color));

	MV->pushMatrix();

	MV->translate(-0.78f, 0.55f, 0.1f);			
	MV->scale(-0.2 * scaleFactor, 0.3 * scaleFactor, -0.2);
	MV->rotate(t, 0, -1, 0);
	glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniformMatrix3fv(useProg->getUniform("invTransformMV"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())))));
	glUniformMatrix4fv(useProg->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));

	teapot->draw(useProg);
	MV->popMatrix();



	MV->pushMatrix();

	MV->translate(0.78f, 0.45f, -0.1f);			
	MV->scale(-0.2 * scaleFactor, 0.3 * scaleFactor, 0.2);
	MV->rotate(t, 0, 1, 0);
	glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniformMatrix3fv(useProg->getUniform("invTransformMV"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())))));
	glUniformMatrix4fv(useProg->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));

	shape->draw(useProg);

	MV->popMatrix();
	P->popMatrix();


	/////////////////////////////////////////////////////


	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);

	glUniformMatrix4fv(useProg->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));


	glUniform1i(glGetUniformLocation(useProg->pid, "lightEnabled[0]"), 1); //Figuring out how to make the lights not affect the HUD took HOURS
	glUniform1i(glGetUniformLocation(useProg->pid, "lightEnabled[1]"), 0);

	glm::vec4 lightPosCamSpace = MV->topMatrix() * glm::vec4(lights[0]->position, 1.0);
	glUniform3fv(glGetUniformLocation(useProg->pid, "lightPositions[0]"), 1, glm::value_ptr(glm::vec3(lightPosCamSpace)));
	glUniform3fv(glGetUniformLocation(useProg->pid, "lightColors[0]"), 1, glm::value_ptr(lights[0]->color));
	glUniform3fv(glGetUniformLocation(useProg->pid, "lightPositions[1]"), 1, glm::value_ptr(lights[1]->position));
	glUniform3fv(glGetUniformLocation(useProg->pid, "lightColors[1]"), 1, glm::value_ptr(lights[1]->color));



	//FLAT SURFACE =============================================================

	groundTexture->bind(useProg->getUniform("groundTexture"));
	glUniformMatrix3fv(useProg->getUniform("T1"), 1, GL_FALSE, glm::value_ptr(T1));

	MV->pushMatrix();

	MV->translate(glm::vec3(0, -0.5, -17));
	MV->rotate((float)(90.0 * M_PI / 180.0), glm::vec3(1, 0, 0));
	MV->scale(glm::vec3(10.0, 80.0, 0.001));
	glUniform3f(useProg->getUniform("ka"), 0.0f, 0.5f, 0.0f);
	glUniform3f(useProg->getUniform("kd"), 0.1f, 0.6f, 0.1f);
	glUniform3f(useProg->getUniform("ks"), 0.0f, 0.0f, 0.0f);
	glUniform1f(useProg->getUniform("shininess"), 1.0f);

	glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniformMatrix3fv(useProg->getUniform("invTransformMV"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())))));


	ground->draw(useProg);

	MV->popMatrix();


	//FLAT SURFACE =============================================================


	float spacing = 1.25f;
	glm::vec3 gridOrigin(-spacing * 4.5f, -0.5f, -spacing * 4.5f);
	int count = 0;

	for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j) {
		
			shared_ptr<Shape> currentShape;

			if (objBunOrTea[i * 10 + j] == 0) {
				currentShape = shape;
			}
			else {
				currentShape = teapot;
			}

			glm::vec3 position = gridOrigin + glm::vec3(spacing * j, 0.0f, spacing * i);


		glUniform3fv(useProg->getUniform("ka"), 1, glm::value_ptr(materials[count % materials.size()]->ka));
		glUniform3fv(useProg->getUniform("kd"), 1, glm::value_ptr(materials[count % materials.size()]->kd));
		glUniform3fv(useProg->getUniform("ks"), 1, glm::value_ptr(materials[count % materials.size()]->ks));
		glUniform1f(useProg->getUniform("shininess"), materials[count % materials.size()]->shininess);
		MV->pushMatrix();

		float shiftedTime = glfwGetTime() + objPhaseShifts[i+j];
		float sFactor = 0.5f + 0.1f * sinf(shiftedTime);


		MV->translate(position);
		MV->rotate(objRotAngles[count], glm::vec3(0, 1, 0));
		MV->scale(glm::vec3(sFactor));
		if (currentShape == shape) { //Grounding
			MV->translate(glm::vec3(0.0f, -0.335f, 0.0f));
		}
		else if (currentShape == teapot) {
			MV->translate(glm::vec3(0.0f, -0.005f, 0.0f));
		}
		glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix3fv(useProg->getUniform("invTransformMV"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())))));
		currentShape->draw(useProg);
		MV->popMatrix();
		count++;
		}
	}

	MV->pushMatrix();
	MV->translate(glm::vec3(10.0, 10.0, 10.0));
	glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniform3f(useProg->getUniform("ka"), 1.0, 1.0, 0.0);
	glUniform3f(useProg->getUniform("kd"), 0.0, 0.0, 0.0);
	glUniform3f(useProg->getUniform("ks"), 0.0, 0.0, 0.0);
	glUniform1f(useProg->getUniform("shininess"), 1.0);
	sphere->draw(useProg);
	MV->popMatrix();


	P->popMatrix();
	MV->popMatrix();



	if (topDownViewActivated) {
		double s = 0.5;
		int viewportWidth = static_cast<int>(s * width);
		int viewportHeight = static_cast<int>(s * height);
		glViewport(0, 0, viewportWidth, viewportHeight);
		glEnable(GL_SCISSOR_TEST);
		glScissor(0, 0, viewportWidth, viewportHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);

		P->pushMatrix();
		MV->pushMatrix();


		P->multMatrix(glm::ortho(-7.0f, 7.0f, -7.0f, 7.0f, -1.0f, 100.0f));

		MV->loadIdentity();
		MV->translate(glm::vec3(0, 0, -50));
		MV->rotate(M_PI_2, glm::vec3(1, 0, 0));


		/////////////////////////////////////////////////////////////////

		glUniformMatrix4fv(useProg->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));

		glUniform1i(glGetUniformLocation(useProg->pid, "lightEnabled[0]"), 1); //Figuring out how to make the lights not affect the HUD took HOURS
		glUniform1i(glGetUniformLocation(useProg->pid, "lightEnabled[1]"), 0);

		glm::vec4 lightPosCamSpace = MV->topMatrix() * glm::vec4(lights[0]->position, 1.0);
		glUniform3fv(glGetUniformLocation(useProg->pid, "lightPositions[0]"), 1, glm::value_ptr(glm::vec3(lightPosCamSpace)));
		glUniform3fv(glGetUniformLocation(useProg->pid, "lightColors[0]"), 1, glm::value_ptr(lights[0]->color));
		glUniform3fv(glGetUniformLocation(useProg->pid, "lightPositions[1]"), 1, glm::value_ptr(lights[1]->position));
		glUniform3fv(glGetUniformLocation(useProg->pid, "lightColors[1]"), 1, glm::value_ptr(lights[1]->color));



		float theta = camera->fovy;
		float a = camera->aspect;
		float sx = atan(theta / 2.0f);
		float sy = sx / a;

		P->pushMatrix();
		MV->pushMatrix();
		glm::mat4 viewMatrix = glm::lookAt(camera->position, camera->position + glm::vec3(std::sin(camera->yaw), -std::sin(camera->pitch), -std::cos(camera->yaw)), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 cameraMatrix = glm::inverse(viewMatrix);
		MV->multMatrix(cameraMatrix);
		glUniform3f(useProg->getUniform("ka"), 0.48f, 0.72f, 0.84f);
		glUniform3f(useProg->getUniform("kd"), 0.48f, 0.72f, 0.84f);
		glUniform3f(useProg->getUniform("ks"), 0.5f, 0.5f, 0.5f);
		glUniform1f(useProg->getUniform("shininess"), 50.0f);
		MV->scale(glm::vec3(sx, sy, 1.0f));
		glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));

		frustum->draw(useProg);

		MV->popMatrix();
		P->popMatrix();

		//FLAT SURFACE =============================================================
		MV->pushMatrix();

		MV->translate(glm::vec3(0, -0.5, -17));
		MV->rotate((float)(90.0 * M_PI / 180.0), glm::vec3(1, 0, 0));
		MV->scale(glm::vec3(10.0, 80.0, 0.001));
		glUniform3f(useProg->getUniform("ka"), 0.0f, 0.5f, 0.0f);
		glUniform3f(useProg->getUniform("kd"), 0.1f, 0.6f, 0.1f);
		glUniform3f(useProg->getUniform("ks"), 0.0f, 0.0f, 0.0f);
		glUniform1f(useProg->getUniform("shininess"), 1.0f);

		groundTexture->bind(useProg->getUniform("groundTexture"));
		glUniformMatrix3fv(useProg->getUniform("T1"), 1, GL_FALSE, glm::value_ptr(T1));
		glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix3fv(useProg->getUniform("invTransformMV"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())))));


		ground->draw(useProg);

		MV->popMatrix();

		//FLAT SURFACE =============================================================


		float spacing = 1.25f;
		glm::vec3 gridOrigin(-spacing * 4.5f, -0.5f, -spacing * 4.5f);
		int count = 0;

		for (int i = 0; i < 10; ++i) {
			for (int j = 0; j < 10; ++j) {

				shared_ptr<Shape> currentShape;

				if (objBunOrTea[i * 10 + j] == 0) {
					currentShape = shape;
				}
				else {
					currentShape = teapot;
				}

				glm::vec3 position = gridOrigin + glm::vec3(spacing * j, 0.0f, spacing * i);


				glUniform3fv(useProg->getUniform("ka"), 1, glm::value_ptr(materials[count % materials.size()]->ka));
				glUniform3fv(useProg->getUniform("kd"), 1, glm::value_ptr(materials[count % materials.size()]->kd));
				glUniform3fv(useProg->getUniform("ks"), 1, glm::value_ptr(materials[count % materials.size()]->ks));
				glUniform1f(useProg->getUniform("shininess"), materials[count % materials.size()]->shininess);
				MV->pushMatrix();

				float shiftedTime = glfwGetTime() + objPhaseShifts[i + j];
				float sFactor = 0.5f + 0.1f * sinf(shiftedTime);


				MV->translate(position);
				MV->rotate(objRotAngles[count], glm::vec3(0, 1, 0));
				MV->scale(glm::vec3(sFactor));
				if (currentShape == shape) {
					MV->translate(glm::vec3(0.0f, -0.335f, 0.0f));
				}
				else if (currentShape == teapot) {
					MV->translate(glm::vec3(0.0f, -0.005f, 0.0f));
				}
				glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
				glUniformMatrix3fv(useProg->getUniform("invTransformMV"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())))));
				currentShape->draw(useProg);
				MV->popMatrix();
				count++;
			}
		}

		MV->pushMatrix();
		MV->translate(glm::vec3(10.0, 10.0, 10.0));
		glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniform3f(useProg->getUniform("ka"), 1.0, 1.0, 0.0);
		glUniform3f(useProg->getUniform("kd"), 0.0, 0.0, 0.0);
		glUniform3f(useProg->getUniform("ks"), 0.0, 0.0, 0.0);
		glUniform1f(useProg->getUniform("shininess"), 1.0);
		sphere->draw(useProg);
		MV->popMatrix();

		/////////////////////////////////////////////////////////////////
		


		MV->popMatrix();
		P->popMatrix();


	}



	useProg->unbind();


	GLSL::checkError(GET_FILE_LINE);

	if (OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}


int main(int argc, char** argv)
{
	if (argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");

	// Optional argument
	if (argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if (!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}









//https://www.turbosquid.com/3d-models/grass-3d-model-1615659
//By Shawn Frost