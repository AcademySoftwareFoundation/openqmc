// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include "trace.h"

#include "parallel.h"
#include "progress.h"
#include <oqmc/gpu.h>
#include <oqmc/oqmc.h>
#include <oqmc/sampler.h>
#include <oqmc/state.h>
#include <oqmc/unused.h>

#pragma push
#pragma diag_suppress 2977
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#pragma pop

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace
{

template <typename T>
OQMC_HOST_DEVICE void swap(T& a, T& b)
{
	const auto temp = a;

	a = b;
	b = temp;
}

struct Scene
{
	struct Quad
	{
		glm::vec3 p0;
		glm::vec3 p1;
		glm::vec3 p2;
		glm::vec3 p3;
	};

	struct Camera
	{
		std::string name;
		glm::vec3 pos;
		glm::vec3 dir;
		glm::vec3 up;
		float filmSize;
		float focalLength;
		float focalDistance;
		float filterWidth;
		float fStop;
		float filmSpeed;
		float shutterSpeed;
		float exposureValue;
	};

	struct Material
	{
		std::string name;
		std::string type;
		glm::vec3 colour;
		glm::vec3 emission;
		float presence;
	};

	struct Object
	{
		std::string name;
		std::string material;
		glm::vec3 motion;
		std::vector<Quad> quads;
	};

	struct Light
	{
		std::string name;
		glm::vec3 colour;
		float watts;
		Quad quad;
	};

	Camera camera;
	std::vector<Material> materials;
	std::vector<Object> objects;
	std::vector<Light> lights;
};

struct Ray
{
	glm::vec3 origin;
	glm::vec3 dir;
	float time;

	/*AUTO_DEFINED*/ Ray() = default;
	OQMC_HOST_DEVICE Ray(glm::vec3 origin, glm::vec3 dir, float time,
	                     glm::vec3 normal);
};

// Ray construction adds a small bias to the ray to avoid floating point issues
// around self intersection. The algorithm used is based on 'A Fast and Robust
// Method for Avoiding Self-Intersection' by Carsten Wächter and Nikolaus
// Binder. This implementation in comparison to the original paper has been
// restructured for added clarity.
Ray::Ray(glm::vec3 origin, glm::vec3 dir, float time, glm::vec3 normal)
    : origin(origin), dir(dir), time(time)
{
	const auto faceForward = [](glm::vec3 d, glm::vec3 n) {
		return glm::dot(d, n) < 0 ? -n : +n;
	};

	const auto offsetRay = [](glm::vec3 p, glm::vec3 n) {
		constexpr auto intScale = 256;

		glm::ivec3 scaledN;
		scaledN.x = intScale * n.x;
		scaledN.y = intScale * n.y;
		scaledN.z = intScale * n.z;

		glm::ivec3 offset;
		offset.x = p.x < 0 ? -scaledN.x : +scaledN.x;
		offset.y = p.y < 0 ? -scaledN.y : +scaledN.y;
		offset.z = p.z < 0 ? -scaledN.z : +scaledN.z;

		glm::vec3 pos;
		pos.x = glm::intBitsToFloat(glm::floatBitsToInt(p.x) + offset.x);
		pos.y = glm::intBitsToFloat(glm::floatBitsToInt(p.y) + offset.y);
		pos.z = glm::intBitsToFloat(glm::floatBitsToInt(p.z) + offset.z);

		constexpr auto minScale = 1.0f / 32;
		constexpr auto floatScale = 1.0f / 65536;

		return glm::vec3(
		    std::abs(p.x) < minScale ? p.x + floatScale * n.x : pos.x,
		    std::abs(p.y) < minScale ? p.y + floatScale * n.y : pos.y,
		    std::abs(p.z) < minScale ? p.z + floatScale * n.z : pos.z);
	};

	this->origin = offsetRay(origin, faceForward(dir, normal));
}

struct Hit
{
	glm::vec3 p0;
	glm::vec3 p1;
	glm::vec3 p2;
	glm::vec2 bc;
	float t;
	int materialId;
};

struct Interaction
{
	Hit prim;
	glm::vec3 pos;
	glm::vec3 normal;
	bool exit;
};

struct Camera
{
	glm::vec3 pos;
	glm::vec3 dir;
	glm::vec3 up;
	float filmSize;
	float focalLength;
	float focalDistance;
	float filterWidth;
	float fStop;
	float filmSpeed;
	float shutterSpeed;
	float exposureValue;

	template <typename Sampler>
	OQMC_HOST_DEVICE Ray generateRay(int x, int y, int xSize, int ySize,
	                                 Sampler cameraDomain) const;

	OQMC_HOST_DEVICE glm::vec3 filmExposure(glm::vec3 radiance) const;
};

// Sampling functions such as the tent and the disk used when generating a
// camera ray are based on 'Sampling Transformations Zoo' by Peter Shirley,
// et. al.
template <typename Sampler>
Ray Camera::generateRay(int x, int y, int xSize, int ySize,
                        Sampler cameraDomain) const
{
	const auto sampleTent = [](float radius, float u) {
		const auto sampleLinear = [](float u) { return 1 - std::sqrt(u); };

		if(u < 0.5f)
		{
			u = 1.0f - (u / 0.5f);
			return -radius * sampleLinear(u);
		}

		u = (u - 0.5f) / 0.5f;
		return +radius * sampleLinear(u);
	};

	const auto sampleDisk = [](float radius, const float u[]) {
		auto a = 2 * u[0] - 1;
		auto b = 2 * u[1] - 1;

		if(b == 0)
		{
			b = 1;
		}

		float r;
		float phi;

		if(a * a > b * b)
		{
			r = radius * a;
			phi = (M_PI / 4) * (b / a);
		}
		else
		{
			r = radius * b;
			phi = (M_PI / 2) - (M_PI / 4) * (a / b);
		}

		return glm::vec3{r * std::cos(phi), r * std::sin(phi), 0};
	};

	enum Domains
	{
		Raster,
		LensTime,
	};

	const auto rasterDomain = cameraDomain.newDomain(Domains::Raster);
	const auto lensTimeDomain = cameraDomain.newDomain(Domains::LensTime);

	y = ySize - y - 1;
	x = xSize - x - 1;

	glm::vec2 pixelCentre;
	pixelCentre.x = x + 0.5f - (xSize / 2.0f);
	pixelCentre.y = y + 0.5f - (ySize / 2.0f);

	float rasterSamples[2];
	rasterDomain.template drawSample<2>(rasterSamples);

	glm::vec2 filterSample;
	filterSample.x = sampleTent(filterWidth, rasterSamples[0]);
	filterSample.y = sampleTent(filterWidth, rasterSamples[1]);

	const auto norm = filmSize / ySize;

	glm::vec3 filmPoint;
	filmPoint.x = pixelCentre.x * norm + filterSample.x * norm;
	filmPoint.y = pixelCentre.y * norm + filterSample.y * norm;
	filmPoint.z = -focalLength; // move EFL distance from second principal plane

	glm::vec3 focalDir;
	focalDir.x = filmPoint.x / filmPoint.z;
	focalDir.y = filmPoint.y / filmPoint.z;
	focalDir.z = 1;

	float lensTimeSamples[3];
	lensTimeDomain.template drawSample<3>(lensTimeSamples);

	const auto apertureWidth = focalLength / fStop;
	const auto apertureRadius = apertureWidth / 2;

	const auto focalPoint = focalDir * focalDistance;
	const auto lensSample = sampleDisk(apertureRadius, lensTimeSamples);
	const auto lensDir = glm::normalize(focalPoint - lensSample);

	const auto w = glm::normalize(dir);
	const auto u = glm::normalize(glm::cross(up, w));
	const auto v = glm::cross(w, u);

	Ray ret;
	ret.origin = pos + u * lensSample.x + v * lensSample.y;
	ret.dir = u * lensDir.x + v * lensDir.y + w * lensDir.z;
	ret.time = lensTimeSamples[2];

	return ret;
}

glm::vec3 Camera::filmExposure(glm::vec3 radiance) const
{
	radiance /= fStop;
	radiance /= shutterSpeed;
	radiance *= filmSpeed;
	radiance *= std::pow(2, exposureValue);

	return radiance;
}

struct Material
{
	struct Sample
	{
		glm::vec3 evaluation;
		glm::vec3 dir;
		bool successful;
	};

	glm::vec3 colour;
	glm::vec3 emission;

	enum class ScatterType
	{
		None,
		Diffuse,
		Conductor,
		Dielectric,
	} type;

	float presence;
	bool light;

	template <typename Sampler>
	OQMC_HOST_DEVICE Sample sample(const Interaction& event, const Ray& ray,
	                               Sampler materialDomain) const;

	OQMC_HOST_DEVICE bool doDirectLighting() const;
};

// Diffuse sampling uses the branchless ONB algorithm from 'Building
// an Orthonormal Basis, Revisited' by Tom Duff, et. al. to construct
// an orthonormal basis. The hemisphere sampling is based on 'Sampling
// Transformations Zoo' by Peter Shirley, et. al.
template <typename Sampler>
OQMC_HOST_DEVICE Material::Sample
diffuseSample(const Interaction& event, const Ray& ray, Sampler materialDomain)
{
	OQMC_MAYBE_UNUSED(ray);

	const auto branchlessONB = [](const glm::vec3& n, glm::vec3& b1,
	                              glm::vec3& b2) {
		const float sign = std::copysign(1.0f, n.z);
		const float a = -1 / (sign + n.z);
		const float b = n.x * n.y * a;

		b1 = glm::vec3(1 + sign * n.x * n.x * a, sign * b, -sign * n.x);
		b2 = glm::vec3(b, sign + n.y * n.y * a, -n.y);
	};

	glm::vec3 u;
	glm::vec3 v;
	const glm::vec3 w = event.normal;
	branchlessONB(w, u, v);

	const auto sampleCosineWeightedHemisphere = [u, v, w](const float rnd[2]) {
		const float r1 = 2 * M_PI * rnd[0];
		const float r2 = rnd[1];
		const float sqrtR2 = std::sqrt(r2);

		glm::vec3 dir = glm::vec3();
		dir += u * sqrtR2 * std::cos(r1);
		dir += v * sqrtR2 * std::sin(r1);
		dir += w * std::sqrt(1 - r2);

		return dir;
	};

	float materialSamples[2];
	materialDomain.template drawSample<2>(materialSamples);

	return {glm::vec3(1), sampleCosineWeightedHemisphere(materialSamples),
	        true};
}

template <typename Sampler>
OQMC_HOST_DEVICE Material::Sample conductorSample(const Interaction& event,
                                                  const Ray& ray,
                                                  Sampler materialDomain)
{
	OQMC_MAYBE_UNUSED(materialDomain);

	return {glm::vec3(1), glm::reflect(ray.dir, event.normal), true};
}

// Dieletric sampling uses a Fresnel Schlick approximation based on the blog
// post 'Memo on Fresnel equations' by Sébastien Lagarde.
template <typename Sampler>
OQMC_HOST_DEVICE Material::Sample dielectricSample(const Interaction& event,
                                                   const Ray& ray,
                                                   Sampler materialDomain)
{
	float etaA = 1.0f;
	float etaB = 1.5f;
	if(event.exit)
	{
		swap(etaA, etaB);
	}

	const glm::vec3 rdir = glm::reflect(ray.dir, event.normal);
	const glm::vec3 tdir = glm::refract(ray.dir, event.normal, etaA / etaB);

	if(tdir == glm::vec3())
	{
		return {glm::vec3(1), rdir, true};
	}

	float cosine;
	if(event.exit)
	{
		cosine = glm::dot(tdir, event.normal);
	}
	else
	{
		cosine = glm::dot(ray.dir, event.normal);
	}

	const auto schlickFresnel = [](float etaA, float etaB, float cosine) {
		const float a = etaB - etaA;
		const float b = etaB + etaA;
		const float r0 = a * a / (b * b);
		const float c = 1 - std::abs(cosine);
		return r0 + (1 - r0) * c * c * c * c * c;
	};

	const float fresnel = schlickFresnel(etaA, etaB, cosine);
	const float prob = 0.25f + 0.5f * fresnel;

	float materialSamples[1];
	materialDomain.template drawSample<1>(materialSamples);

	if(materialSamples[0] < prob)
	{
		return {glm::vec3(fresnel / prob), rdir, true};
	}

	return {glm::vec3((1 - fresnel) / (1 - prob)), tdir, true};
}

template <typename Sampler>
Material::Sample Material::sample(const Interaction& event, const Ray& ray,
                                  Sampler materialDomain) const
{
	auto sample = Material::Sample{
	    /*evaluation*/ {0, 0, 0},
	    /*dir*/ {0, 0, 0},
	    /*successful*/ false,
	};

	if(type == Material::ScatterType::Diffuse)
	{
		sample = diffuseSample(event, ray, materialDomain);
		sample.evaluation *= colour;
	}

	if(type == Material::ScatterType::Conductor)
	{
		sample = conductorSample(event, ray, materialDomain);
		sample.evaluation *= colour;
	}

	if(type == Material::ScatterType::Dielectric)
	{
		sample = dielectricSample(event, ray, materialDomain);
		sample.evaluation *= colour;
	}

	return sample;
}

bool Material::doDirectLighting() const
{
	return type == Material::ScatterType::Diffuse;
}

struct Triangle
{
	glm::vec3 p0;
	glm::vec3 p1;
	glm::vec3 p2;
	glm::vec3 motion;
	int materialId;

	OQMC_HOST_DEVICE bool intersect(const Ray& ray, Hit& prim) const;
};

bool Triangle::intersect(const Ray& ray, Hit& prim) const
{
	glm::vec2 barycentricCoordinate;
	float distance;

	if(glm::intersectRayTriangle(ray.origin - motion * ray.time, ray.dir, p0,
	                             p1, p2, barycentricCoordinate, distance))
	{
		if(distance > 0)
		{
			prim.p0 = p0;
			prim.p1 = p1;
			prim.p2 = p2;
			prim.bc = barycentricCoordinate;
			prim.t = distance;
			prim.materialId = materialId;
			return true;
		}
	}

	return false;
}

struct Light
{
	glm::vec3 energy;
	glm::vec3 origin;
	glm::vec3 normal;
	glm::vec3 u;
	glm::vec3 v;
	int materialId;

	OQMC_HOST_DEVICE glm::vec3 sample(glm::vec3 pos, const float rnd[],
	                                  float& rcpDistanceSquared) const;

	OQMC_HOST_DEVICE glm::vec3 emission(glm::vec3 dir) const;
};

glm::vec3 Light::sample(glm::vec3 pos, const float rnd[],
                        float& rcpDistanceSquared) const
{
	const auto sample = origin + u * rnd[0] + v * rnd[1];
	const auto segment = sample - pos;

	rcpDistanceSquared = 1 / glm::dot(segment, segment);

	return glm::normalize(segment);
}

glm::vec3 Light::emission(glm::vec3 dir) const
{
	return energy * std::abs(glm::dot(dir, normal));
}

struct Session
{
	Camera* camera;

	int numMaterials;
	Material* materials;

	int numTriangles;
	Triangle* triangles;

	int numLights;
	Light* lights;

	Session(const Scene& scene);
	void release() const;
};

Session::Session(const Scene& scene)
{
	auto objectsQuadsSize = 0;
	for(const auto& object : scene.objects)
	{
		objectsQuadsSize += object.quads.size();
	}

	numMaterials = scene.materials.size() + scene.lights.size();
	numTriangles = objectsQuadsSize + scene.lights.size();
	numLights = scene.lights.size();

	numTriangles *= 2;

	OQMC_ALLOCATE(&camera, 1);
	OQMC_ALLOCATE(&materials, numMaterials);
	OQMC_ALLOCATE(&triangles, numTriangles);
	OQMC_ALLOCATE(&lights, numLights);

	camera->pos = scene.camera.pos;
	camera->dir = scene.camera.dir;
	camera->up = scene.camera.up;
	camera->filmSize = scene.camera.filmSize;
	camera->focalLength = scene.camera.focalLength;
	camera->focalDistance = scene.camera.focalDistance;
	camera->filterWidth = scene.camera.filterWidth;
	camera->fStop = scene.camera.fStop;
	camera->filmSpeed = scene.camera.filmSpeed;
	camera->shutterSpeed = scene.camera.shutterSpeed;
	camera->exposureValue = scene.camera.exposureValue;

	auto materialIndex = 0;

	for(const auto& material : scene.materials)
	{
		auto type = Material::ScatterType::None;

		if(material.type == "diffuse")
		{
			type = Material::ScatterType::Diffuse;
		}

		if(material.type == "conductor")
		{
			type = Material::ScatterType::Conductor;
		}

		if(material.type == "dielectric")
		{
			type = Material::ScatterType::Dielectric;
		}

		materials[materialIndex].colour = material.colour;
		materials[materialIndex].emission = material.emission;
		materials[materialIndex].type = type;
		materials[materialIndex].presence = material.presence;
		materials[materialIndex].light = false;
		++materialIndex;
	}

	auto trianglesIndex = 0;

	for(std::size_t i = 0; i < scene.objects.size(); ++i)
	{
		const auto& object = scene.objects[i];

		auto materialId = 0;
		for(std::size_t j = 0; j < scene.materials.size(); ++j)
		{
			const auto& material = scene.materials[j];

			if(object.material == material.name)
			{
				materialId = j;

				break;
			}
		}

		for(std::size_t j = 0; j < object.quads.size(); ++j)
		{
			const auto& quad = object.quads[j];

			triangles[trianglesIndex].p0 = quad.p0;
			triangles[trianglesIndex].p1 = quad.p1;
			triangles[trianglesIndex].p2 = quad.p2;
			triangles[trianglesIndex].motion = object.motion;
			triangles[trianglesIndex].materialId = materialId;
			++trianglesIndex;

			triangles[trianglesIndex].p0 = quad.p0;
			triangles[trianglesIndex].p1 = quad.p2;
			triangles[trianglesIndex].p2 = quad.p3;
			triangles[trianglesIndex].motion = object.motion;
			triangles[trianglesIndex].materialId = materialId;
			++trianglesIndex;
		}
	}

	for(std::size_t i = 0; i < scene.lights.size(); ++i)
	{
		const auto& light = scene.lights[i];

		const auto u = light.quad.p1 - light.quad.p0;
		const auto v = light.quad.p3 - light.quad.p0;
		const auto w = glm::cross(u, v);
		const auto area = glm::length(w);

		lights[i].energy = light.colour * light.watts;
		lights[i].origin = light.quad.p0;
		lights[i].normal = w / area;
		lights[i].u = u;
		lights[i].v = v;
		lights[i].materialId = materialIndex;

		triangles[trianglesIndex].p0 = light.quad.p0;
		triangles[trianglesIndex].p1 = light.quad.p1;
		triangles[trianglesIndex].p2 = light.quad.p2;
		triangles[trianglesIndex].motion = glm::vec3();
		triangles[trianglesIndex].materialId = materialIndex;
		++trianglesIndex;

		triangles[trianglesIndex].p0 = light.quad.p0;
		triangles[trianglesIndex].p1 = light.quad.p2;
		triangles[trianglesIndex].p2 = light.quad.p3;
		triangles[trianglesIndex].motion = glm::vec3();
		triangles[trianglesIndex].materialId = materialIndex;
		++trianglesIndex;

		materials[materialIndex].colour = glm::vec3();
		materials[materialIndex].emission = light.colour * light.watts / area;
		materials[materialIndex].type = Material::ScatterType::None;
		materials[materialIndex].presence = 1;
		materials[materialIndex].light = true;
		++materialIndex;
	}
}

void Session::release() const
{
	OQMC_FREE(camera);
	OQMC_FREE(materials);
	OQMC_FREE(triangles);
	OQMC_FREE(lights);
}

// Russian roulette is based on the method descsribed in 'Robust Monte Carlo
// Methods for Light Transport Simulation' by Eric Veach.
template <typename Sampler>
OQMC_HOST_DEVICE bool russianRoulette(glm::vec3 throughput,
                                      Sampler rouletteDomain, float& weight)
{
	constexpr auto threshold = 0.05f;
	constexpr auto lowProb = 1e-2f;

	float maxCoeff;
	maxCoeff = std::fmax(throughput.x, throughput.y);
	maxCoeff = std::fmax(maxCoeff, throughput.z);

	const float prob =
	    std::fmin(std::fmax(maxCoeff / threshold, lowProb), 1.0f);

	float rouletteSamples[1];
	rouletteDomain.template drawSample<1>(rouletteSamples);

	if(rouletteSamples[0] > prob)
	{
		return false;
	}

	weight = 1 / prob;
	return true;
}

OQMC_HOST_DEVICE bool intersect(const Session& session, const Ray& ray,
                                Interaction& event)
{
	bool hit = false;
	for(int i = 0; i < session.numTriangles; ++i)
	{
		Hit prim;
		if(session.triangles[i].intersect(ray, prim))
		{
			if(!hit || prim.t < event.prim.t)
			{
				hit = true;
				event.prim = prim;
			}
		}
	}

	if(hit)
	{
		const glm::vec3 u = event.prim.p1 - event.prim.p0;
		const glm::vec3 v = event.prim.p2 - event.prim.p0;

		event.pos = event.prim.p0 + u * event.prim.bc.x + v * event.prim.bc.y;
		event.normal = glm::normalize(glm::cross(u, v));

		event.exit = false;
		if(glm::dot(event.normal, ray.dir) > 0)
		{
			event.normal = -event.normal;
			event.exit = true;
		}

		return true;
	}

	return false;
}

template <typename Sampler>
OQMC_HOST_DEVICE bool
intersectOpacityCheck(const Session& session, int maxOpacity, Ray ray,
                      Sampler opacityDomain, Interaction& event)
{
	for(int i = 0; i < maxOpacity; ++i)
	{
		enum Domains
		{
			Next,
		};

		if(!intersect(session, ray, event))
		{
			break;
		}

		float opacitySamples[1];
		opacityDomain.template drawSample<1>(opacitySamples);

		const auto& material = session.materials[event.prim.materialId];

		if(opacitySamples[0] < material.presence)
		{
			return true;
		}

		ray = Ray(event.pos, ray.dir, ray.time, event.normal);
		opacityDomain = opacityDomain.newDomain(Domains::Next);
	}

	return false;
}

template <typename Sampler>
OQMC_HOST_DEVICE glm::vec3
directLighting(const Session& session, int numLightSamples, int maxOpacity,
               const Ray& pathRay, const Interaction& pathEvent,
               Sampler directDomain)
{
	glm::vec3 direct = glm::vec3();
	for(int i = 0; i < session.numLights; ++i)
	{
		const auto& light = session.lights[i];

		auto splitDomain = directDomain.newDomainSplit(i, numLightSamples);

		for(int j = 0; j < numLightSamples; ++j)
		{
			enum Domains
			{
				Light,
				Opacity,
			};

			const auto lightDomain = splitDomain.newDomain(Domains::Light);
			const auto opacityDomain = splitDomain.newDomain(Domains::Opacity);

			float lightSamples[2];
			lightDomain.template drawSample<2>(lightSamples);

			float rcpDistSqr;
			const auto dir =
			    light.sample(pathEvent.pos, lightSamples, rcpDistSqr);

			const auto shadowRay =
			    Ray(pathEvent.pos, dir, pathRay.time, pathEvent.normal);

			Interaction shadowEvent;
			const auto shadowHit = intersectOpacityCheck(
			    session, maxOpacity, shadowRay, opacityDomain, shadowEvent);

			if(shadowHit && shadowEvent.prim.materialId == light.materialId &&
			   !shadowEvent.exit)
			{
				const auto project = std::abs(glm::dot(dir, pathEvent.normal));
				const auto illuminance = light.emission(dir) * rcpDistSqr;

				direct +=
				    project * illuminance / static_cast<float>(numLightSamples);
			}

			splitDomain = splitDomain.nextDomainIndex();
		}
	}

	return direct;
}

template <typename Sampler>
OQMC_HOST_DEVICE glm::vec3 trace(const Session& session, int numLightSamples,
                                 int maxDepth, int maxOpacity, Ray ray,
                                 Sampler traceDomain)
{
	bool computeEmission = true;
	glm::vec3 throughput = glm::vec3(1);

	glm::vec3 radiance = glm::vec3();
	for(int depth = 0; depth <= maxDepth; ++depth)
	{
		enum Domains
		{
			Opacity,
			Direct,
			Material,
			Roulette,
			Next,
		};

		const auto opacityDomain = traceDomain.newDomain(Domains::Opacity);
		const auto materialDomain = traceDomain.newDomain(Domains::Material);
		const auto rouletteDomain = traceDomain.newDomain(Domains::Roulette);

		Interaction event;
		if(!intersectOpacityCheck(session, maxOpacity, ray, opacityDomain,
		                          event))
		{
			break;
		}

		const auto& material = session.materials[event.prim.materialId];

		glm::vec3 emissionContribution;
		if((computeEmission || !material.light) && !event.exit)
		{
			emissionContribution = throughput * material.emission;
		}
		else
		{
			emissionContribution = glm::vec3();
		}

		radiance += emissionContribution;

		glm::vec3 directLightingContribution;
		if(material.doDirectLighting())
		{
			// Moved to within if block, no need to compute if not needed.
			const auto directDomain = traceDomain.newDomain(Domains::Direct);

			const auto bsdf = glm::vec3(M_1_PI) * material.colour;
			const auto light = directLighting(
			    session, numLightSamples, maxOpacity, ray, event, directDomain);

			directLightingContribution = throughput * bsdf * light;
			computeEmission = false;
		}
		else
		{
			directLightingContribution = glm::vec3();
			computeEmission = true;
		}

		radiance += directLightingContribution;

		const auto sample = material.sample(event, ray, materialDomain);

		if(!sample.successful)
		{
			break;
		}

		throughput *= sample.evaluation;

		if(throughput == glm::vec3())
		{
			break;
		}

		float rr;
		if(!russianRoulette(throughput, rouletteDomain, rr))
		{
			break;
		}

		throughput *= rr;

		ray = Ray(event.pos, sample.dir, ray.time, event.normal);
		traceDomain = traceDomain.newDomain(Domains::Next);
	}

	return radiance;
}

namespace scenes
{

namespace cameras
{

const auto camera = Scene::Camera{
    /*name*/ "camera",
    /*pos*/ {278, 273, -2250},
    /*dir*/ {0, 0, 1},
    /*up*/ {0, 1, 0},
    /*filmSize*/ 24,
    /*focalLength*/ 100,
    /*focalDistance*/ 2550,
    /*filterWidth*/ 1,
    /*fStop*/ 1.2,
    /*filmSpeed*/ 200,
    /*shutterSpeed*/ 100,
    /*exposureValue*/ 0,
};

const auto cameraNoDof = Scene::Camera{
    /*name*/ "camera",
    /*pos*/ {278, 273, -2250},
    /*dir*/ {0, 0, 1},
    /*up*/ {0, 1, 0},
    /*filmSize*/ 24,
    /*focalLength*/ 100,
    /*focalDistance*/ 2550,
    /*filterWidth*/ 1,
    /*fStop*/ 256,
    /*filmSpeed*/ 200,
    /*shutterSpeed*/ 100,
    /*exposureValue*/ 8,
};

} // namespace cameras

namespace materials
{

const auto white = Scene::Material{
    /*name*/ "white",
    /*type*/ "diffuse",
    /*colour*/ {1, 1, 1},
    /*emission*/ {0, 0, 0},
    /*presence*/ 1,
};

const auto green = Scene::Material{
    /*name*/ "green",
    /*type*/ "diffuse",
    /*colour*/ {0, 1, 0},
    /*emission*/ {0, 0, 0},
    /*presence*/ 1,
};

const auto red = Scene::Material{
    /*name*/ "red",
    /*type*/ "diffuse",
    /*colour*/ {1, 0, 0},
    /*emission*/ {0, 0, 0},
    /*presence*/ 1,
};

const auto metal = Scene::Material{
    /*name*/ "metal",
    /*type*/ "conductor",
    /*colour*/ {1, 0.844, 0.428},
    /*emission*/ {0, 0, 0},
    /*presence*/ 1,
};

const auto glass = Scene::Material{
    /*name*/ "glass",
    /*type*/ "dielectric",
    /*colour*/ {1, 1, 1},
    /*emission*/ {0, 0, 0},
    /*presence*/ 1,
};

const auto emissive10 = Scene::Material{
    /*name*/ "emissive10",
    /*type*/ "null",
    /*colour*/ {1, 1, 1},
    /*emission*/ {10, 10, 10},
    /*presence*/ 1,
};

const auto emissivE01 = Scene::Material{
    /*name*/ "emissive01",
    /*type*/ "null",
    /*colour*/ {1, 1, 1},
    /*emission*/ {1, 1, 1},
    /*presence*/ 1,
};

const auto transparent = Scene::Material{
    /*name*/ "transparent",
    /*type*/ "null",
    /*colour*/ {1, 1, 1},
    /*emission*/ {0, 0, 0},
    /*presence*/ 0.2,
};

const auto matte = Scene::Material{
    /*name*/ "matte",
    /*type*/ "null",
    /*colour*/ {1, 1, 1},
    /*emission*/ {0, 0, 0},
    /*presence*/ 1,
};

} // namespace materials

namespace objects
{

const auto emitter = Scene::Object{
    /*name*/ "emitter",
    /*material*/ "emissive10",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
        {343, /*548.8*/ 548, 227},
        {343, /*548.8*/ 548, 332},
        {213, /*548.8*/ 548, 332},
        {213, /*548.8*/ 548, 227},
    }},
};

const auto floor = Scene::Object{
    /*name*/ "floor",
    /*material*/ "white",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
        {552.8, 0, 0},
        {0, 0, 0},
        {0, 0, 559.2},
        {549.6, 0, 559.2},
    }},
};

