#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>

#include <cstdlib>  // For rand() and srand()
#include <ctime>    // For time()

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


#include "Image.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;




double RANDOM_COLORS[7][3] = {
	{0.0000,    0.4470,    0.7410},
	{0.8500,    0.3250,    0.0980},
	{0.9290,    0.6940,    0.1250},
	{0.4940,    0.1840,    0.5560},
	{0.4660,    0.6740,    0.1880},
	{0.3010,    0.7450,    0.9330},
	{0.6350,    0.0780,    0.1840},
};

struct Vertex {
	float x, y, z;
	unsigned char r, g, b; 
	float normX, normY, normZ;
};

struct Triangle {
	Vertex vertices[3];
	float minX, maxX, minY, maxY;
};

void compute_bounding_box(const vector<Vertex>& vertices, float& minX, float& maxX, float& minY, float& maxY){
	minX = minY = numeric_limits<float>::max();
	maxX = maxY = numeric_limits<float>::lowest();

	for (const auto& v : vertices){
		minX = min(minX, v.x);
		maxX = max(maxX, v.x);
		minY = min(minY, v.y);
		maxY = max(maxY, v.y);
	}
}

void task_one(const vector<Vertex>& vertices, const string& outFName, int imageWidth, int imageHeight){
	float minX, maxX, minY, maxY;
	compute_bounding_box(vertices, minX, maxX, minY, maxY);

	float scaleX = imageWidth / (maxX - minX);
	float scaleY = imageHeight / (maxY - minY);
	float finalScale = min(scaleX, scaleY);
	float translationX = (imageWidth - finalScale * (minX + maxX)) / 2;
	float translationY = (imageHeight - finalScale * (minY + maxY)) / 2;

	Image image(imageWidth, imageHeight);

	for (size_t i = 0; i < vertices.size(); i += 3){
		Triangle tri;
		for (int j = 0; j < 3; ++j){
			tri.vertices[j].x = finalScale * vertices[i + j].x + translationX;
			tri.vertices[j].y = finalScale * vertices[i + j].y + translationY;
		}

		compute_bounding_box({ tri.vertices[0], tri.vertices[1], tri.vertices[2] }, tri.minX, tri.maxX, tri.minY, tri.maxY);

		int colorIndex = (i / 3) % 7;
		unsigned char r = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][0] * 255);
		unsigned char g = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][1] * 255);
		unsigned char b = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][2] * 255);

		for (int x = static_cast<int>(tri.minX); x <= tri.maxX; ++x){
			for (int y = static_cast<int>(tri.minY); y <= tri.maxY; ++y){
				if (x >= 0 && x < imageWidth && y >= 0 && y < imageHeight){
					unsigned char color = static_cast<unsigned char>((i / 3) % 256);
					image.setPixel(x, y, r, g, b);
				}
			}
		}
	}

	image.writeToFile(outFName);
}









bool baryc_triangle(float x, float y, const Vertex& v0, const Vertex& v1, const Vertex& v2){
	float det = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
	float a = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / det;
	float b = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / det;
	float c = 1.0f - a - b;

	const float EPSILON = 0.0001f;
	return a >= -EPSILON && b >= -EPSILON && c >= -EPSILON;
}

