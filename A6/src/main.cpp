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
const double EPSILON = 1e-5;

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

	Vec3 operator/(const Vec3& v) const {
		return Vec3(x / v.x, y / v.y, z / v.z);
	}

};

Vec3 operator*(double scalar, const Vec3& v) {
	return Vec3(v.x * scalar, v.y * scalar, v.z * scalar);
}

struct Hit {
	double s;
	Vec3 x;
	Vec3 n;

	Hit(double s, Vec3 x, Vec3 n) : s(s), x(x), n(n) {}
};

class Shape {
public:
	Vec3 diffuse;
	Vec3 specular;
	Vec3 ambient;
	double exponent;
	double reflectiveness;

	Shape(const Vec3& diff, const Vec3& spec, const Vec3& ambi, double expo, double reflect)
		: diffuse(diff), specular(spec), ambient(ambi), exponent(expo), reflectiveness(reflect) {}

	virtual std::optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const = 0;

	virtual ~Shape() = default;
	virtual Vec3 normalAt(const Vec3& point) const = 0;
};

class Sphere : public Shape {
public:
	Vec3 position;
	Vec3 scale;
	Vec3 normalAt(const Vec3& point) const override {
		return (point - position).normalize();
	}

	Sphere(const Vec3& pos, const Vec3& sc, const Vec3& diff, const Vec3& spec, const Vec3& ambi, double expo, double reflect)
		: Shape(diff, spec, ambi, expo, reflect), position(pos), scale(sc) {}

	std::optional<Hit> Sphere::intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const override {
		Vec3 oc = rayOrigin - position;
		double a = rayDirect.dot(rayDirect);
		double b = 2.0 * oc.dot(rayDirect);
		double c = oc.dot(oc) - (scale.x * scale.x);
		double discriminant = b * b - 4 * a * c;
		if (discriminant < 0) {
			return std::nullopt;
		}
		double t = (-b - sqrt(discriminant)) / (2 * a);
		if (t < 0) {
			t = (-b + sqrt(discriminant)) / (2 * a);
			if (t < 0) return std::nullopt;
		}
		Vec3 hitPoint = rayOrigin + t * rayDirect;
		Vec3 normal = normalAt(hitPoint);
		double distance = t;
		return Hit(distance, hitPoint, normal);
	}

};

class Ellipsoid : public Shape {
public:
	Vec3 position;
	Vec3 scale;
	Vec3 normalAt(const Vec3& point) const override {
		Vec3 normalizedPoint = (point - position) / scale;
		return Vec3(normalizedPoint.x / scale.x, normalizedPoint.y / scale.y, normalizedPoint.z / scale.z).normalize();
	}

	Ellipsoid(const Vec3& pos, const Vec3& sc, const Vec3& diff, const Vec3& spec, const Vec3& ambi, double expo, double reflect)
		: Shape(diff, spec, ambi, expo, reflect), position(pos), scale(sc) {}

	std::optional<Hit> Ellipsoid::intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const override {
		Vec3 oc = (rayOrigin - position) / scale;
		Vec3 rd = rayDirect / scale;
		double a = rd.dot(rd);
		double b = 2.0 * oc.dot(rd);
		double c = oc.dot(oc) - 1.0;
		double discriminant = b * b - 4 * a * c;
		if (discriminant < 0) {
			return std::nullopt;
		}
		double t = (-b - sqrt(discriminant)) / (2 * a);
		if (t < 0) {
			t = (-b + sqrt(discriminant)) / (2 * a);
			if (t < 0) return std::nullopt;
		}
		Vec3 hitPoint = rayOrigin + t * rayDirect;
		Vec3 normal = normalAt(hitPoint);
		double distance = t;
		return Hit(distance, hitPoint, normal);
	}
};

class Plane : public Shape {
public:
	Vec3 position;
	Vec3 normal;
	Vec3 normalAt(const Vec3&) const override {
		return normal;
	}

	Plane(const Vec3& pos, const Vec3& norm, const Vec3& diff, const Vec3& spec, const Vec3& ambi, double expo, double reflect)
		: Shape(diff, spec, ambi, expo, reflect), position(pos), normal(norm) {}

	std::optional<Hit> Plane::intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const override {
		double denom = normal.dot(rayDirect);
		if (std::abs(denom) > EPSILON) {
			Vec3 p0l0 = position - rayOrigin;
			double t = p0l0.dot(normal) / denom;
			if (t >= 0) {
				Vec3 hitPoint = rayOrigin + t * rayDirect;
				Vec3 normalAtPoint = normal;
				return Hit(t, hitPoint, normalAtPoint);
			}
		}
		return std::nullopt;
	}


};

struct Light {
	Vec3 position;
	double intensity;

	Light(const Vec3& p, double i) : position(p), intensity(i) {}
};


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

double length(const Vec3& v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}



bool is_shadowed(const Vec3& point, const Vec3& lightDir, const std::vector<std::unique_ptr<Shape>>& shapes, const double lightDist) {
	for (const auto& shape : shapes) {
		auto shadowIntersect = shape->intersect(point + lightDir * EPSILON, lightDir);
		if (shadowIntersect) {
			Hit shadowHit = shadowIntersect.value();
			if (shadowHit.s < lightDist) {
				return true;
			}
		}
	}
	return false;
}