const auto ceiling = Scene::Object{
    /*name*/ "ceiling",
    /*material*/ "white",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
        {556, 548.8, 0},
        {556, 548.8, 559.2},
        {0, 548.8, 559.2},
        {0, 548.8, 0},
    }},
};

const auto backWall = Scene::Object{
    /*name*/ "back wall",
    /*material*/ "white",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
        {549.6, 0, 559.2},
        {0, 0, 559.2},
        {0, 548.8, 559.2},
        {556, 548.8, 559.2},
    }},
};

const auto rightWall = Scene::Object{
    /*name*/ "right wall",
    /*material*/ "green",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
        {0, 0, 559.2},
        {0, 0, 0},
        {0, 548.8, 0},
        {0, 548.8, 559.2},
    }},
};

const auto leftWall = Scene::Object{
    /*name*/ "left wall",
    /*material*/ "red",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
        {552.8, 0, 0},
        {549.6, 0, 559.2},
        {556, 548.8, 559.2},
        {556, 548.8, 0},
    }},
};

const auto shortBlock = Scene::Object{
    /*name*/ "short block",
    /*material*/ "white",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
         {130, 165, 65},
         {82, 165, 225},
         {240, 165, 272},
         {290, 165, 114},
     },
     {
         {290, 0, 114},
         {290, 165, 114},
         {240, 165, 272},
         {240, 0, 272},
     },
     {
         {130, 0, 65},
         {130, 165, 65},
         {290, 165, 114},
         {290, 0, 114},
     },
     {
         {82, 0, 225},
         {82, 165, 225},
         {130, 165, 65},
         {130, 0, 65},
     },
     {
         {240, 0, 272},
         {240, 165, 272},
         {82, 165, 225},
         {82, 0, 225},
     }},
};