void task_two(const vector<Vertex>& vertices, const string& outFName, int imageWidth, int imageHeight){
	Image image(imageWidth, imageHeight);

	float minX, maxX, minY, maxY;
	compute_bounding_box(vertices, minX, maxX, minY, maxY);

	float scaleX = imageWidth / (maxX - minX);
	float scaleY = imageHeight / (maxY - minY);
	float finalScale = min(scaleX, scaleY);
	float translationX = (imageWidth - finalScale * (minX + maxX)) / 2;
	float translationY = (imageHeight - finalScale * (minY + maxY)) / 2;

	for (size_t i = 0; i < vertices.size(); i += 3){
		Triangle tri;
		for (int j = 0; j < 3; ++j){
			tri.vertices[j].x = finalScale * vertices[i + j].x + translationX;
			tri.vertices[j].y = finalScale * vertices[i + j].y + translationY;
		}

		compute_bounding_box({ tri.vertices[0], tri.vertices[1], tri.vertices[2] }, tri.minX, tri.maxX, tri.minY, tri.maxY);

		//Implement prof suggestion
		int boundBoxMinX = max(static_cast<int>(floor(tri.minX)), 0);
		int boundBoxMaxX = min(static_cast<int>(ceil(tri.maxX)), imageWidth - 1);
		int boundBoxMinY = max(static_cast<int>(floor(tri.minY)), 0);
		int boundBoxMaxY = min(static_cast<int>(ceil(tri.maxY)), imageHeight - 1);

		int colorIndex = (i / 3) % 7;
		unsigned char r = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][0] * 255);
		unsigned char g = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][1] * 255);
		unsigned char b = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][2] * 255);

		for (int y = boundBoxMinY; y <= boundBoxMaxY; ++y){
			for (int x = boundBoxMinX; x <= boundBoxMaxX; ++x){
				if (baryc_triangle(x, y, tri.vertices[0], tri.vertices[1], tri.vertices[2])){
					image.setPixel(x, y, r, g, b);
				}
			}
		}
	}

	image.writeToFile(outFName);
}











void baryc_triangle_task3(float x, float y, const Vertex& v0, const Vertex& v1, const Vertex& v2, float& a, float& b, float& c){
	float det = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
	a = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / det;
	b = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / det;
	c = 1.0f - a - b;
}

void task_three(vector<Vertex>& vertices, const string& outFName, int imageWidth, int imageHeight){
	Image image(imageWidth, imageHeight);

	for (auto& vertex : vertices){
		int colorIndex = rand() % 7;
		vertex.r = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][0] * 255);
		vertex.g = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][1] * 255);
		vertex.b = static_cast<unsigned char>(RANDOM_COLORS[colorIndex][2] * 255);
	}

	float minX, maxX, minY, maxY;
	compute_bounding_box(vertices, minX, maxX, minY, maxY);

	float scaleX = imageWidth / (maxX - minX);
	float scaleY = imageHeight / (maxY - minY);
	float finalScale = min(scaleX, scaleY);
	float translationX = (imageWidth - finalScale * (maxX + minX)) / 2;
	float translationY = (imageHeight - finalScale * (minY + maxY)) / 2;

	for (size_t i = 0; i < vertices.size(); i += 3){
		Triangle tri;
		for (int j = 0; j < 3; ++j){
			tri.vertices[j].x = finalScale * vertices[i + j].x + translationX;
			tri.vertices[j].y = finalScale * vertices[i + j].y + translationY;
			tri.vertices[j].r = vertices[i + j].r;
			tri.vertices[j].g = vertices[i + j].g;
			tri.vertices[j].b = vertices[i + j].b;
		}

		compute_bounding_box({ tri.vertices[0], tri.vertices[1], tri.vertices[2] }, tri.minX, tri.maxX, tri.minY, tri.maxY);

		int boundBoxMinX = max(static_cast<int>(floor(tri.minX)), 0);
		int boundBoxMaxX = min(static_cast<int>(ceil(tri.maxX)), imageWidth - 1);
		int boundBoxMinY = max(static_cast<int>(floor(tri.minY)), 0);
		int boundBoxMaxY = min(static_cast<int>(ceil(tri.maxY)), imageHeight - 1);

		for (int y = boundBoxMinY; y <= boundBoxMaxY; ++y){
			for (int x = boundBoxMinX; x <= boundBoxMaxX; ++x){
				float alpha, beta, gamma;
				baryc_triangle_task3(x, y, tri.vertices[0], tri.vertices[1], tri.vertices[2], alpha, beta, gamma);

				if (alpha >= 0 && beta >= 0 && gamma >= 0){
					unsigned char r = static_cast<unsigned char>(alpha * tri.vertices[0].r + beta * tri.vertices[1].r + gamma * tri.vertices[2].r);
					unsigned char g = static_cast<unsigned char>(alpha * tri.vertices[0].g + beta * tri.vertices[1].g + gamma * tri.vertices[2].g);
					unsigned char b = static_cast<unsigned char>(alpha * tri.vertices[0].b + beta * tri.vertices[1].b + gamma * tri.vertices[2].b);

					if (x >= 0 && x < imageWidth && y >= 0 && y < imageHeight){
						image.setPixel(x, y, r, g, b);
					}
				}
			}
		}
	}

	image.writeToFile(outFName);
}



