#ifndef CAMERA_H
#define CAMERA_H

#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
	Camera(float fieldOfView, int width, int height, float znear = 0.1f, float zfar = 1000.0f)
		: m_fov(fieldOfView), m_znear(znear), m_zfar(zfar), m_changedView(false),
		m_yaw(0), m_pitch(0), m_roll(0)
	{
		setAspectRatio(width, height);
		updateProjection();
	}

	virtual ~Camera() { };

	inline glm::mat4 getProjection() const {
		return m_projection;
	}

	inline glm::mat4 getView() {
		if (m_changedView)
			updateView();
		return m_view;
	}

	inline void setFieldOfView(float fieldOfView) {
		m_fov = fieldOfView;
		updateProjection();
	}

	inline void setAspectRatio(int width, int height) {
		m_aspectRatio = (float) width / (float) height;
		updateProjection();
	}

	inline void setZNear(float znear) {
		m_znear = znear;
		updateProjection();
	}

	inline void setZFar(float zfar) {
		m_zfar = zfar;
		updateProjection();
	}

	inline void setPosition(const glm::vec3& vec) {
		m_position = vec;
		m_changedView = true;
	}

	const glm::vec3 getPosition() const {
		return m_position;
	}

	inline void rotate(float yaw, float pitch, float roll) {
		m_yaw += glm::radians(yaw);
		m_pitch += glm::radians(pitch);
		m_roll += glm::radians(roll);
		updateView();
	}

protected:
	void updateProjection() {
		m_projection = glm::perspective(m_fov, m_aspectRatio, m_znear, m_zfar);
	}

	virtual void updateView() = 0;

protected:
	glm::mat4 m_projection;
	glm::mat4 m_view;

	glm::vec3 m_position;
	glm::vec3 m_look, m_up, m_right;

	float m_fov; // field of view in degrees
	float m_aspectRatio, m_znear, m_zfar;
	float m_yaw, m_pitch, m_roll;
	bool m_changedView;
};


class FreeCamera : public Camera {
public:

	FreeCamera(float fieldOfView, int width, int height, float n = 0.1f, float f = 1000.0f)
		: Camera(fieldOfView, width, height, n, f), m_speed(1.0f)
	{
		m_look = glm::vec3(0, 0, 1);
		m_up = glm::vec3(0, 1, 0);
		m_right = glm::vec3(1, 0, 0);
	}

	virtual ~FreeCamera() { }

	inline void walk(float dt) {
		m_position += (m_look * m_speed * dt);
		m_changedView = true;
	}

	inline void strafe(float dt) {
		m_position += (m_right * m_speed * dt);
		m_changedView = true;
	}

	inline void lift(float dt) {
		m_position += (m_up * m_speed * dt);
		m_changedView = true;
	}

	inline void setSpeed(float speed) {
		m_speed = speed;
	}

protected:
	virtual void updateView();

private:
	float m_speed;
};

#endif
