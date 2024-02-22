#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"


using namespace std;

GLFWwindow *window; // Main application window
string RES_DIR = ""; // Where data files live
shared_ptr<Program> prog;
shared_ptr<Program> progIM; // immediate mode
shared_ptr<Shape> shape;


#include <vector>
#include <memory>
#include <glm/glm.hpp>



class MatrixStack;


shared_ptr<Shape> jointSphere;


class Component {
public:
	glm::vec3 transJoint;
	glm::vec3 rotateJoint;
	glm::vec3 transMesh;
	glm::vec3 scale;
	shared_ptr<Shape> mesh;
	vector<shared_ptr<Component>> children;
	glm::vec3 rotationAnimation;

	Component(shared_ptr<Shape> mesh) :
		transJoint(0.0f),
		rotateJoint(0.0f),
		transMesh(0.0f),
		scale(1.0f),
		rotationAnimation(0.0f),
		mesh(mesh) {}

	void add_child(shared_ptr<Component> child) {
		children.push_back(child);
	}

	void Component::draw(shared_ptr<MatrixStack> MV) {
		MV->pushMatrix();

		MV->translate(transJoint);



		MV->rotate(rotateJoint.x, glm::vec3(1.0f, 0.0f, 0.0f));
		MV->rotate(rotateJoint.y, glm::vec3(0.0f, 1.0f, 0.0f));
		MV->rotate(rotateJoint.z, glm::vec3(0.0f, 0.0f, 1.0f));

		if (jointSphere) {
			MV->pushMatrix();
			MV->rotate(rotationAnimation.x, glm::vec3(1.0f, 0.0f, 0.0f)); //Animate the spheres with the actual part
			MV->rotate(rotationAnimation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			MV->rotate(rotationAnimation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			MV->scale(glm::vec3(0.7f, 0.7f, 0.7f)); //Size of the sphere on the joints
			glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
			jointSphere->draw(prog);
			MV->popMatrix();
		}

		MV->translate(transMesh);

		MV->scale(scale);


		MV->pushMatrix();
		MV->rotate(rotationAnimation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		MV->rotate(rotationAnimation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		MV->rotate(rotationAnimation.z, glm::vec3(0.0f, 0.0f, 1.0f));

		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		mesh->draw(prog);
		MV->popMatrix();

		for (const auto& child : children) {
			MV->pushMatrix();

			MV->scale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z); //Children will not inherit size (took 3 hours to figure out this problem -_-)

			child->draw(MV);

			MV->popMatrix();
		}

		MV->popMatrix();
	}
};

class Robot {
public:
	shared_ptr<Component> torso;
	shared_ptr<Component> head;
	shared_ptr<Component> upperLeftArm;
	shared_ptr<Component> lowerLeftArm;
	shared_ptr<Component> upperRightArm;
	shared_ptr<Component> lowerRightArm;
	shared_ptr<Component> upperLeftLeg;
	shared_ptr<Component> lowerLeftLeg;
	shared_ptr<Component> upperRightLeg;
	shared_ptr<Component> lowerRightLeg;
	vector<shared_ptr<Component>> componentList;
	shared_ptr<Component> selectedComp;


	Robot(shared_ptr<Shape> shape) {
		torso = make_shared<Component>(shape);
		head = make_shared<Component>(shape);
		upperLeftArm = make_shared<Component>(shape);
		lowerLeftArm = make_shared<Component>(shape);
		upperRightArm = make_shared<Component>(shape);
		lowerRightArm = make_shared<Component>(shape);
		upperLeftLeg = make_shared<Component>(shape);
		lowerLeftLeg = make_shared<Component>(shape);
		upperRightLeg = make_shared<Component>(shape);
		lowerRightLeg = make_shared<Component>(shape);

		hierarchy_setup();
		transform_setup();
		selectedComp = torso;
	}
	void populate_component_list() {
		componentList.clear();
		componentList.push_back(torso);
		componentList.push_back(head);
		componentList.push_back(upperLeftArm);
		componentList.push_back(lowerLeftArm);
		componentList.push_back(upperRightArm);
		componentList.push_back(lowerRightArm);
		componentList.push_back(upperLeftLeg);
		componentList.push_back(lowerLeftLeg);
		componentList.push_back(upperRightLeg);
		componentList.push_back(lowerRightLeg);
	}
	shared_ptr<Component> get_next_component(bool next = true) {
		static size_t currentIndex = 0;
		if (next) {
			currentIndex = (currentIndex + 1) % componentList.size();
		}
		else {
			if (currentIndex == 0) {
				currentIndex = componentList.size() - 1;
			}
			else {
				--currentIndex;
			}
		}
		return componentList[currentIndex];
	}
private:
	void hierarchy_setup() {
		torso->add_child(head);
		torso->add_child(upperLeftArm);
		torso->add_child(upperRightArm);
		torso->add_child(upperLeftLeg);
		torso->add_child(upperRightLeg);
		upperLeftArm->add_child(lowerLeftArm);
		upperRightArm->add_child(lowerRightArm);
		upperLeftLeg->add_child(lowerLeftLeg);
		upperRightLeg->add_child(lowerRightLeg);
	}

	void transform_setup() {
		//TORSO
		torso->transJoint = glm::vec3(0.0f, 0.0f, 0.0f);
		torso->rotateJoint = glm::vec3(0.1f, 0.0f, 0.0f); //Rotation
		torso->transMesh = glm::vec3(0.0f, 0.0f, 0.0f);
		torso->scale = glm::vec3(1.5f, 2.5f, 1.0f);

		//HEAD
		head->transJoint = glm::vec3(0.0f, 1.55f, 0.0f);
		head->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		head->transMesh = glm::vec3(0.0f, 0.0f, 0.0f);
		head->scale = glm::vec3(0.75f, 0.75f, 0.75f);

		//UPPER LEFT ARM
		upperLeftArm->transJoint = glm::vec3(-0.75f, 0.95f, 0.0f);
		upperLeftArm->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		upperLeftArm->transMesh = glm::vec3(-0.75f, 0.0f, 0.0f);
		upperLeftArm->scale = glm::vec3(1.5f, 0.5f, 0.5f);

		//LOWER LEFT ARM
		lowerLeftArm->transJoint = glm::vec3(-0.8f, 0.0f, 0.0f);
		lowerLeftArm->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		lowerLeftArm->transMesh = glm::vec3(-0.6f, 0.0f, 0.0f);
		lowerLeftArm->scale = glm::vec3(1.35f, 0.40f, 0.40f);

		//UPPER RIGHT ARM
		upperRightArm->transJoint = glm::vec3(0.75f, 0.95f, 0.0f);
		upperRightArm->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		upperRightArm->transMesh = glm::vec3(0.75f, 0.0f, 0.0f);
		upperRightArm->scale = glm::vec3(1.5f, 0.5f, 0.5f);

		//LOWER RIGHT ARM
		lowerRightArm->transJoint = glm::vec3(0.8f, 0.0f, 0.0f);
		lowerRightArm->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		lowerRightArm->transMesh = glm::vec3(0.6f, 0.0f, 0.0f);
		lowerRightArm->scale = glm::vec3(1.35f, 0.40f, 0.40f);

		//UPPER LEFT LEG
		upperLeftLeg->transJoint = glm::vec3(-0.35f, -1.25f, 0.0f);
		upperLeftLeg->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		upperLeftLeg->transMesh = glm::vec3(0.0f, -0.75f, 0.0f);
		upperLeftLeg->scale = glm::vec3(0.60f, 1.6f, 0.60f);

		//LOWER LEFT LEG
		lowerLeftLeg->transJoint = glm::vec3(0.0f, -0.75f, 0.0f);
		lowerLeftLeg->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		lowerLeftLeg->transMesh = glm::vec3(0.0f, -0.75f, 0.0f);
		lowerLeftLeg->scale = glm::vec3(0.5f, 1.5f, 0.50f);

		//UPPER RIGHT LEG
		upperRightLeg->transJoint = glm::vec3(0.35f, -1.25f, 0.0f);
		upperRightLeg->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		upperRightLeg->transMesh = glm::vec3(0.0f, -0.75f, 0.0f);
		upperRightLeg->scale = glm::vec3(0.60f, 1.6f, 0.60f);

		//LOWER RIGHT LEG
		lowerRightLeg->transJoint = glm::vec3(0.0f, -0.75f, 0.0f);
		lowerRightLeg->rotateJoint = glm::vec3(0.0f, 0.0f, 0.0f); //Rotation
		lowerRightLeg->transMesh = glm::vec3(0.0f, -0.75f, 0.0f);
		lowerRightLeg->scale = glm::vec3(0.5f, 1.5f, 0.50f);
	}
};

unique_ptr<Robot> robot;


static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_PERIOD: //Forward in hierarchy
			robot->selectedComp = robot->get_next_component(true);
			break;
		case GLFW_KEY_COMMA: //Backward in hierarchy
			robot->selectedComp = robot->get_next_component(false);
			break;
		case GLFW_KEY_X: //X-axis rotation
			if (mods & GLFW_MOD_SHIFT) {
				robot->selectedComp->rotateJoint.x -= 0.1;
			} else {
				robot->selectedComp->rotateJoint.x += 0.1;
			}
			break;
		case GLFW_KEY_Y: //Y-axis rotation
			if (mods & GLFW_MOD_SHIFT) {
				robot->selectedComp->rotateJoint.y -= 0.1;
			} else {
				robot->selectedComp->rotateJoint.y += 0.1;
			}
			break;
		case GLFW_KEY_Z: //Z-axis rotation
			if (mods & GLFW_MOD_SHIFT) {
				robot->selectedComp->rotateJoint.z -= 0.1;
			} else {
				robot->selectedComp->rotateJoint.z += 0.1;
			}
			break;
		case GLFW_KEY_ESCAPE:
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
				glfwSetWindowShouldClose(window, GL_TRUE);
			}
			break;
		}
	}
}