void task_four(vector<Vertex>& vertices, const string& outFName, int imageWidth, int imageHeight){
	Image image(imageWidth, imageHeight);

	float minX, maxX, minY, maxY;
	compute_bounding_box(vertices, minX, maxX, minY, maxY);

	for (auto& vertex : vertices){
		float lerpFactor = (vertex.y - minY) / (maxY - minY);
		vertex.r = static_cast<unsigned char>(255 * lerpFactor);
		vertex.b = static_cast<unsigned char>(255 * (1 - lerpFactor));
		vertex.g = 0;
	}

	float scaleX = imageWidth / (maxX - minX);
	float scaleY = imageHeight / (maxY - minY);
	float finalScale = min(scaleX, scaleY);
	float translationX = (imageWidth - finalScale * (maxX + minX)) / 2;
	float translationY = (imageHeight - finalScale * (minY + maxY)) / 2;

	for (size_t i = 0; i < vertices.size(); i += 3){
		Triangle tri;
		for (int j = 0; j < 3; ++j){
			tri.vertices[j].x = finalScale * vertices[i + j].x + translationX;
			tri.vertices[j].y = finalScale * vertices[i + j].y + translationY;
			tri.vertices[j].r = vertices[i + j].r;
			tri.vertices[j].g = vertices[i + j].g;
			tri.vertices[j].b = vertices[i + j].b;
		}

		compute_bounding_box({ tri.vertices[0], tri.vertices[1], tri.vertices[2] }, tri.minX, tri.maxX, tri.minY, tri.maxY);

		int boundBoxMinX = max(static_cast<int>(floor(tri.minX)), 0);
		int boundBoxMaxX = min(static_cast<int>(ceil(tri.maxX)), imageWidth - 1);
		int boundBoxMinY = max(static_cast<int>(floor(tri.minY)), 0);
		int boundBoxMaxY = min(static_cast<int>(ceil(tri.maxY)), imageHeight - 1);

		for (int y = boundBoxMinY; y <= boundBoxMaxY; ++y){
			for (int x = boundBoxMinX; x <= boundBoxMaxX; ++x){
				float alpha, beta, gamma;
				baryc_triangle_task3(x, y, tri.vertices[0], tri.vertices[1], tri.vertices[2], alpha, beta, gamma);

				if (alpha >= 0 && beta >= 0 && gamma >= 0){
					unsigned char r = static_cast<unsigned char>(alpha * tri.vertices[0].r + beta * tri.vertices[1].r + gamma * tri.vertices[2].r);
					unsigned char g = static_cast<unsigned char>(alpha * tri.vertices[0].g + beta * tri.vertices[1].g + gamma * tri.vertices[2].g);
					unsigned char b = static_cast<unsigned char>(alpha * tri.vertices[0].b + beta * tri.vertices[1].b + gamma * tri.vertices[2].b);

					image.setPixel(x, y, r, g, b);
				}
			}
		}
	}

	image.writeToFile(outFName);
}




