#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <optional>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Image.h"

// This allows you to skip the `` in front of C++ standard library
// functions. You can also say `using cout` to be more selective.
// You should never do this in a header file.
using namespace std;
const double EPSILON = 1e-5;

const int AO_SAMPLES = 64;
const double AO_MAX_DIST = 2.0;

struct BoundingSphere {
	glm::vec3 center;
	float radius;
	bool valid;

	BoundingSphere() : center(glm::vec3(0)), radius(0), valid(false) {}
	BoundingSphere(const glm::vec3& c, float r) : center(c), radius(r), valid(true) {}

	bool intersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const {
		if (!valid) {
			return true;
		}
		glm::vec3 rayOriginToCenterVec = rayOrigin - center;
		float b = glm::dot(rayOriginToCenterVec, rayDirection);
		float c = glm::dot(rayOriginToCenterVec, rayOriginToCenterVec) - radius * radius;
		float discrim = b * b - c;
		return discrim > 0;
	}
};


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

Vec3 create_uniform_hemisphere_sample(const Vec3& normal) {
	double u = static_cast<double>(rand()) / RAND_MAX;
	double v = static_cast<double>(rand()) / RAND_MAX;
	double theta = 2 * M_PI * u;
	double phi = acos(2 * v - 1);
	double x = sin(phi) * cos(theta);
	double y = sin(phi) * sin(theta);
	double z = cos(phi);
	Vec3 randVec(x, y, z);
	if (randVec.dot(normal) < 0) {
		randVec = -randVec;
	}
	return randVec.normalize();
}


struct Hit {
	double s;
	Vec3 x;
	Vec3 n;
	glm::vec3 color;
	bool textured;

	Hit(double s, Vec3 x, Vec3 n, glm::vec3 color = glm::vec3(), bool textured = false) : s(s), x(x), n(n), color(color), textured(textured) {}
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

	virtual optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const = 0;

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

	optional<Hit> Sphere::intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const override {
		Vec3 rayOriginToCenterVec = rayOrigin - position;
		double a = rayDirect.dot(rayDirect);
		double b = 2.0 * rayOriginToCenterVec.dot(rayDirect);
		double c = rayOriginToCenterVec.dot(rayOriginToCenterVec) - (scale.x * scale.x);
		double discrim = b * b - 4 * a * c;
		if (discrim < 0) {
			return nullopt;
		}
		double t = (-b - sqrt(discrim)) / (2 * a);
		if (t < 0) {
			t = (-b + sqrt(discrim)) / (2 * a);
			if (t < 0) return nullopt;
		}
		Vec3 hitPoint = rayOrigin + t * rayDirect;
		Vec3 normal = normalAt(hitPoint);
		double distance = t;
		return Hit(distance, hitPoint, normal);
	}

};

class TexturedSphere : public Sphere {
public:
	Image texture;

	TexturedSphere(const Vec3& pos, const Vec3& sc, const Vec3& diff, const Vec3& spec, const Vec3& ambi, double expo, double reflect, const string& textureFile)
		: Sphere(pos, sc, diff, spec, ambi, expo, reflect), texture(textureFile) {}

	glm::vec3 getColorFromTexture(const Vec3& point) const {
		Vec3 localHit = (point - position).normalize();
		double u = 0.5 + atan2(localHit.z, localHit.x) / (2 * M_PI);
		double v = 0.5 - asin(localHit.y) / M_PI;
		return texture.getColorAt(u, v);
	}

