#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <optional>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Image.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;

class Vec3 {
public:
	double x, y, z;

	Vec3(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z) {}

	Vec3 operator-(const Vec3& v) const {
		return Vec3(x - v.x, y - v.y, z - v.z);
	}

	Vec3 operator-() const {
		return Vec3(-x, -y, -z);
	}

	Vec3 operator+(const Vec3& v) const {
		return Vec3(x + v.x, y + v.y, z + v.z);
	}

	Vec3 operator*(double scalar) const {
		return Vec3(x * scalar, y * scalar, z * scalar);
	}

	Vec3 operator*(const Vec3& v) const {
		return Vec3(x * v.x, y * v.y, z * v.z);
	}

	Vec3 normalize() const {
		double len = sqrt(x * x + y * y + z * z);
		return Vec3(x / len, y / len, z / len);
	}

	double dot(const Vec3& v) const {
		return x * v.x + y * v.y + z * v.z;
	}
};

Vec3 operator*(double scalar, const Vec3& v) {
	return Vec3(v.x * scalar, v.y * scalar, v.z * scalar);
}



struct Sphere {
	Vec3 position;
	Vec3 scale;
	Vec3 diffuse;
	Vec3 specular;
	Vec3 ambient;
	double exponent;

	Sphere(const Vec3& pos, const Vec3& sc, const Vec3& diff, const Vec3& spec, const Vec3& amb, double exp)
		: position(pos), scale(sc), diffuse(diff), specular(spec), ambient(amb), exponent(exp) {}
};

struct Light {
	Vec3 position;
	double intensity;

	Light(const Vec3& p, double i) : position(p), intensity(i) {}
};


std::optional<Vec3> ray_and_sphere_intersect(const Vec3& rayOrigin, const Vec3& rayDirect, const Sphere& sphere) {
	Vec3 oc = rayOrigin - sphere.position;
	double a = rayDirect.dot(rayDirect);
	double b = 2.0 * oc.dot(rayDirect);
	double c = oc.dot(oc) - (sphere.scale.x * sphere.scale.x);
	double discrim = b * b - 4 * a * c;

	if (discrim < 0) {
		return {};
	}

	double dist = (-b - sqrt(discrim)) / (2.0 * a);
	if (dist < 0) {
		dist = (-b + sqrt(discrim)) / (2.0 * a);
	}
	if (dist < 0) {
		return {};
	}

	return rayOrigin + rayDirect * dist;
}

Vec3 blinnPhong(const Vec3& normal, const Vec3& hitPoint, const Light& light,
				const Vec3& diffuseColor, const Vec3& specularColor,
				const Vec3& ambientColor, double specExponent, const Vec3& cameraPos) {
	Vec3 L = (light.position - hitPoint).normalize();
	Vec3 V = (cameraPos - hitPoint).normalize();
	Vec3 H = (L + V).normalize();
	Vec3 N = normal.normalize();

	Vec3 lightColor = light.intensity * Vec3(1.0, 1.0, 1.0);

	Vec3 ambient = ambientColor;
	Vec3 diffuse = diffuseColor * std::max(N.dot(L), 0.0);
	Vec3 specular = specularColor * pow(std::max(N.dot(H), 0.0), specExponent);


	return ambient + (diffuse + specular) * lightColor;
}





vector<Vec3> generate_rays(int width, int height, Vec3 cameraPos, double fovDegrees, double zPlane) {
	vector<Vec3> rays;
	double hHeight = tan((fovDegrees * (M_PI / 180.0)) / 2.0);
	double hWidth = hHeight;

	double pixHeight = 2.0 * hHeight / height;
	double pixWidth = 2.0 * hWidth / width;

	double yStart = hHeight - pixHeight / 2.0;
	double xStart = -hWidth + pixWidth / 2.0;

	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			double x = xStart + j * pixWidth;
			double y = -yStart + i * pixHeight;
			double z = zPlane - cameraPos.z;

			rays.push_back(Vec3(x, y, z).normalize());
		}
	}

	return rays;
}



const double EPSILON = 1e-5;

bool isShadowed(const Vec3& point, const Vec3& lightDir, const vector<Sphere>& spheres, const double lightDistance) {
	for (const auto& sphere : spheres) {
		auto shadowIntersect = ray_and_sphere_intersect(point + lightDir * EPSILON, lightDir, sphere);
		if (shadowIntersect) {
			Vec3 shadowPoint = shadowIntersect.value();
			double shadowDist = sqrt((shadowPoint - point).x * (shadowPoint - point).x +
				(shadowPoint - point).y * (shadowPoint - point).y +
				(shadowPoint - point).z * (shadowPoint - point).z);
			if (shadowDist < lightDistance) {
				return true;
			}
		}
	}
	return false;
}