const auto tallBlock = Scene::Object{
    /*name*/ "tall block",
    /*material*/ "white",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
         {423, 330, 247},
         {265, 330, 296},
         {314, 330, 456},
         {472, 330, 406},
     },
     {
         {423, 0, 247},
         {423, 330, 247},
         {472, 330, 406},
         {472, 0, 406},
     },
     {
         {472, 0, 406},
         {472, 330, 406},
         {314, 330, 456},
         {314, 0, 456},
     },
     {
         {314, 0, 456},
         {314, 330, 456},
         {265, 330, 296},
         {265, 0, 296},
     },
     {
         {265, 0, 296},
         {265, 330, 296},
         {423, 330, 247},
         {423, 0, 247},
     }},
};

const auto emissiveBackWall = Scene::Object{
    /*name*/ "emissive back wall",
    /*material*/ "emissive01",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
        {549.6, 0, 301},
        {0, 0, 301},
        {0, 548.8, 301},
        {556, 548.8, 301},
    }},
};

const auto transparentBlocker = Scene::Object{
    /*name*/ "transparent blocker",
    /*material*/ "transparent",
    /*motion*/ {0, 0, 0},
    /*quads*/
    {{
        {476.4, 74.4, 300},
        {76.4, 74.4, 300},
        {76.4, 474.4, 300},
        {476.4, 474.4, 300},
    }},
};

