#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

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
#include "Material.h"
#include "Light.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;

shared_ptr<Shape> teapot;
shared_ptr<Shape> shape;

shared_ptr<Program> prog;
shared_ptr<Program> bPhShader;
shared_ptr<Program> silhouetteShader;
shared_ptr<Program> celShader;

vector<shared_ptr<Material>> materials;
vector<shared_ptr<Light>> lights;

int currMaterial = 0;
int currShader = 0;
int currLight = 0;


bool keyToggles[256] = {false}; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		case GLFW_KEY_S:
			currShader = (currShader + 1) % 4;
			break;
		case GLFW_KEY_M:
			currMaterial = (currMaterial + 1) % materials.size();
			break;
		case GLFW_KEY_L:
			currLight = (currLight + 1) % lights.size();
			break;
		case GLFW_KEY_X:
			if (mods & GLFW_MOD_SHIFT) {
				lights[currLight]->position.x += 0.1;
			}
			else {
				lights[currLight]->position.x -= 0.1;
			}
			break;
		case GLFW_KEY_Y:
			if (mods & GLFW_MOD_SHIFT) {
				lights[currLight]->position.y += 0.1;
			}
			else {
				lights[currLight]->position.y -= 0.1;
			}
			break;
		}
	}
}

// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
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

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w)
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
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
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

	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "normal_vert.glsl", RESOURCE_DIR + "normal_frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addUniform("MV");
	prog->addUniform("P");
	prog->setVerbose(false);

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
	bPhShader->addUniform("lightPositions");
	bPhShader->addUniform("lightColors");
	bPhShader->setVerbose(false);

	silhouetteShader = make_shared<Program>();
	silhouetteShader->setShaderNames(RESOURCE_DIR + "shaders_vert.glsl", RESOURCE_DIR + "silhouette_frag.glsl");
	silhouetteShader->setVerbose(true);
	silhouetteShader->init();
	silhouetteShader->addAttribute("aPos");
	silhouetteShader->addAttribute("aNor");
	silhouetteShader->addUniform("MV");
	silhouetteShader->addUniform("P");
	silhouetteShader->addUniform("invTransformMV");
	silhouetteShader->addUniform("outlineColor");
	silhouetteShader->addUniform("outlineWidth");
	silhouetteShader->setVerbose(false);

	celShader = make_shared<Program>();
	celShader->setShaderNames(RESOURCE_DIR + "shaders_vert.glsl", RESOURCE_DIR + "cel_frag.glsl");
	celShader->setVerbose(true);
	celShader->init();
	celShader->addAttribute("aPos");
	celShader->addAttribute("aNor");
	celShader->addUniform("MV");
	celShader->addUniform("P");
	celShader->addUniform("invTransformMV");
	celShader->addUniform("ka");
	celShader->addUniform("kd");
	celShader->addUniform("ks");
	celShader->addUniform("shininess");
	celShader->addUniform("lightPositions");
	celShader->addUniform("lightColors");
	celShader->setVerbose(false);

	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation

	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();

	teapot = make_shared<Shape>();
	teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
	teapot->init();


	materials.push_back(make_shared<Material>(glm::vec3(0.2, 0.2, 0.2), glm::vec3(0.8, 0.7, 0.7), glm::vec3(1.0, 0.9, 1.0), 200.0f)); //Mat 1
	materials.push_back(make_shared<Material>(glm::vec3(0.1, 0.1, 0.1), glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.5, 0.5, 0.7), 150.0f)); //Mat 2
	materials.push_back(make_shared<Material>(glm::vec3(0.13, 0.13, 0.13), glm::vec3(0.2, 0.2, 0.25), glm::vec3(0.2, 0.2, 0.5), 10.0f)); //Mat 3

	lights.push_back(make_shared<Light>(glm::vec3(1.0, 1.0, 1.0), glm::vec3(0.8, 0.8, 0.8))); //Light 1
	lights.push_back(make_shared<Light>(glm::vec3(-1.0, 1.0, 1.0), glm::vec3(0.2, 0.2, 0.0))); //Light 2


	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	}
	else {
		glDisable(GL_CULL_FACE);
	}
	if (keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width / (float)height);

	double t = glfwGetTime();
	if (!keyToggles[(unsigned)' ']) {
		// Spacebar turns animation on/off
		t = 0.0f;
	}

	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();

	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);


	shared_ptr<Program> useProg;
	if (currShader == 0) {
		useProg = prog;
	}
	else if (currShader == 1) {
		useProg = bPhShader;
	}
	else if (currShader == 2) {
		useProg = silhouetteShader;
	}
	else if (currShader == 3) {
		useProg = celShader;
	}

	useProg->bind();

	glUniformMatrix4fv(useProg->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));


	for (int i = 0; i < lights.size(); ++i) {
		int lightPosUniformLoc = glGetUniformLocation(useProg->pid, ("lightPositions[" + std::to_string(i) + "]").c_str());
		int lightColorUniformLoc = glGetUniformLocation(useProg->pid, ("lightColors[" + std::to_string(i) + "]").c_str());

		glUniform3fv(lightPosUniformLoc, 1, glm::value_ptr(lights[i]->position));
		glUniform3fv(lightColorUniformLoc, 1, glm::value_ptr(lights[i]->color));
	}

	if (currShader == 1 || currShader == 3) {
		glUniform3fv(useProg->getUniform("ka"), 1, glm::value_ptr(materials[currMaterial]->ka));
		glUniform3fv(useProg->getUniform("kd"), 1, glm::value_ptr(materials[currMaterial]->kd));
		glUniform3fv(useProg->getUniform("ks"), 1, glm::value_ptr(materials[currMaterial]->ks));
		glUniform1f(useProg->getUniform("shininess"), materials[currMaterial]->shininess);

	}
	else if (currShader == 2) {
		glUniform3fv(useProg->getUniform("outlineColor"), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
		glUniform1f(useProg->getUniform("outlineWidth"), 0.3f);
	}

	//Bunny
	MV->pushMatrix();
	MV->translate(glm::vec3(0.0f, -0.5f, 0.0f)); //Task 1
	MV->translate(glm::vec3(-0.5f, 0.0f, 0.0f)); 
	MV->scale(0.5f); //Task 1
	MV->rotate(t, 0.0f, 1.0f, 0.0f);

	glm::mat3 invTransformMV = glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())));
	glUniformMatrix3fv(useProg->getUniform("invTransformMV"), 1, GL_FALSE, glm::value_ptr(invTransformMV));
	glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	shape->draw(useProg);
	MV->popMatrix();


	//Teapot
	MV->pushMatrix();
	MV->translate(glm::vec3(0.5f, 0.0f, 0.0f));

	glm::mat4 S(1.0f);
	S[0][1] = 0.5f * cos(t);
	MV->multMatrix(S);

	MV->scale(0.5f);
	MV->rotate(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	invTransformMV = glm::transpose(glm::inverse(glm::mat3(MV->topMatrix())));
	glUniformMatrix3fv(useProg->getUniform("invTransformMV"), 1, GL_FALSE, glm::value_ptr(invTransformMV));
	glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	teapot->draw(useProg);
	MV->popMatrix();


	useProg->unbind();

	MV->popMatrix();
	P->popMatrix();

	GLSL::checkError(GET_FILE_LINE);

	if (OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Optional argument
	if(argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
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
	while(!glfwWindowShouldClose(window)) {
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
