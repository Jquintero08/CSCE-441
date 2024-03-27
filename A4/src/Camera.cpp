#define _USE_MATH_DEFINES
#include <cmath> 
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "MatrixStack.h"

Camera::Camera() :
	aspect(1.0f),
	fovy((float)(45.0 * M_PI / 180.0)),
	minFovy((float)(4.0 * M_PI / 180.0)),
	maxFovy((float)(114.0 * M_PI / 180.0)),
	znear(0.1f),
	zfar(1000.0f),
	rotations(0.0, 0.0),
	translations(0.0f, 0.0f, -5.0f),
	rfactor(0.005f),
	tfactor(0.001f),
	sfactor(0.005f),
	position(glm::vec3(0.0f, 0.0f, 5.0f)),
	yaw(0.0f),
	pitch(0.0f)
{
}

Camera::~Camera()
{
}

void Camera::mouseClicked(float x, float y, bool shift, bool ctrl, bool alt)
{
	mousePrev.x = x;
	mousePrev.y = y;
	if (shift) {
		state = Camera::TRANSLATE;
	}
	else if (ctrl) {
		state = Camera::SCALE;
	}
	else {
		state = Camera::ROTATE;
	}
}

void Camera::mouseMoved(float x, float y)
{
	glm::vec2 mouseCurr(x, y);
	glm::vec2 dv = mouseCurr - mousePrev;

	if (state == Camera::ROTATE) {
		yaw -= rfactor * dv.x;
		pitch -= rfactor * dv.y;

		float maxPitch = ((float)(60.0 * M_PI / 180.0));
		pitch = std::max(-maxPitch, std::min(((float)(60.0 * M_PI / 180.0)), pitch));
	}

	mousePrev = mouseCurr;
}

void Camera::zoomIn() {
	fovy -= ((float)(1.0 * M_PI / 180.0));
	if (fovy < minFovy) {
		fovy = minFovy;
	}
}

void Camera::zoomOut() {
	fovy += ((float)(1.0 * M_PI / 180.0));
	if (fovy > maxFovy) {
		fovy = maxFovy;
	}
}

void Camera::moveForward(float delta)
{
	position += delta * glm::vec3(std::sin(yaw), 0.0f, -std::cos(yaw));;
}

void Camera::moveRight(float delta)
{
	glm::vec3 forward = glm::vec3(std::sin(yaw), 0.0f, -std::cos(yaw));
	glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
	position += delta * right;
}

void Camera::moveUp(float delta)
{
	position.y += delta;
}

void Camera::applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const
{
	// Modify provided MatrixStack
	P->multMatrix(glm::perspective(fovy, aspect, znear, zfar));
}

void Camera::applyViewMatrix(std::shared_ptr<MatrixStack> MV) const
{
	glm::vec3 forward = glm::vec3(std::sin(yaw), -std::sin(pitch), -std::cos(yaw));
	glm::vec3 target = position + forward;
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	MV->multMatrix(glm::lookAt(position, target, up));
}