const auto movingBlocker = Scene::Object{
    /*name*/ "moving blocker",
    /*material*/ "matte",
    /*motion*/ {80, 80, 0},
    /*quads*/
    {{
        {0, 0, 300},
        {400, 0, 300},
        {400, 400, 300},
        {0, 400, 300},
    }},
};

} // namespace objects

namespace lights
{

const auto light = Scene::Light{
    /*name*/ "light",
    /*colour*/ {1, 1, 1},
    /*watts*/ 136500,
    /*quad*/
    {
        {343, /*548.8*/ 548, 227},
        {343, /*548.8*/ 548, 332},
        {213, /*548.8*/ 548, 332},
        {213, /*548.8*/ 548, 227},
    },
};

} // namespace lights

// http://www.graphics.cornell.edu/online/box/data.html
const auto cornellBox = Scene{
    /*camera*/ cameras::camera,
    /*materials*/
    {
        // materials::emissive10,
        // materials::metal,
        // materials::glass,
        materials::white,
        materials::green,
        materials::red,
    },
    /*objects*/
    {
        // objects::emitter,
        objects::floor,
        objects::ceiling,
        objects::backWall,
        objects::rightWall,
        objects::leftWall,
        objects::shortBlock,
        objects::tallBlock,
    },
    /*lights*/
    {
        lights::light,
    },
};