void task_five(vector<Vertex>& vertices, const string& outFName, int imageWidth, int imageHeight) {
	Image image(imageWidth, imageHeight);
	vector<float> zBuf(imageWidth * imageHeight, numeric_limits<float>::lowest());

	float minZ = numeric_limits<float>::max();
	float maxZ = numeric_limits<float>::lowest();
	for (const auto& vertex : vertices) {
		minZ = min(minZ, vertex.z);
		maxZ = max(maxZ, vertex.z);
	}

	float minX, maxX, minY, maxY;
	compute_bounding_box(vertices, minX, maxX, minY, maxY);

	float scaleX = imageWidth / (maxX - minX);
	float scaleY = imageHeight / (maxY - minY);
	float finalScale = min(scaleX, scaleY);
	float translationX = (imageWidth - finalScale * (maxX + minX)) / 2;
	float translationY = (imageHeight - finalScale * (minY + maxY)) / 2;

	for (size_t i = 0; i < vertices.size(); i += 3) {
		Triangle tri;
		for (int j = 0; j < 3; ++j) {
			tri.vertices[j].x = finalScale * vertices[i + j].x + translationX;
			tri.vertices[j].y = finalScale * vertices[i + j].y + translationY;
			tri.vertices[j].z = vertices[i + j].z;
		}

		compute_bounding_box({ tri.vertices[0], tri.vertices[1], tri.vertices[2] }, tri.minX, tri.maxX, tri.minY, tri.maxY);

		int boundBoxMinX = max(static_cast<int>(floor(tri.minX)), 0);
		int boundBoxMaxX = min(static_cast<int>(ceil(tri.maxX)), imageWidth - 1);
		int boundBoxMinY = max(static_cast<int>(floor(tri.minY)), 0);
		int boundBoxMaxY = min(static_cast<int>(ceil(tri.maxY)), imageHeight - 1);

		for (int y = boundBoxMinY; y <= boundBoxMaxY; ++y) {
			for (int x = boundBoxMinX; x <= boundBoxMaxX; ++x) {
				float alpha, beta, gamma;
				baryc_triangle_task3(x, y, tri.vertices[0], tri.vertices[1], tri.vertices[2], alpha, beta, gamma);

				if (alpha >= 0 && beta >= 0 && gamma >= 0) {
					float zPixel = alpha * tri.vertices[0].z + beta * tri.vertices[1].z + gamma * tri.vertices[2].z;
					int zi = y * imageWidth + x;

					if (zPixel > zBuf[zi]) {
						unsigned char redVal = static_cast<unsigned char>((zPixel - minZ) / (maxZ - minZ) * 255);
						image.setPixel(x, y, redVal, 0, 0);
						zBuf[zi] = zPixel;
					}
				}
			}
		}
	}

	image.writeToFile(outFName);
}





void task_six(vector<Vertex>& vertices, const string& outFName, int imageWidth, int imageHeight, const vector<float>& norBuf) {
	Image image(imageWidth, imageHeight);
	vector<float> zBuf(imageWidth * imageHeight, numeric_limits<float>::lowest()); 

	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].normX = norBuf[3 * i];
		vertices[i].normY = norBuf[3 * i + 1];
		vertices[i].normZ = norBuf[3 * i + 2];
	}

	float minX, maxX, minY, maxY;
	compute_bounding_box(vertices, minX, maxX, minY, maxY);

	float scaleX = imageWidth / (maxX - minX);
	float scaleY = imageHeight / (maxY - minY);
	float finalScale = min(scaleX, scaleY);
	float translationX = (imageWidth - finalScale * (maxX + minX)) / 2;
	float translationY = (imageHeight - finalScale * (minY + maxY)) / 2;

	for (size_t i = 0; i < vertices.size(); i += 3) {
		Triangle tri;
		for (int j = 0; j < 3; ++j) {
			tri.vertices[j].x = finalScale * vertices[i + j].x + translationX;
			tri.vertices[j].y = finalScale * vertices[i + j].y + translationY;
			tri.vertices[j].z = vertices[i + j].z;
			tri.vertices[j].normX = vertices[i + j].normX;
			tri.vertices[j].normY = vertices[i + j].normY;
			tri.vertices[j].normZ = vertices[i + j].normZ;
		}

		compute_bounding_box({ tri.vertices[0], tri.vertices[1], tri.vertices[2] }, tri.minX, tri.maxX, tri.minY, tri.maxY);

		int boundBoxMinX = max(static_cast<int>(floor(tri.minX)), 0);
		int boundBoxMaxX = min(static_cast<int>(ceil(tri.maxX)), imageWidth - 1);
		int boundBoxMinY = max(static_cast<int>(floor(tri.minY)), 0);
		int boundBoxMaxY = min(static_cast<int>(ceil(tri.maxY)), imageHeight - 1);

		for (int y = boundBoxMinY; y <= boundBoxMaxY; ++y) {
			for (int x = boundBoxMinX; x <= boundBoxMaxX; ++x) {
				float alpha, beta, gamma;
				baryc_triangle_task3(x, y, tri.vertices[0], tri.vertices[1], tri.vertices[2], alpha, beta, gamma);

				if (alpha >= 0 && beta >= 0 && gamma >= 0) {
					float normX = alpha * tri.vertices[0].normX + beta * tri.vertices[1].normX + gamma * tri.vertices[2].normX;
					float normY = alpha * tri.vertices[0].normY + beta * tri.vertices[1].normY + gamma * tri.vertices[2].normY;
					float normZ = alpha * tri.vertices[0].normZ + beta * tri.vertices[1].normZ + gamma * tri.vertices[2].normZ;

					float zPixel = alpha * tri.vertices[0].z + beta * tri.vertices[1].z + gamma * tri.vertices[2].z;
					int zi = y * imageWidth + x;

					if (zPixel > zBuf[zi]) {
						unsigned char r = static_cast<unsigned char>((normX * 0.5f + 0.5f) * 255);
						unsigned char g = static_cast<unsigned char>((normY * 0.5f + 0.5f) * 255);
						unsigned char b = static_cast<unsigned char>((normZ * 0.5f + 0.5f) * 255);

						image.setPixel(x, y, r, g, b);
						zBuf[zi] = zPixel;
					}
				}
			}
		}
	}

	image.writeToFile(outFName);
}