	virtual optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const override {
		auto intersectResult = Sphere::intersect(rayOrigin, rayDirect);
		if (intersectResult) {
			Hit hit = intersectResult.value();
			glm::vec3 texColor = getColorFromTexture(hit.x);
			hit.color = texColor;
			hit.textured = true;
			return hit;
		}
		return nullopt;
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

	optional<Hit> Ellipsoid::intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const override {
		Vec3 rayOriginToCenterVec = (rayOrigin - position) / scale;
		Vec3 rayDirectionVec = rayDirect / scale;
		double a = rayDirectionVec.dot(rayDirectionVec);
		double b = 2.0 * rayOriginToCenterVec.dot(rayDirectionVec);
		double c = rayOriginToCenterVec.dot(rayOriginToCenterVec) - 1.0;
		double discrim = b * b - 4 * a * c;
		if (discrim < 0) {
			return nullopt;
		}
		double t = (-b - sqrt(discrim)) / (2 * a);
		if (t < 0) {
			t = (-b + sqrt(discrim)) / (2 * a);
			if (t < 0) return nullopt;
		}
		Vec3 hitPoint = rayOrigin + t * rayDirect;
		Vec3 normal = normalAt(hitPoint);
		return Hit(t, hitPoint, normal);
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

	optional<Hit> Plane::intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const override {
		double denom = normal.dot(rayDirect);
		if (abs(denom) > EPSILON) {
			Vec3 posRayOrigin = position - rayOrigin;
			double t = posRayOrigin.dot(normal) / denom;
			if (t >= 0) {
				Vec3 hitPoint = rayOrigin + t * rayDirect;
				Vec3 normalAtPoint = normal;
				return Hit(t, hitPoint, normalAtPoint);
			}
		}
		return nullopt;
	}


};

class Cube : public Shape { //Used ChatGPT 3.5 to get a cube Shape similar to how the other shapes were implemented... 
	//I do not take any credit for it. I only generated this to show off ambient occlusion. Please do add/subtract any points based on this shape  
public:
	Vec3 position;
	double size;

	// Constructor initializing the cube along with its material properties
	Cube(const Vec3& pos, double size, const Vec3& diff, const Vec3& spec, const Vec3& ambi, double expo, double reflect)
		: Shape(diff, spec, ambi, expo, reflect), position(pos), size(size) {}

	// Method to determine if a ray intersects with the cube
	optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDirect) const override {
		// Calculate the minimum and maximum bounds of the cube
		Vec3 minBound = position - Vec3(size / 2, size / 2, size / 2);
		Vec3 maxBound = position + Vec3(size / 2, size / 2, size / 2);

		// Intersection logic for the X-axis
		double tMin = (minBound.x - rayOrigin.x) / rayDirect.x;
		double tMax = (maxBound.x - rayOrigin.x) / rayDirect.x;
		if (tMin > tMax) swap(tMin, tMax);

		// Intersection logic for the Y-axis
		double tyMin = (minBound.y - rayOrigin.y) / rayDirect.y;
		double tyMax = (maxBound.y - rayOrigin.y) / rayDirect.y;
		if (tyMin > tyMax) swap(tyMin, tyMax);

		// Exit early if no valid intersection exists
		if ((tMin > tyMax) || (tyMin > tMax))
			return nullopt;

		// Update min and max t values for valid intersection intervals
		if (tyMin > tMin) tMin = tyMin;
		if (tyMax < tMax) tMax = tyMax;

		// Intersection logic for the Z-axis
		double tzMin = (minBound.z - rayOrigin.z) / rayDirect.z;
		double tzMax = (maxBound.z - rayOrigin.z) / rayDirect.z;
		if (tzMin > tzMax) swap(tzMin, tzMax);

		if ((tMin > tzMax) || (tzMin > tMax))
			return nullopt;

		if (tzMin > tMin) tMin = tzMin;
		if (tzMax < tMax) tMax = tzMax;

		// Ensure we only consider intersections in the positive ray direction
		if (tMin < 0 && tMax < 0)
			return nullopt;

		double t = tMin >= 0 ? tMin : tMax;
		if (t < 0)
			return nullopt;

		// Calculate the hit point and determine the normal at that point
		Vec3 hitPoint = rayOrigin + t * rayDirect;
		return Hit(t, hitPoint, normalAt(hitPoint));
	}

	// Calculate the normal at a given point on the cube's surface
	Vec3 normalAt(const Vec3& point) const override {
		Vec3 centerToPoint = point - position;
		Vec3 absVec = Vec3(fabs(centerToPoint.x), fabs(centerToPoint.y), fabs(centerToPoint.z));

		// Determine which face of the cube the point is on by comparing the components of the point to the cube's dimensions
		if (absVec.x > absVec.y && absVec.x > absVec.z)
			return Vec3((centerToPoint.x > 0) ? 1 : -1, 0, 0);
		else if (absVec.y > absVec.z)
			return Vec3(0, (centerToPoint.y > 0) ? 1 : -1, 0);
		else
			return Vec3(0, 0, (centerToPoint.z > 0) ? 1 : -1);
	}
};


bool intersect_triangle(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& vert0, const glm::vec3& vert1, const glm::vec3& vert2, double& t, double& u, double& v) {
	//I interpreted this code from raytri.c code @ https://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/raytri/
	glm::vec3 edge1 = vert1 - vert0;
	glm::vec3 edge2 = vert2 - vert0;
	glm::vec3 pvec = glm::cross(dir, edge2);

	double det = glm::dot(edge1, pvec);
	double inv_det = 1.0 / det;

	if (det > -EPSILON && det < EPSILON) {
		return false;
	}

	glm::vec3 tvec = orig - vert0;

	u = glm::dot(tvec, pvec) * inv_det;
	if (u < 0.0 || u > 1.0) {
		return false;
	}

	glm::vec3 qvec = glm::cross(tvec, edge1);
	v = glm::dot(dir, qvec) * inv_det;
	if (v < 0.0 || u + v > 1.0) {
		return false;
	}

	t = glm::dot(edge2, qvec) * inv_det;

	return t > EPSILON;
}


class Triangle : public Shape {
public:
	glm::vec3 vert0, vert1, vert2;
	glm::vec3 norm0, norm1, norm2;

	Triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
		const glm::vec3& n0, const glm::vec3& n1, const glm::vec3& n2,
		const Vec3& diff, const Vec3& spec, const Vec3& ambi, double expo)
		: Shape(diff, spec, ambi, expo, 0.0), vert0(v0), vert1(v1), vert2(v2),
		norm0(n0), norm1(n1), norm2(n2) {}

	optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDirection) const override {
		double t, u, v;
		bool intersect = intersect_triangle(glm::vec3(rayOrigin.x, rayOrigin.y, rayOrigin.z),
			glm::vec3(rayDirection.x, rayDirection.y, rayDirection.z), vert0, vert1, vert2, t, u, v);
		if (intersect && t > 0) {
			glm::vec3 interpNormal = (1.0f - static_cast<float>(u) - static_cast<float>(v)) * norm0 +
				static_cast<float>(u) * norm1 +
				static_cast<float>(v) * norm2;
			interpNormal = glm::normalize(interpNormal);

			glm::vec3 hitPoint = glm::vec3(rayOrigin.x, rayOrigin.y, rayOrigin.z) +
				static_cast<float>(t) * glm::vec3(rayDirection.x, rayDirection.y, rayDirection.z);

			return Hit(t, Vec3(hitPoint.x, hitPoint.y, hitPoint.z), Vec3(interpNormal.x, interpNormal.y, interpNormal.z));
		}
		return nullopt;
	}

	Vec3 normalAt(const Vec3& point) const override {
		return Vec3(0, 0, 0);
	}
};


struct Light {
	Vec3 position;
	double intensity;

	Light(const Vec3& p, double i) : position(p), intensity(i) {}
};


Vec3 blinnPhong(const Vec3& normal, const Vec3& hitPoint, const Light& light, const Vec3& diffuseColor, const Vec3& specularColor, const Vec3& ambientColor, double specExponent, const Vec3& cameraPos) {
	Vec3 L = (light.position - hitPoint).normalize();
	Vec3 V = (cameraPos - hitPoint).normalize();
	Vec3 H = (L + V).normalize();
	Vec3 N = normal.normalize();

	Vec3 lightColor = light.intensity * Vec3(1.0, 1.0, 1.0);

	Vec3 ambient = ambientColor;
	Vec3 diffuse = diffuseColor * max(N.dot(L), 0.0);
	Vec3 specular = specularColor * pow(max(N.dot(H), 0.0), specExponent);


	return ambient + (diffuse + specular) * lightColor;
}