const auto presenceExample = Scene{
    /*camera*/ cameras::cameraNoDof,
    /*materials*/
    {
        materials::emissivE01,
        materials::transparent,
    },
    /*objects*/
    {
        objects::emissiveBackWall,
        objects::transparentBlocker,
    },
    /*lights*/ {},
};

const auto motionBlurExample = Scene{
    /*camera*/ cameras::cameraNoDof,
    /*materials*/
    {
        materials::emissivE01,
        materials::matte,
    },
    /*objects*/
    {
        objects::emissiveBackWall,
        objects::movingBlocker,
    },
    /*lights*/ {},
};

} // namespace scenes

struct Buffer
{
	glm::vec3* image;
	void* cache;
};

template <typename Sampler>
Buffer start(int numPixels)
{
	Buffer ret;
	OQMC_ALLOCATE(&ret.image, numPixels);
	OQMC_ALLOCATE(&ret.cache, Sampler::cacheSize);

	return ret;
}

void stop(Buffer out)
{
	OQMC_FREE(out.image);
	OQMC_FREE(out.cache);
}

template <typename Sampler>
bool run(const char* name, int width, int height, int frame,
         int numPixelSamples, int numLightSamples, int maxDepth, int maxOpacity,
         float3* out)
{
	Scene scene;
	bool valid = false;

	if(std::string(name) == "box")
	{
		scene = scenes::cornellBox;
		valid = true;
	}

	if(std::string(name) == "presence")
	{
		scene = scenes::presenceExample;
		valid = true;
	}

	if(std::string(name) == "blur")
	{
		scene = scenes::motionBlurExample;
		valid = true;
	}

	if(!valid)
	{
		return false;
	}

	const auto session = Session(scene);
	const auto numPixels = width * height;

	auto buffer = start<Sampler>(numPixels);
	Sampler::initialiseCache(buffer.cache);

	auto start = oqmc_progress_start("Tracing image:", numPixelSamples);

	for(int i = 0; i < numPixels; ++i)
	{
		buffer.image[i] = glm::vec3();
	}

	for(int i = 0; i < numPixelSamples; ++i)
	{
		const auto func = [=] OQMC_HOST_DEVICE(std::size_t idx) {
			const auto x = idx % width;
			const auto y = idx / width;

			const auto pixelDomain = Sampler(x, y, frame, i, buffer.cache);

			enum Domains
			{
				Camera,
				Trace,
			};

			const auto cameraDomain = pixelDomain.newDomain(Domains::Camera);
			const auto traceDomain = pixelDomain.newDomain(Domains::Trace);

			const auto ray =
			    session.camera->generateRay(x, y, width, height, cameraDomain);

			const auto radiance = trace(session, numLightSamples, maxDepth,
			                            maxOpacity, ray, traceDomain);

			const auto delta =
			    session.camera->filmExposure(radiance) - buffer.image[idx];

			const auto deltaOverN = delta / (i + 1.0f);

			buffer.image[idx] += deltaOverN;
		};

		const auto begin = 0;
		const auto end = numPixels;

		OQMC_FORLOOP(func, begin, end);

		oqmc_progress_add("Tracing image:", numPixelSamples, i + 1, start);
	}

	oqmc_progress_end();

	for(int i = 0; i < numPixels; ++i)
	{
		out[i].x = buffer.image[i].x;
		out[i].y = buffer.image[i].y;
		out[i].z = buffer.image[i].z;
	}

	session.release();
	stop(buffer);

	return true;
}