void task_seven(vector<Vertex>& vertices, const string& outFName, int imageWidth, int imageHeight, const vector<float>& norBuf) {
	Image image(imageWidth, imageHeight);
	vector<float> zBuf(imageWidth * imageHeight, numeric_limits<float>::lowest());

	const float light[3] = { 1 / sqrt(3), 1 / sqrt(3), 1 / sqrt(3) };

	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].normX = norBuf[3 * i];
		vertices[i].normY = norBuf[3 * i + 1];
		vertices[i].normZ = norBuf[3 * i + 2];
	}

	float minX, maxX, minY, maxY;
	compute_bounding_box(vertices, minX, maxX, minY, maxY);

	float scaleX = imageWidth / (maxX - minX);
	float scaleY = imageHeight / (maxY - minY);
	float finalScale = min(scaleX, scaleY);
	float translationX = (imageWidth - finalScale * (maxX + minX)) / 2;
	float translationY = (imageHeight - finalScale * (minY + maxY)) / 2;

	for (size_t i = 0; i < vertices.size(); i += 3) {
		Triangle tri;
		for (int j = 0; j < 3; ++j) {
			tri.vertices[j].x = finalScale * vertices[i + j].x + translationX;
			tri.vertices[j].y = finalScale * vertices[i + j].y + translationY;
			tri.vertices[j].z = vertices[i + j].z;
			tri.vertices[j].normX = vertices[i + j].normX;
			tri.vertices[j].normY = vertices[i + j].normY;
			tri.vertices[j].normZ = vertices[i + j].normZ;
		}

		compute_bounding_box({ tri.vertices[0], tri.vertices[1], tri.vertices[2] }, tri.minX, tri.maxX, tri.minY, tri.maxY);

		int boundBoxMinX = max(static_cast<int>(floor(tri.minX)), 0);
		int boundBoxMaxX = min(static_cast<int>(ceil(tri.maxX)), imageWidth - 1);
		int boundBoxMinY = max(static_cast<int>(floor(tri.minY)), 0);
		int boundBoxMaxY = min(static_cast<int>(ceil(tri.maxY)), imageHeight - 1);

		for (int y = boundBoxMinY; y <= boundBoxMaxY; ++y) {
			for (int x = boundBoxMinX; x <= boundBoxMaxX; ++x) {
				float alpha, beta, gamma;
				baryc_triangle_task3(x, y, tri.vertices[0], tri.vertices[1], tri.vertices[2], alpha, beta, gamma);

				if (alpha >= 0 && beta >= 0 && gamma >= 0) {
					float normX = alpha * tri.vertices[0].normX + beta * tri.vertices[1].normX + gamma * tri.vertices[2].normX;
					float normY = alpha * tri.vertices[0].normY + beta * tri.vertices[1].normY + gamma * tri.vertices[2].normY;
					float normZ = alpha * tri.vertices[0].normZ + beta * tri.vertices[1].normZ + gamma * tri.vertices[2].normZ;

					float length = sqrt(normX * normX + normY * normY + normZ * normZ);
					normX /= length;
					normY /= length;
					normZ /= length;

					float dot = max(normX * light[0] + normY * light[1] + normZ * light[2], 0.0f);

					unsigned char cIntensity = static_cast<unsigned char>(dot * 255);

					int zIndex = y * imageWidth + x;
					float pixelZ = alpha * tri.vertices[0].z + beta * tri.vertices[1].z + gamma * tri.vertices[2].z;
					if (pixelZ > zBuf[zIndex]) {
						image.setPixel(x, y, cIntensity, cIntensity, cIntensity);
						zBuf[zIndex] = pixelZ;
					}
				}
			}
		}
	}

	image.writeToFile(outFName);
}