vector<Vec3> create_rays(int width, int height, Vec3 cameraPos, double fovDegrees, double zPlane) {
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

vector<Vec3> create_rays_8(int width, int height, glm::vec3 position, glm::vec3 lookAt, glm::vec3 up, double fov, double zPlane) {
	vector<Vec3> rays;
	double aspectRatio = static_cast<double>(width) / height;
	double hHeight = tan(fov * M_PI / 360.0);
	double hWidth = aspectRatio * hHeight;

	double pixHeight = 2.0 * hHeight / height;
	double pixWidth = 2.0 * hWidth / width;

	double yStart = hHeight - pixHeight / 2.0;
	double xStart = -hWidth + pixWidth / 2.0;

	glm::mat4 viewMatrix = glm::inverse(glm::lookAt(position, lookAt, up));

	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			double x = xStart + j * pixWidth;
			double y = -yStart + i * pixHeight;
			glm::vec4 dir = viewMatrix * glm::vec4(x, y, -zPlane, 0.0);
			rays.push_back(Vec3(dir.x, dir.y, dir.z).normalize());
		}
	}
	return rays;
}

double length(const Vec3& v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}


bool is_shadowed(const Vec3& point, const Vec3& lightDir, const vector<unique_ptr<Shape>>& shapes, const double lightDist) {
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


double calculate_ambient_occlusion(const Vec3& hitPoint, const Vec3& normal, const vector<unique_ptr<Shape>>& shapes) {
	int occludedRays = 0;
	Vec3 sampRay;
	for (int i = 0; i < AO_SAMPLES; i++) {
		sampRay = create_uniform_hemisphere_sample(normal);
		if (is_shadowed(hitPoint, sampRay, shapes, AO_MAX_DIST)) {
			occludedRays++;
		}
	}
	return static_cast<double>(occludedRays) / AO_SAMPLES;
}


Vec3 trace_ray(const Vec3& rayOrigin, const Vec3& rayDirect, const vector<unique_ptr<Shape>>& shapes, const BoundingSphere& boundingSphere, const vector<Light>& lights, const Vec3& cameraPos, int scene, int depth) {
	if (depth >= 7) {  //Set to 2 for Scene 4, set to a higher value for Scene 5
		return Vec3(0.0, 0.0, 0.0);
	}

	if (!boundingSphere.intersect(glm::vec3(rayOrigin.x, rayOrigin.y, rayOrigin.z), glm::vec3(rayDirect.x, rayDirect.y, rayDirect.z))) {
		return Vec3(0.0, 0.0, 0.0); //If ray doesn't intersect the bounding sphere
	}

	Vec3 pixColor(0.0, 0.0, 0.0);
	double minDist = numeric_limits<double>::infinity();

	for (const auto& shape : shapes) {
		auto intersectResult = shape->intersect(rayOrigin, rayDirect);

		if (intersectResult) {
			Hit hit = intersectResult.value();
			if (hit.s < minDist) {
				minDist = hit.s;
				Vec3 accumColor(0.0, 0.0, 0.0);

				TexturedSphere* texturedSphere = dynamic_cast<TexturedSphere*>(shape.get());
				if (texturedSphere != nullptr) {
					glm::vec3 texColor = texturedSphere->getColorFromTexture(hit.x);
					Vec3 baseColor(texColor.r, texColor.g, texColor.b);

					for (const auto& light : lights) {
						Vec3 toLight = (light.position - hit.x).normalize();
						double lightDist = length(light.position - hit.x);

						if (!is_shadowed(hit.x, toLight, shapes, lightDist)) {
							accumColor = accumColor + blinnPhong(hit.n, hit.x, light, baseColor, texturedSphere->specular, texturedSphere->ambient, texturedSphere->exponent, cameraPos);
						}
						else {
							accumColor = accumColor + texturedSphere->ambient * baseColor;
						}
					}
				}
				else {
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
				}

				if (shape->reflectiveness > 0) {
					Vec3 reflectDirect = rayDirect - 2 * rayDirect.dot(hit.n) * hit.n;
					Vec3 reflectOrigin = hit.x + EPSILON * reflectDirect;
					Vec3 reflectedColor = trace_ray(reflectOrigin, reflectDirect, shapes, boundingSphere, lights, reflectOrigin, scene, depth + 1);
					accumColor = (1 - shape->reflectiveness) * accumColor + shape->reflectiveness * reflectedColor;
				}

				pixColor = accumColor;
			}
		}
	}

	return pixColor;
}


Vec3 trace_ray_scene9(const Vec3& rayOrigin, const Vec3& rayDirect, const vector<unique_ptr<Shape>>& shapes, const BoundingSphere& boundingSphere, const vector<Light>& lights, const Vec3& cameraPos, int depth) {
	if (depth >= 7) {  //Set to 2 for Scene 4, set to higher value for Scene 5
		return Vec3(0.0, 0.0, 0.0);
	}

	if (!boundingSphere.intersect(glm::vec3(rayOrigin.x, rayOrigin.y, rayOrigin.z), glm::vec3(rayDirect.x, rayDirect.y, rayDirect.z))) {
		return Vec3(0.0, 0.0, 0.0); //If ray doesn't intersect the bounding sphere
	}

	Vec3 pixColor(0.0, 0.0, 0.0);
	double minDist = numeric_limits<double>::infinity();

	for (const auto& shape : shapes) {
		auto intersectResult = shape->intersect(rayOrigin, rayDirect);
		if (intersectResult) {
			Hit hit = intersectResult.value();
			if (hit.s < minDist) {
				minDist = hit.s;
				Vec3 accumColor(0.0, 0.0, 0.0);

				double ao = calculate_ambient_occlusion(hit.x, hit.n, shapes);


				Vec3 ambientAO = shape->diffuse * shape->ambient * (1.0 - ao);

				for (const auto& light : lights) {
					Vec3 toLight = (light.position - hit.x).normalize();
					double lightDist = length(light.position - hit.x);

					if (!is_shadowed(hit.x, toLight, shapes, lightDist)) {
						accumColor = accumColor + blinnPhong(hit.n, hit.x, light, shape->diffuse, shape->specular, ambientAO, shape->exponent, cameraPos);
					}
					else {
						accumColor = accumColor + ambientAO;
					}
				}
				Vec3 reflectedColor(0.0, 0.0, 0.0);
				if (shape->reflectiveness > 0) {
					Vec3 reflectDirect = rayDirect - 2 * rayDirect.dot(hit.n) * hit.n;
					Vec3 reflectOrigin = hit.x + EPSILON * reflectDirect;
					reflectedColor = trace_ray_scene9(reflectOrigin, reflectDirect, shapes, boundingSphere, lights, reflectOrigin, depth + 1);
				}

				double reflectionRatio = 0.3;
				double localRatio = 0.7;
				pixColor = localRatio * accumColor + reflectionRatio * reflectedColor;
			}
		}
	}
	return pixColor;
}


bool loadMesh(const string& meshName, vector<unique_ptr<Shape>>& shapes, BoundingSphere& boundingSphere, const Vec3& materialDiffuse, const Vec3& materialSpecular, const Vec3& materialAmbient, double exponent) {
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> lShapes;
	vector<tinyobj::material_t> materials;
	string err;

	tinyobj::LoadObj(&attrib, &lShapes, &materials, &err, meshName.c_str());

	glm::vec3 minV(numeric_limits<float>::max());
	glm::vec3 maxV(-numeric_limits<float>::max());

	for (const auto& shape : lShapes) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			int fv = shape.mesh.num_face_vertices[f];
			vector<glm::vec3> vertices;
			vector<glm::vec3> normals;

			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
				glm::vec3 vertex(attrib.vertices[3 * idx.vertex_index + 0], attrib.vertices[3 * idx.vertex_index + 1], attrib.vertices[3 * idx.vertex_index + 2]);
				vertices.push_back(vertex);
				minV = glm::min(minV, vertex);
				maxV = glm::max(maxV, vertex);

				if (idx.normal_index >= 0) {
					glm::vec3 normal(attrib.normals[3 * idx.normal_index + 0], attrib.normals[3 * idx.normal_index + 1], attrib.normals[3 * idx.normal_index + 2]);
					normals.push_back(normal);
				}
				else {
					normals.push_back(glm::vec3(0, 0, 0));
				}
			}

			shapes.push_back(make_unique<Triangle>(vertices[0], vertices[1], vertices[2], normals[0], normals[1], normals[2], materialDiffuse, materialSpecular, materialAmbient, exponent));
			index_offset += fv;
		}
	}

	glm::vec3 center = (minV + maxV) * 0.5f;
	float radius = glm::distance(center, maxV);
	boundingSphere = BoundingSphere(center, radius);
	boundingSphere.valid = true;

	return true;
}





