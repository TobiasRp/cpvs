#ifndef BOUNDING_VOLUMES_H
#define BOUNDING_VOLUMES_H

#include "cpvs.h"

struct AABB {
	vec3 min, max;

	inline vec3 getCenter() const {
		return (max + min) * 0.5f;
	}

	inline vec3 getExtents() const {
		return max - getCenter();
	}
};

class Plane {
public:
	Plane() = default;
	Plane(const vec3& normal, float d)
		: N(- normal), dist(d) { }

	Plane(const vec3& p0, const vec3& p1, const vec3& p2) {
		N = normalize(cross(p1 - p0, p2 - p0));
		dist = - dot(N, p1);
	}

	inline float getDistance(const vec3& pt) const noexcept {
		return dot(N, pt) + dist;
	}

	/** Returns true if the AABB lies in the positive halfspace of the plane, i.e. the side in which
	 * the normal points.
	 */ 
	inline bool inPosHalfspace(const AABB& box) const noexcept {
		vec3 p = box.min, n = box.max;
		if(N.x>=0) {
			p.x = box.max.x;
			n.x = box.min.x;
		}
		if(N.y>=0) {
			p.y = box.max.y;
			n.y = box.min.y;
		}
		if(N.z>=0) {
			p.z = box.max.z;
			n.z = box.min.z;
		}

		if (getDistance(p) < 0) {
			return false;
		} 

		return true;
	}

	vec3 N;
	float dist;
};

class Frustum {
public:
	Frustum() = default;

	/** Constructs a Frustum from 6 planes.
	 * The order of the planes is (currently?) not important
	 */
	Frustum(const array<Plane, 6>& pls)
		: planes(pls) { }

	/** Returns true if the given bounding box is fully inside the frustum. */
	inline bool inside(const AABB& box) const noexcept {
		for (const auto& pl : planes) {
			if (!pl.inPosHalfspace(box))
				return false;
		}
		return true;
	}

	array<Plane, 6> planes;
};

#endif