void task_eight(vector<Vertex>& vertices, const string& outFName, int imageWidth, int imageHeight, const vector<float>& norBuf) {
	Image image(imageWidth, imageHeight);
	vector<float> zBuf(imageWidth * imageHeight, numeric_limits<float>::lowest());

	const float light[3] = { 1 / sqrt(3), 1 / sqrt(3), 1 / sqrt(3) };
	const float theta = 3.141592653589 / 4; // π/4

	for (size_t i = 0; i < vertices.size(); i++) {
		float x = vertices[i].x;
		float z = vertices[i].z;
		vertices[i].x = cos(theta) * x + sin(theta) * z;
		vertices[i].z = -sin(theta) * x + cos(theta) * z;

		float normX = norBuf[3 * i];
		float normZ = norBuf[3 * i + 2];
		vertices[i].normX = cos(theta) * normX + sin(theta) * normZ;
		vertices[i].normY = norBuf[3 * i + 1];
		vertices[i].normZ = -sin(theta) * normX + cos(theta) * normZ;
	}

	float minX, maxX, minY, maxY;
	compute_bounding_box(vertices, minX, maxX, minY, maxY);

	float scaleX = imageWidth / (maxX - minX);
	float scaleY = imageHeight / (maxY - minY);
	float finalScale = min(scaleX, scaleY);
	float translationX = (imageWidth - finalScale * (maxX + minX)) / 2;
	float translationY = (imageHeight - finalScale * (minY + maxY)) / 2;

	for (size_t i = 0; i < vertices.size(); i += 3) {
		Triangle tri;
		for (int j = 0; j < 3; ++j) {
			tri.vertices[j].x = finalScale * vertices[i + j].x + translationX;
			tri.vertices[j].y = finalScale * vertices[i + j].y + translationY;
			tri.vertices[j].z = vertices[i + j].z;
			tri.vertices[j].normX = vertices[i + j].normX;
			tri.vertices[j].normY = vertices[i + j].normY;
			tri.vertices[j].normZ = vertices[i + j].normZ;
		}

		compute_bounding_box({ tri.vertices[0], tri.vertices[1], tri.vertices[2] }, tri.minX, tri.maxX, tri.minY, tri.maxY);

		int boundBoxMinX = max(static_cast<int>(floor(tri.minX)), 0);
		int boundBoxMaxX = min(static_cast<int>(ceil(tri.maxX)), imageWidth - 1);
		int boundBoxMinY = max(static_cast<int>(floor(tri.minY)), 0);
		int boundBoxMaxY = min(static_cast<int>(ceil(tri.maxY)), imageHeight - 1);

		for (int y = boundBoxMinY; y <= boundBoxMaxY; ++y) {
			for (int x = boundBoxMinX; x <= boundBoxMaxX; ++x) {
				float alpha, beta, gamma;
				baryc_triangle_task3(x, y, tri.vertices[0], tri.vertices[1], tri.vertices[2], alpha, beta, gamma);

				if (alpha >= 0 && beta >= 0 && gamma >= 0) {
					float normX = alpha * tri.vertices[0].normX + beta * tri.vertices[1].normX + gamma * tri.vertices[2].normX;
					float normY = alpha * tri.vertices[0].normY + beta * tri.vertices[1].normY + gamma * tri.vertices[2].normY;
					float normZ = alpha * tri.vertices[0].normZ + beta * tri.vertices[1].normZ + gamma * tri.vertices[2].normZ;

					float length = sqrt(normX * normX + normY * normY + normZ * normZ);
					normX /= length;
					normY /= length;
					normZ /= length;


					float dot = max(normX * light[0] + normY * light[1] + normZ * light[2], 0.0f);
					unsigned char cIntensity = static_cast<unsigned char>(dot * 255);


					int zIndex = y * imageWidth + x;
					float pixelZ = alpha * tri.vertices[0].z + beta * tri.vertices[1].z + gamma * tri.vertices[2].z;
					if (pixelZ > zBuf[zIndex]) {
						image.setPixel(x, y, cIntensity, cIntensity, cIntensity);
						zBuf[zIndex] = pixelZ;
					}
				}
			}
		}
	}

	image.writeToFile(outFName);
}