class RngImpl
{
	friend oqmc::SamplerInterface<RngImpl>;

	static constexpr std::size_t cacheSize = 0;
	static void initialiseCache(void* cache);

	OQMC_HOST_DEVICE RngImpl(oqmc::State64Bit state);
	OQMC_HOST_DEVICE RngImpl(int x, int y, int frame, int sampleId,
	                         const void* cache);

	OQMC_HOST_DEVICE RngImpl newDomain(int key) const;
	OQMC_HOST_DEVICE RngImpl newDomainSplit(int key, int size) const;
	OQMC_HOST_DEVICE RngImpl nextDomainIndex() const;

	template <int Size>
	OQMC_HOST_DEVICE void drawSample(std::uint32_t samples[Size]) const;

	template <int Size>
	OQMC_HOST_DEVICE void drawRnd(std::uint32_t rnds[Size]) const;

	oqmc::State64Bit state;
};

inline void RngImpl::initialiseCache(void* cache)
{
	OQMC_MAYBE_UNUSED(cache);
}

inline RngImpl::RngImpl(oqmc::State64Bit state) : state(state)
{
}

inline RngImpl::RngImpl(int x, int y, int frame, int sampleId,
                        const void* cache)
    : state(x, y, frame, sampleId)
{
	OQMC_MAYBE_UNUSED(cache);
	state.pixelDecorrelate();
}