static void init()
{
	GLSL::checkVersion();

	// Check how many texture units are supported in the vertex shader
	int tmp;
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &tmp);
	cout << "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS = " << tmp << endl;
	// Check how many uniforms are supported in the vertex shader
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &tmp);
	cout << "GL_MAX_VERTEX_UNIFORM_COMPONENTS = " << tmp << endl;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &tmp);
	cout << "GL_MAX_VERTEX_ATTRIBS = " << tmp << endl;

	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);


	shape = make_shared<Shape>();
	shape->loadMesh(RES_DIR + "cube.obj");
	shape->init();


	jointSphere = make_shared<Shape>();
	jointSphere->loadMesh(RES_DIR + "sphere.obj");
	jointSphere->init();

	robot = make_unique<Robot>(shape);
	robot->populate_component_list();
	robot->selectedComp = robot->componentList[0];
	
	// Initialize the GLSL programs.
	prog = make_shared<Program>();
	prog->setVerbose(true);
	prog->setShaderNames(RES_DIR + "nor_vert.glsl", RES_DIR + "nor_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->setVerbose(false);
	
	progIM = make_shared<Program>();
	progIM->setVerbose(true);
	progIM->setShaderNames(RES_DIR + "simple_vert.glsl", RES_DIR + "simple_frag.glsl");
	progIM->init();

	progIM->addUniform("P");
	progIM->addUniform("MV");
	progIM->setVerbose(false);
	
	// If there were any OpenGL errors, this will print something.
	// You can intersperse this line in your code to find the exact location
	// of your OpenGL error.
	GLSL::checkError(GET_FILE_LINE);
}