int main(int argc, char** argv) {
	if (argc < 6){
		cout << "Usage: ./A1 ../../resources/*object*.obj <output name> <x-axis> <y-axis> <Task #>" << endl;
		return 1;
	}
	string meshName = argv[1];
	string outFName = argv[2];
	int imageWidth = stoi(argv[3]);
	int imageHeight = stoi(argv[4]);
	int taskNumber = stoi(argv[5]);

	vector<float> posBuf; // List of vertex positions
	vector<float> norBuf; // List of vertex normals, not used in Task 1
	vector<float> texBuf; // List of vertex texture coords, not used in Task 1
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string errStr;

	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &errStr, meshName.c_str());
	if (!rc){
		cerr << errStr << endl;
		return 1;
	}

	// Some OBJ files have different indices for vertex positions, normals,
	// and texture coordinates. For example, a cube corner vertex may have
	// three different normals. Here, we are going to duplicate all such
	// vertices.
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = shapes[s].mesh.num_face_vertices[f];
			// Loop over faces (polygons)
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				posBuf.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
				posBuf.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
				posBuf.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
				if (!attrib.normals.empty()) {
					norBuf.push_back(attrib.normals[3 * idx.normal_index + 0]);
					norBuf.push_back(attrib.normals[3 * idx.normal_index + 1]);
					norBuf.push_back(attrib.normals[3 * idx.normal_index + 2]);
				}
				if (!attrib.texcoords.empty()) {
					texBuf.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
					texBuf.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
				}
			}
			index_offset += fv;
			// per-face material (IGNORE)
			shapes[s].mesh.material_ids[f];
		}
	}

	vector<Vertex> vertices;
	for (size_t i = 0; i < posBuf.size(); i += 3) {
		Vertex v;
		v.x = posBuf[i];
		v.y = posBuf[i + 1];
		v.z = posBuf[i + 2];
		v.normX = norBuf[i];
		v.normY = norBuf[i + 1];
		v.normZ = norBuf[i + 2];
		vertices.push_back(v);
	}

	if (taskNumber == 1){
		task_one(vertices, outFName, imageWidth, imageHeight);
	} else if (taskNumber == 2){
		task_two(vertices, outFName, imageWidth, imageHeight);
	} else if (taskNumber == 3){
		task_three(vertices, outFName, imageWidth, imageHeight);
	} else if (taskNumber == 4) {
		task_four(vertices, outFName, imageWidth, imageHeight);
	} else if (taskNumber == 5) {
		task_five(vertices, outFName, imageWidth, imageHeight);
	} else if (taskNumber == 6) {
		task_six(vertices, outFName, imageWidth, imageHeight, norBuf);
	} else if (taskNumber == 7) {
		task_seven(vertices, outFName, imageWidth, imageHeight, norBuf);
	} else if (taskNumber == 8) {
		task_eight(vertices, outFName, imageWidth, imageHeight, norBuf);
	}

	cout << "Number of vertices: " << posBuf.size() / 3 << endl;

	return 0;
}