// Function to compute the length of a vector
double length(const Vec3& v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}





int main(int argc, char **argv)
{
	if(argc < 4) {
		cout << "Usage: A6 <SCENE> <IMAGE SIZE> <IMAGE FILENAME>" << endl;
		return 0;
	}
	int scene = stoi(argv[1]);
	int imageSize = stoi(argv[2]);
	string imageFilename(argv[3]);



	/*
	// Load geometry
	vector<float> posBuf; // list of vertex positions
	vector<float> norBuf; // list of vertex normals
	vector<float> texBuf; // list of vertex texture coords
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string errStr;
	*/




	Image image(imageSize, imageSize);

	Vec3 cameraPos(0, 0, 5);
	double fov = 45.0;
	double zPlane = 4.0;
	vector<Vec3> rays = generate_rays(imageSize, imageSize, cameraPos, fov, zPlane);

	std::vector<Sphere> spheres;
	std::vector<Light> lights;

	if (scene == 1 || scene == 2) {
		spheres.push_back(Sphere(Vec3(-0.5, -1.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0));
		spheres.push_back(Sphere(Vec3(0.5, -1.0, -1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0));
		spheres.push_back(Sphere(Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0));
		lights.push_back(Light(Vec3(-2.0, 1.0, 1.0), 1.0));
	}
	else if (scene == 3) {
		std::cout << "Scene 3 setup is not implemented yet." << std::endl;
	}


	for (int y = 0; y < imageSize; ++y) {
		for (int x = 0; x < imageSize; ++x) {
			Vec3 rayDirect = rays[y * imageSize + x];
			Vec3 pixColor(0.0, 0.0, 0.0);
			double minDist = std::numeric_limits<double>::infinity();

			for (const auto& sphere : spheres) {
				auto hitPointOpt = ray_and_sphere_intersect(cameraPos, rayDirect, sphere);

				if (hitPointOpt) {
					Vec3 hitPoint = hitPointOpt.value();
					Vec3 N = (hitPoint - sphere.position).normalize();
					double distance = length(hitPoint - cameraPos);

					if (distance < minDist) {
						minDist = distance;
						Vec3 accumulatedColor(0.0, 0.0, 0.0);

						for (const auto& light : lights) {
							Vec3 toLight = (light.position - hitPoint).normalize();
							double lightDistance = length(light.position - hitPoint);

							if (!isShadowed(hitPoint, toLight, spheres, lightDistance) || scene == 1) {
								accumulatedColor = accumulatedColor + blinnPhong(N, hitPoint, light, sphere.diffuse, sphere.specular, sphere.ambient, sphere.exponent, cameraPos);
							}
							else {
								accumulatedColor = accumulatedColor + sphere.ambient * light.intensity;
							}
						}

						pixColor = accumulatedColor;
					}
				}
			}

			pixColor.x = std::min(std::max(pixColor.x, 0.0), 1.0);
			pixColor.y = std::min(std::max(pixColor.y, 0.0), 1.0);
			pixColor.z = std::min(std::max(pixColor.z, 0.0), 1.0);

			image.setPixel(x, y, static_cast<unsigned char>(pixColor.x * 255),
				static_cast<unsigned char>(pixColor.y * 255),
				static_cast<unsigned char>(pixColor.z * 255));
		}
	}





	/*
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		// Some OBJ files have different indices for vertex positions, normals,
		// and texture coordinates. For example, a cube corner vertex may have
		// three different normals. Here, we are going to duplicate all such
		// vertices.
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			size_t index_offset = 0;
			for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
				size_t fv = shapes[s].mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+0]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+1]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+2]);
					if(!attrib.normals.empty()) {
						norBuf.push_back(attrib.normals[3*idx.normal_index+0]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+1]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+2]);
					}
					if(!attrib.texcoords.empty()) {
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
					}
				}
				index_offset += fv;
				// per-face material (IGNORE)
				shapes[s].mesh.material_ids[f];
			}
		}
	}
	*/

	image.writeToFile(imageFilename);

	cout << "Rendered scene " << scene << " to " << imageFilename << " with size " << imageSize << "x" << imageSize << endl;
	
	return 0;
}