int main(int argc, char** argv)
{
	if (argc < 4) {
		cout << "Usage: A6 <SCENE> <IMAGE SIZE> <IMAGE FILENAME>" << endl;
		cout << "<SCENE> should be 0-9" << endl;
		return 0;
	}
	int scene = stoi(argv[1]);
	int imageSize = stoi(argv[2]);
	string imageFilename(argv[3]);

	Image image(imageSize, imageSize);


	vector<unique_ptr<Shape>> shapes;
	vector<Light> lights;

	BoundingSphere boundingSphere;

	if (scene == 8) {
		glm::vec3 cameraPos(-3, 0, 0);
		glm::vec3 cameraLookAt(1, 0, 0);
		glm::vec3 cameraUpVec(0, 1, 0);
		double fov = 60;
		double zPlane = 1;

		vector<Vec3> rays = create_rays_8(imageSize, imageSize, cameraPos, cameraLookAt, cameraUpVec, fov, zPlane);

		shapes.push_back(make_unique<Sphere>(Vec3(-0.5, -1.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Sphere
		shapes.push_back(make_unique<Sphere>(Vec3(0.5, -1.0, -1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Green Sphere
		shapes.push_back(make_unique<Sphere>(Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Sphere
		lights.push_back(Light(Vec3(-2.0, 1.0, 1.0), 1.0));

		

		for (int y = 0; y < imageSize; ++y) {
			for (int x = 0; x < imageSize; ++x) {
				Vec3 rayDirect = rays[y * imageSize + x];
				Vec3 pixColor = trace_ray(Vec3(cameraPos.x, cameraPos.y, cameraPos.z), rayDirect, shapes, boundingSphere, lights, Vec3(cameraPos.x, cameraPos.y, cameraPos.z), scene, 0);
				image.setPixel(x, y, static_cast<unsigned char>(min(max(pixColor.x, 0.0), 1.0) * 255), static_cast<unsigned char>(min(max(pixColor.y, 0.0), 1.0) * 255), static_cast<unsigned char>(min(max(pixColor.z, 0.0), 1.0) * 255));
			}
		}
	}
	else {
		Vec3 cameraPos(0, 0, 5);
		double fov = 45.0;
		double zPlane = 4.0;
		if (scene == 0) {
			// ========================= Change to your correct path =========================
			string texturePath = "C:/Users/Jakey/Desktop/Spring2024/CSCE441/A6/resources/Fray.jpg";
			lights.push_back(Light(Vec3(-1.0, 2.0, 1.0), 0.5));
			lights.push_back(Light(Vec3(0.5, -0.5, 0.0), 0.5));


			shapes.push_back(make_unique<TexturedSphere>(Vec3(0.5, -0.7, 0.5), Vec3(0.3, 0.3, 0.3), Vec3(1.0, 1.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.1, 0.1, 0.1), 100.0, 0.0, texturePath)); //Textured Sphere
			shapes.push_back(make_unique<Sphere>(Vec3(1.0, -0.7, 0.0), Vec3(0.3, 0.3, 0.3), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Sphere
			shapes.push_back(make_unique<Plane>(Vec3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Floor
			shapes.push_back(make_unique<Plane>(Vec3(0.0, 0.0, -3.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Back Wall
			shapes.push_back(make_unique<Sphere>(Vec3(-0.5, 0.0, -0.5), Vec3(1.0, 1.0, 1.0), Vec3(), Vec3(), Vec3(), 0, 1.0)); //Reflective Sphere 1
			shapes.push_back(make_unique<Sphere>(Vec3(1.5, 0.0, -1.5), Vec3(1.0, 1.0, 1.0), Vec3(), Vec3(), Vec3(), 0, 1.0)); //Reflective Sphere 2
		}

		else if (scene == 1 || scene == 2) {
			shapes.push_back(make_unique<Sphere>(Vec3(-0.5, -1.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Sphere
			shapes.push_back(make_unique<Sphere>(Vec3(0.5, -1.0, -1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Green Sphere
			shapes.push_back(make_unique<Sphere>(Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Sphere
			lights.push_back(Light(Vec3(-2.0, 1.0, 1.0), 1.0));
		}
		else if (scene == 3) {
			lights.push_back(Light(Vec3(1.0, 2.0, 2.0), 0.5));
			lights.push_back(Light(Vec3(-1.0, 2.0, -1.0), 0.5));


			shapes.push_back(make_unique<Ellipsoid>(Vec3(0.5, 0.0, 0.5), Vec3(0.5, 0.6, 0.2), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Ellipse
			shapes.push_back(make_unique<Sphere>(Vec3(-0.5, 0.0, -0.5), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Green Sphere
			shapes.push_back(make_unique<Plane>(Vec3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Flat Plane
		}
		else if (scene == 4 || scene == 5) {
			lights.push_back(Light(Vec3(-1.0, 2.0, 1.0), 0.5));
			lights.push_back(Light(Vec3(0.5, -0.5, 0.0), 0.5));

			//Uncomment to see the difference between Scene 5 and Scene 9 (Light intensity & location are different)
			/*
			shapes.push_back(make_unique<Cube>(Vec3(1.0, 1.0, 0.0), 0.75, Vec3(0.5, 0.5, 0.2), Vec3(1.0, 1.0, 1.0), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Yellow Cube
			shapes.push_back(make_unique<Cube>(Vec3(0.8, 1.5, 0.5), 0.55, Vec3(0.75, 0.5, 0.73), Vec3(0.5, 1.0, 1.0), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Pink Cube
			shapes.push_back(make_unique<Cube>(Vec3(-0.7, 0.8, 0.4), 0.55, Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Cube
			shapes.push_back(make_unique<Cube>(Vec3(-1.2, 0.6, 0.65), 0.55, Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Cube
			*/

			shapes.push_back(make_unique<Sphere>(Vec3(0.5, -0.7, 0.5), Vec3(0.3, 0.3, 0.3), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Sphere
			shapes.push_back(make_unique<Sphere>(Vec3(1.0, -0.7, 0.0), Vec3(0.3, 0.3, 0.3), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Sphere
			shapes.push_back(make_unique<Plane>(Vec3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Floor
			shapes.push_back(make_unique<Plane>(Vec3(0.0, 0.0, -3.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Back Wall
			shapes.push_back(make_unique<Sphere>(Vec3(-0.5, 0.0, -0.5), Vec3(1.0, 1.0, 1.0), Vec3(), Vec3(), Vec3(), 0, 1.0)); //Reflective Sphere 1
			shapes.push_back(make_unique<Sphere>(Vec3(1.5, 0.0, -1.5), Vec3(1.0, 1.0, 1.0), Vec3(), Vec3(), Vec3(), 0, 1.0)); //Reflective Sphere 2
		}
		else if (scene == 6 || scene == 7) {
			if (scene == 7) {
				lights.clear();
				lights.push_back(Light(Vec3(1.0, 1.0, 2.0), 1.0));
			}
			else {
				lights.push_back(Light(Vec3(-1.0, 1.0, 1.0), 1.0));
			}
			loadMesh("../../resources/bunny.obj", shapes, boundingSphere, Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0);

			if (scene == 7) {
				glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, -1.5f, 0.0f));
				glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(1.5f));

				glm::mat4 transform = T * R * S;

				for (auto& shape : shapes) {
					auto& triangle = dynamic_cast<Triangle&>(*shape);
					triangle.vert0 = glm::vec3(transform * glm::vec4(triangle.vert0, 1.0));
					triangle.vert1 = glm::vec3(transform * glm::vec4(triangle.vert1, 1.0));
					triangle.vert2 = glm::vec3(transform * glm::vec4(triangle.vert2, 1.0));
				}

				boundingSphere.center = glm::vec3(transform * glm::vec4(boundingSphere.center, 1.0));
				boundingSphere.radius *= 1.5;
			}
		}
		else if (scene == 9) {
			lights.push_back(Light(Vec3(-1.2, 1.8, 1.0), 1.00));
			lights.push_back(Light(Vec3(0.5, -0.5, 0.0), 0.75));

			shapes.push_back(make_unique<Cube>(Vec3(1.0, 1.0, 0.0), 0.75, Vec3(0.5, 0.5, 0.2), Vec3(1.0, 1.0, 1.0), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Yellow Cube
			shapes.push_back(make_unique<Cube>(Vec3(0.8, 1.5, 0.5), 0.55, Vec3(0.75, 0.5, 0.73), Vec3(0.5, 1.0, 1.0), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Pink Cube
			shapes.push_back(make_unique<Cube>(Vec3(-0.7, 0.8, 0.4), 0.55, Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Cube
			shapes.push_back(make_unique<Cube>(Vec3(-1.2, 0.6, 0.65), 0.55, Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Cube

			shapes.push_back(make_unique<Sphere>(Vec3(0.5, -0.7, 0.5), Vec3(0.3, 0.3, 0.3), Vec3(1.0, 0.0, 0.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Red Sphere
			shapes.push_back(make_unique<Sphere>(Vec3(1.0, -0.7, 0.0), Vec3(0.3, 0.3, 0.3), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 0.5), Vec3(0.1, 0.1, 0.1), 100.0, 0.0)); //Blue Sphere
			shapes.push_back(make_unique<Plane>(Vec3(0.0, -1.0, 0.0), Vec3(0.0, 1.0, 0.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Floor
			shapes.push_back(make_unique<Plane>(Vec3(0.0, 0.0, -3.0), Vec3(0.0, 0.0, 1.0), Vec3(1.0, 1.0, 1.0), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.1, 0.1), 0.0, 0.0)); //Back Wall
			shapes.push_back(make_unique<Sphere>(Vec3(-0.5, 0.0, -0.5), Vec3(1.0, 1.0, 1.0), Vec3(0.8, 0.0, 0.0), Vec3(0.0, 0.0, 0.0), Vec3(0.45, 0.1, 0.1), 0, 1.0)); //Reflective Sphere 1
			shapes.push_back(make_unique<Sphere>(Vec3(1.5, 0.0, -1.5), Vec3(1.0, 1.0, 1.0), Vec3(0.5, 0.0, 0.8), Vec3(0.0, 0.0, 0.0), Vec3(0.1, 0.0, 0.2), 0, 1.0)); //Reflective Sphere 2

		}
		if (scene < 9) {
			vector<Vec3> rays = create_rays(imageSize, imageSize, cameraPos, fov, zPlane);

			for (int y = 0; y < imageSize; ++y) {
				for (int x = 0; x < imageSize; ++x) {
					Vec3 rayDirect = rays[y * imageSize + x];
					Vec3 pixColor = trace_ray(cameraPos, rayDirect, shapes, boundingSphere, lights, cameraPos, scene, 0);

					image.setPixel(x, y, static_cast<unsigned char>(min(max(pixColor.x, 0.0), 1.0) * 255), static_cast<unsigned char>(min(max(pixColor.y, 0.0), 1.0) * 255), static_cast<unsigned char>(min(max(pixColor.z, 0.0), 1.0) * 255));
				}
			}
		}
		else {
			vector<Vec3> rays = create_rays(imageSize, imageSize, cameraPos, fov, zPlane);

			for (int y = 0; y < imageSize; ++y) {
				for (int x = 0; x < imageSize; ++x) {
					Vec3 rayDirect = rays[y * imageSize + x];
					Vec3 pixColor = trace_ray_scene9(cameraPos, rayDirect, shapes, boundingSphere, lights, cameraPos, 0);

					image.setPixel(x, y, static_cast<unsigned char>(min(max(pixColor.x, 0.0), 1.0) * 255), static_cast<unsigned char>(min(max(pixColor.y, 0.0), 1.0) * 255), static_cast<unsigned char>(min(max(pixColor.z, 0.0), 1.0) * 255));
				}
			}
		}

	}

	image.writeToFile(imageFilename);

	cout << "Rendered scene " << scene << " to " << imageFilename << " with size " << imageSize << "x" << imageSize << endl;


	return 0;


}