inline RngImpl RngImpl::newDomain(int key) const
{
	return {state.newDomain(key)};
}

inline RngImpl RngImpl::newDomainSplit(int key, int size) const
{
	return {state.newDomainSplit(key, size)};
}

inline RngImpl RngImpl::nextDomainIndex() const
{
	return {state.nextDomainIndex()};
}

template <int Size>
void RngImpl::drawSample(std::uint32_t samples[Size]) const
{
	drawRnd<Size>(samples);
}

template <int Size>
void RngImpl::drawRnd(std::uint32_t rnds[Size]) const
{
	state.drawRnd<Size>(rnds);
}

using RngSampler = oqmc::SamplerInterface<RngImpl>;

} // namespace

OQMC_CABI bool oqmc_trace(const char* name, const char* scene, int width,
                          int height, int frame, int numPixelSamples,
                          int numLightSamples, int maxDepth, int maxOpacity,
                          float3* image)
{
	assert(name);
	assert(scene);
	assert(width >= 0);
	assert(height >= 0);
	assert(numPixelSamples >= 0);
	assert(numLightSamples >= 0);
	assert(maxDepth >= 0);
	assert(maxOpacity >= 0);
	assert(image);

	if(std::string(name) == "pmj")
	{
		return run<oqmc::PmjSampler>(scene, width, height, frame,
		                             numPixelSamples, numLightSamples, maxDepth,
		                             maxOpacity, image);
	}

	if(std::string(name) == "pmjbn")
	{
		return run<oqmc::PmjBnSampler>(scene, width, height, frame,
		                               numPixelSamples, numLightSamples,
		                               maxDepth, maxOpacity, image);
	}

	if(std::string(name) == "sobol")
	{
		return run<oqmc::SobolSampler>(scene, width, height, frame,
		                               numPixelSamples, numLightSamples,
		                               maxDepth, maxOpacity, image);
	}

	if(std::string(name) == "sobolbn")
	{
		return run<oqmc::SobolBnSampler>(scene, width, height, frame,
		                                 numPixelSamples, numLightSamples,
		                                 maxDepth, maxOpacity, image);
	}

	if(std::string(name) == "lattice")
	{
		return run<oqmc::LatticeSampler>(scene, width, height, frame,
		                                 numPixelSamples, numLightSamples,
		                                 maxDepth, maxOpacity, image);
	}

	if(std::string(name) == "latticebn")
	{
		return run<oqmc::LatticeBnSampler>(scene, width, height, frame,
		                                   numPixelSamples, numLightSamples,
		                                   maxDepth, maxOpacity, image);
	}

	if(std::string(name) == "rng")
	{
		return run<RngSampler>(scene, width, height, frame, numPixelSamples,
		                       numLightSamples, maxDepth, maxOpacity, image);
	}

	return false;
}
