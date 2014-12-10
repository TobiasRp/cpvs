#include "Camera.h"
#include <glm/gtx/euler_angles.hpp>

using namespace glm;

void FreeCamera::updateView() {
	m_changedView = false;

	glm::mat4 R = glm::yawPitchRoll(m_yaw, m_pitch, m_roll);

	m_look = glm::vec3(R*glm::vec4(0, 0, 1, 0));
	m_up = glm::vec3(R*glm::vec4(0, 1, 0, 0));
	m_right = glm::cross(m_look, m_up);

	glm::vec3 tgt = m_position + m_look;
	m_view = glm::lookAt(m_position, tgt, m_up);
}