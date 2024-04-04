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
#include "Texture.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>



using namespace std;

GLFWwindow* window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;

shared_ptr<Shape> teapot;
shared_ptr<Shape> shape;

shared_ptr<Shape> ball;
shared_ptr<Shape> sphere;
shared_ptr<Shape> ground;
shared_ptr<Shape> frustum;

shared_ptr<Texture> texture0;
map<string, GLuint> bufIDs;
int indCount;

shared_ptr<Program> bPhShader;

vector<shared_ptr<Material>> materials;
vector<shared_ptr<Light>> lights;

//Vectors containing items for 100 objects
vector<int> objBunOrTea;
vector<float> objRotAngles;
vector<float> objScales;



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
	if (state == GLFW_PRESS) {
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
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	bPhShader = make_shared<Program>();
	bPhShader->setShaderNames(RESOURCE_DIR + "shaders_vert.glsl", RESOURCE_DIR + "blinnphong_frag.glsl");
	bPhShader->setVerbose(true);
	bPhShader->init();
	bPhShader->addAttribute("aPos");
	bPhShader->addAttribute("aNor");
	bPhShader->addAttribute("aTex");
	bPhShader->addUniform("MV");
	bPhShader->addUniform("P");
	bPhShader->addUniform("invTransformMV");
	bPhShader->addUniform("ka");
	bPhShader->addUniform("kd");
	bPhShader->addUniform("ks");
	bPhShader->addUniform("shininess");
	bPhShader->addUniform("texture0");
	for (int i = 0; i < 2; ++i) {
		string uniformName = "lightEnabled[" + std::to_string(i) + "]";
		bPhShader->addUniform(uniformName);
	}
	bPhShader->addUniform("numLights");
	for (int i = 0; i < 10; ++i) {
		bPhShader->addUniform("lights[" + std::to_string(i) + "].position");
		bPhShader->addUniform("lights[" + std::to_string(i) + "].color");
	}
	bPhShader->setVerbose(false);


	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation

	texture0 = make_shared<Texture>();
	texture0->setFilename(RESOURCE_DIR + "tamu.jpg");
	texture0->init();
	texture0->setUnit(0);
	texture0->setWrapModes(GL_REPEAT, GL_REPEAT);

	vector<float> posBuf;
	vector<float> norBuf;
	vector<float> texBuf;
	vector<unsigned int> indBuf;



	int gridN = 50;
	float radius = 0.5f;
	for (int i = 0; i <= gridN; ++i) {
		float theta = M_PI * (float)i / gridN;
		for (int j = 0; j <= gridN; ++j) {
			float phi = 2.0f * M_PI * (float)j / gridN;


			float x = radius * sin(theta) * sin(phi);
			float y = radius * cos(theta);
			float z = radius * sin(theta) * cos(phi);


			posBuf.push_back(x);
			posBuf.push_back(y);
			posBuf.push_back(z);

			norBuf.push_back(x);
			norBuf.push_back(y);
			norBuf.push_back(z);
			texBuf.push_back((float)j / gridN);
			texBuf.push_back(1.0f - (float)i / gridN);
		}
	}

	for (int i = 0; i < gridN; ++i) {
		for (int j = 0; j < gridN; ++j) {
			int row1 = i * (gridN + 1);
			int row2 = (i + 1) * (gridN + 1);
			//Triangle 1
			indBuf.push_back(row1 + j);
			indBuf.push_back(row2 + j);
			indBuf.push_back(row1 + j + 1);
			//Triangle 2
			indBuf.push_back(row1 + j + 1);
			indBuf.push_back(row2 + j);
			indBuf.push_back(row2 + j + 1);
		}
	}

	indCount = (int)indBuf.size();

	GLuint tmp[4];
	glGenBuffers(4, tmp);
	bufIDs["bPos"] = tmp[0];
	bufIDs["bNor"] = tmp[1];
	bufIDs["bTex"] = tmp[2];
	bufIDs["bInd"] = tmp[3];
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bPos"]);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bNor"]);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bTex"]);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size() * sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs["bInd"]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size() * sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	assert(norBuf.size() == posBuf.size());


	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();

	teapot = make_shared<Shape>();
	teapot->loadMesh(RESOURCE_DIR + "teapot.obj");
	teapot->init();


	sphere = make_shared<Shape>();
	sphere->loadMesh(RESOURCE_DIR + "sphere.obj");
	sphere->init();

	ball = make_shared<Shape>();
	ball->loadMesh(RESOURCE_DIR + "ball.obj");
	ball->init();

	ground = make_shared<Shape>();
	ground->loadMesh(RESOURCE_DIR + "square.obj");
	ground->init();

	frustum = make_shared<Shape>();
	frustum->loadMesh(RESOURCE_DIR + "frustum.obj");
	frustum->init();


	//Randomizing Objects (infinite error if done in render)
	std::random_device rd;
	std::mt19937 eng(rd());
	std::uniform_int_distribution<> distr(0, 2);
	objBunOrTea.resize(100);
	for (auto& choice : objBunOrTea) {
		choice = distr(eng);
	}

	//Randomizing Sizing
	std::uniform_real_distribution<> phaseDistr(0.4, 0.6);
	objScales.resize(100);
	for (auto& phase : objScales) {
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



	lights.push_back(make_shared<Light>(glm::vec3(1.0, 0.0, 1.0), glm::vec3(0.8, 0.8, 0.8)));
	lights.push_back(make_shared<Light>(glm::vec3(0.0, 0.0, -5.0), glm::vec3(0.7, 0.7, 0.7)));
	lights.push_back(make_shared<Light>(glm::vec3(-2.0, 0.0, 0.0), glm::vec3(0.7, 0.2, 0.9)));
	lights.push_back(make_shared<Light>(glm::vec3(3.0, 0.0, 2.5), glm::vec3(0.8, 1.2, 1.0)));
	lights.push_back(make_shared<Light>(glm::vec3(-2.0, 0.0, 4.0), glm::vec3(0.1, 0.1, 0.4)));
	lights.push_back(make_shared<Light>(glm::vec3(3.1, 0.0, -2.0), glm::vec3(0.1, 0.1, 0.0)));
	lights.push_back(make_shared<Light>(glm::vec3(-5.0, 0.0, -10.0), glm::vec3(0.0, 0.1, 0.2)));
	lights.push_back(make_shared<Light>(glm::vec3(1.0, 0.0, 2.75), glm::vec3(0.25, 0.0, 0.15)));
	lights.push_back(make_shared<Light>(glm::vec3(-4.0, 0.0, 1.0), glm::vec3(0.05, 0.05, 0.05)));
	lights.push_back(make_shared<Light>(glm::vec3(0.0, 0.0, 0.5), glm::vec3(0.15, 0.15, 0.05)));


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


	useProg->bind();




	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);

	MV->translate(glm::vec3(0.0f, -0.5f, 0.0f));



	/////////////////////////LIGHTS///////////////////////////////////////////////

	glUniform1i(glGetUniformLocation(useProg->pid, "numLights"), lights.size());
	for (int i = 0; i < lights.size(); ++i) {
		glUniform1i(glGetUniformLocation(useProg->pid, ("lightEnabled[" + std::to_string(i) + "]").c_str()), 1);

	
		glm::vec4 lightPosCamSpace = MV->topMatrix() * glm::vec4(lights[i]->position, 1.0);



		glUniform3fv(glGetUniformLocation(useProg->pid, ("lights[" + std::to_string(i) + "].position").c_str()), 1, glm::value_ptr(glm::vec3(lightPosCamSpace)));
		glUniform3fv(glGetUniformLocation(useProg->pid, ("lights[" + std::to_string(i) + "].color").c_str()), 1, glm::value_ptr(lights[i]->color));

	}


	glUniformMatrix4fv(useProg->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));

	for (int i = 0; i < lights.size(); ++i) {
		MV->pushMatrix();

		MV->translate(lights[i]->position);
		MV->scale(glm::vec3(0.1));


		glUniformMatrix4fv(useProg->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));

		glUniform3fv(useProg->getUniform("ke"), 1, glm::value_ptr(lights[i]->color));
		glUniform3f(useProg->getUniform("kd"), 0.0f, 0.0f, 0.0f);
		glUniform3f(useProg->getUniform("ks"), 0.0f, 0.0f, 0.0f);
		glUniform1f(useProg->getUniform("shininess"), 10.0f);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bPos"]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, bufIDs["bNor"]);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs["bInd"]);
		glDrawElements(GL_TRIANGLES, indCount, GL_UNSIGNED_INT, (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		MV->popMatrix();
	}


	//////////////////////////////////////////////////////////


	//FLAT SURFACE =============================================================


	MV->pushMatrix();

	MV->translate(glm::vec3(0, -0.5, -17));
	MV->rotate((float)(90.0 * M_PI / 180.0), glm::vec3(-1, 0, 0));
	MV->scale(glm::vec3(80.0, 80.0, 1.0));
	glUniform3f(useProg->getUniform("ke"), 0.0f, 0.0f, 0.0f);
	glUniform3f(useProg->getUniform("kd"), 0.1f, 0.6f, 0.1f);
	glUniform3f(useProg->getUniform("ks"), 1.0f, 1.0f, 1.0f);
	glUniform1f(useProg->getUniform("shininess"), 10.0f);

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
			else if (objBunOrTea[i * 10 + j] == 1) {
				currentShape = teapot;
			}
			else {
				currentShape = ball;
			}

			glm::vec3 position = gridOrigin + glm::vec3(spacing * j, 0.0f, spacing * i);


			glUniform3f(useProg->getUniform("ke"), 0.0f, 0.0f, 0.0f);
			//glUniform3fv(useProg->getUniform("ka"), 1, glm::value_ptr(materials[count % materials.size()]->ka));
			glUniform3fv(useProg->getUniform("kd"), 1, glm::value_ptr(materials[count % materials.size()]->kd));
			glUniform3f(useProg->getUniform("ks"), 1.0f, 1.0f, 1.0f);
			glUniform1f(useProg->getUniform("shininess"), 10.0f);
			MV->pushMatrix();

			float scale = objScales[i * 10 + j];




			if (currentShape == shape) { //Bunny
				MV->translate(position);
				MV->rotate(objRotAngles[count], glm::vec3(0, 1, 0));
				MV->scale(glm::vec3(scale));
				MV->translate(glm::vec3(0.0f, -0.335f, 0.0f));
				MV->rotate(t + objRotAngles[count], glm::vec3(0, 1, 0));
			}
			else if (currentShape == teapot) { //Teapot
				MV->translate(position);
				MV->rotate(objRotAngles[count], glm::vec3(0, 1, 0));
				MV->scale(glm::vec3(scale));
				glm::mat4 shearMatrix = glm::mat4(1.0f);
				float shearAmount = sin(t) * 1.0f;
				shearMatrix[1][0] = shearAmount;
				MV->multMatrix(shearMatrix);
				MV->translate(glm::vec3(0.0f, -0.005f, 0.0f));
			}
			else if (currentShape == ball) { //Ball
				float bounceHeight = 1.0;
				float bounceSpeed = 1.3;
				float offset = (i + j) * 3.0f;
				float currentTime = static_cast<float>(t) + offset;

				float bounce = bounceHeight - (abs(sin(currentTime * bounceSpeed)) * bounceHeight);

				float phaseAdjustment = M_PI / 2;
				float adjustedTime = currentTime * bounceSpeed + phaseAdjustment;
				float squashControlFactor = (pow(sin(adjustedTime), 2) * 3);

				float squashAmount = abs(sin(adjustedTime));
				float squashFactor = 1.0 - squashControlFactor * 0.15;
				float stretchFactor = 1.0 + squashControlFactor * 0.7;

				glm::vec3 squashStretchScale = glm::vec3((scale*0.5) * 0.4 * squashFactor, ((scale*0.5) * 0.2 * stretchFactor), (scale*0.5) * 0.4 * squashFactor);

				MV->translate(position);
				MV->translate(glm::vec3(0.0f, 0.09f, 0.0f));
				MV->translate(glm::vec3(0, bounce, 0));
				MV->scale(squashStretchScale);
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