Vec3 trace_ray(const Vec3& rayOrigin, const Vec3& rayDirect, const vector<unique_ptr<Shape>>& shapes, const vector<Light>& lights, const Vec3& cameraPos, int scene, int depth) {
	if (depth >= 7) {  //Set to 2 for Scene 4, set to higher value for Scene 5
		return Vec3(0.0, 0.0, 0.0);
	}
	
	Vec3 pixColor(0.0, 0.0, 0.0);
	double minDist = numeric_limits<double>::infinity();

	for (const auto& shape : shapes) {
		auto hitOpt = shape->intersect(rayOrigin, rayDirect);

		if (hitOpt) {
			Hit hit = hitOpt.value();
			if (hit.s < minDist) {
				minDist = hit.s;
				Vec3 accumColor(0.0, 0.0, 0.0);

				for (const auto& light : lights) {
					Vec3 toLight = (light.position - hit.x).normalize();
					double lightDist = length(light.position - hit.x);

					if (!is_shadowed(hit.x, toLight, shapes, lightDist) || scene == 1) {
						accumColor = accumColor + blinnPhong(hit.n, hit.x, light, shape->diffuse, shape->specular, shape->ambient, shape->exponent, cameraPos);
					}
					else {
						accumColor = accumColor + shape->ambient;
					}


				}
				pixColor = accumColor;
				if ((scene == 4 || scene == 5) && shape->reflectiveness > 0) {
					Vec3 reflectDirect = rayDirect - 2 * rayDirect.dot(hit.n) * hit.n;
					Vec3 reflectOrigin = hit.x + EPSILON * reflectDirect;
					Vec3 reflectedColor = trace_ray(reflectOrigin, reflectDirect, shapes, lights, reflectOrigin, scene, depth + 1);
					pixColor = (1 - shape->reflectiveness) * pixColor + shape->reflectiveness * reflectedColor;
				}

			}
		}
	}
	return pixColor;
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

	std::vector<std::unique_ptr<Shape>> shapes;
	std::vector<Light> lights;

	if (scene == 1 || scene == 2) {
		shapes.push_back(std::make_unique<Sphere>(Vec3(-0.5, -1.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Sphere
		shapes.push_back(std::make_unique<Sphere>(Vec3(0.5, -1.0, -1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Green Sphere
		shapes.push_back(std::make_unique<Sphere>(Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Sphere
		lights.push_back(Light(Vec3(-2.0, 1.0, 1.0), 1.0));
	}
	else if (scene == 3) {
		lights.push_back(Light(Vec3(1.0, 2.0, 2.0), 0.5));
		lights.push_back(Light(Vec3(-1.0, 2.0, -1.0), 0.5));
		

		shapes.push_back(std::make_unique<Ellipsoid>(Vec3(0.5, 0.0, 0.5), Vec3(0.5, 0.6, 0.2), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Ellipse
		shapes.push_back(std::make_unique<Sphere>(Vec3(-0.5, 0.0, -0.5), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Green Sphere
		shapes.push_back(std::make_unique<Plane>(Vec3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Flat Plane
	}
	else if (scene == 4 || scene == 5) {
		lights.push_back(Light(Vec3(-1.0, 2.0, 1.0), 0.5));
		lights.push_back(Light(Vec3(0.5, -0.5, 0.0), 0.5));
		shapes.push_back(std::make_unique<Sphere>(Vec3(0.5, -0.7, 0.5), Vec3(0.3, 0.3, 0.3), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Sphere
		shapes.push_back(std::make_unique<Sphere>(Vec3(1.0, -0.7, 0.0), Vec3(0.3, 0.3, 0.3), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Sphere
		shapes.push_back(std::make_unique<Plane>(Vec3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Floor
		shapes.push_back(std::make_unique<Plane>(Vec3(0.0, 0.0, -3.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Back Wall
		shapes.push_back(std::make_unique<Sphere>(Vec3(-0.5, 0.0, -0.5), Vec3(1.0, 1.0, 1.0), Vec3(), Vec3(), Vec3(), 0, 1.0)); //Reflective Sphere 1
		shapes.push_back(std::make_unique<Sphere>(Vec3(1.5, 0.0, -1.5), Vec3(1.0, 1.0, 1.0), Vec3(), Vec3(), Vec3(), 0, 1.0)); //Reflective Sphere 2
	}


	for (int y = 0; y < imageSize; ++y) {
		for (int x = 0; x < imageSize; ++x) {
			Vec3 rayDirect = rays[y * imageSize + x];
			Vec3 pixColor = trace_ray(cameraPos, rayDirect, shapes, lights, cameraPos, scene, 0);

			image.setPixel(x, y, static_cast<unsigned char>(min(max(pixColor.x, 0.0), 1.0) * 255),
				static_cast<unsigned char>(min(max(pixColor.y, 0.0), 1.0) * 255),
				static_cast<unsigned char>(min(max(pixColor.z, 0.0), 1.0) * 255));
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
