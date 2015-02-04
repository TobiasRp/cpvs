#include "Camera.h"
#include <glm/gtx/euler_angles.hpp>

using namespace glm;

void Camera::updateViewFrustum() {
	glm::vec3 cN = m_position + m_look * m_znear;
	glm::vec3 cF = m_position + m_look * m_zfar; 

	float Hnear = 2.0f * tan(m_fov / 2.0f) * m_znear;
	float Wnear = Hnear * m_aspectRatio;
	float Hfar = 2.0f * tan(m_fov / 2.0f) * m_zfar;
	float Wfar = Hfar * m_aspectRatio;
	float hHnear = Hnear/2.0f;
	float hWnear = Wnear/2.0f;
	float hHfar = Hfar/2.0f;
	float hWfar = Wfar/2.0f;

	vec3 farPts[4], nearPts[4];

	farPts[0] = cF + m_up * hHfar - m_right * hWfar;
	farPts[1] = cF - m_up * hHfar - m_right * hWfar;
	farPts[2] = cF - m_up * hHfar + m_right * hWfar;
	farPts[3] = cF + m_up * hHfar + m_right * hWfar;

	nearPts[0] = cN + m_up * hHnear - m_right * hWnear;
	nearPts[1] = cN - m_up * hHnear - m_right * hWnear;
	nearPts[2] = cN - m_up * hHnear + m_right * hWnear;
	nearPts[3] = cN + m_up * hHnear + m_right * hWnear;

	array<Plane, 6> planes;
	planes[0] = Plane(nearPts[3],nearPts[0],farPts[0]);
	planes[1] = Plane(nearPts[1],nearPts[2],farPts[2]);
	planes[2] = Plane(nearPts[0],nearPts[1],farPts[1]);
	planes[3] = Plane(nearPts[2],nearPts[3],farPts[2]);
	planes[4] = Plane(nearPts[0],nearPts[3],nearPts[2]);
	planes[5] = Plane(farPts[3] ,farPts[0] ,farPts[1]);

	m_viewFrustum = Frustum(planes);
}

void FreeCamera::updateView() {
	m_changedView = false;

	glm::mat4 R = glm::yawPitchRoll(m_yaw, m_pitch, m_roll);

	m_look = glm::vec3(R*glm::vec4(0, 0, 1, 0));
	m_up = glm::vec3(R*glm::vec4(0, 1, 0, 0));
	m_right = glm::cross(m_look, m_up);

	glm::vec3 tgt = m_position + m_look;
	m_view = glm::lookAt(m_position, tgt, m_up);

	updateViewFrustum();
}