static void render()
{
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspect = width / (float)height;
	glViewport(0, 0, width, height);

	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Create matrix stacks.
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	// Apply projection.
	P->pushMatrix();
	P->multMatrix(glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f));
	// Apply camera transform.
	MV->pushMatrix();
	MV->translate(glm::vec3(0.0f, 1.0f, -10.0f));


	double t = glfwGetTime();
	float a = 0.05f;
	float f = 2.0f;
	float scale_factor = 1 + (a/2) + ((a/2) * sin(2 * M_PI * f * t));
	float rotationAngle1 = 1.0f * t;
	float rotationAngle2 = 7.0f * t;
	//MV->rotate(t, 0.0, 1.0, 0.0); //Comment out if you don't want automatic rotation

	robot->upperLeftArm->rotationAnimation.x = rotationAngle1; //Comment out if you don't want continuous upper left arm rotation
	robot->lowerRightLeg->rotationAnimation.y = rotationAngle2; //Comment out if you don't want continuous lower right leg rotation


	if (robot->selectedComp) {
		glm::vec3 originalScale = robot->selectedComp->scale;
		robot->selectedComp->scale *= scale_factor;

		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		robot->torso->draw(MV);
		prog->unbind();

		robot->selectedComp->scale = originalScale;
	}
	else {
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		robot->torso->draw(MV);
		prog->unbind();
	}


	// Pop matrix stacks
	MV->popMatrix();
	P->popMatrix();

	GLSL::checkError(GET_FILE_LINE);
}


int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RES_DIR = argv[1] + string("/");

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// https://en.wikipedia.org/wiki/OpenGL
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(1280, 720, "YOUR NAME", NULL, NULL);
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
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		if(!glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
			// Render scene.
			render();
			// Swap front and back buffers.
			glfwSwapBuffers(window);
		}
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
