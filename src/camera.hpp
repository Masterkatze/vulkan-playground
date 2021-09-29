#pragma once

#include <glm/glm.hpp>

#include <chrono>

namespace vkpg
{

class Camera
{
public:
	enum CameraType { lookat, firstperson };
	CameraType type = CameraType::firstperson;

    glm::vec3 rotation{};
	glm::vec3 position{};
	glm::vec4 view_position{};

	float rotation_speed = 1.0f;
	float movement_speed = 0.8f;

	bool updated = false;
	bool flip_y = false;

	struct
	{
		glm::mat4 perspective;
		glm::mat4 view;
	} matrices;

	struct
	{
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	} keys;

	bool IsMoving();

	float GetNearClip();
	float GetFarClip();

	void SetPerspective(float fov, float aspect, float znear, float zfar);

	void UpdateAspectRatio(float aspect);

	void SetPosition(glm::vec3 position);
	void SetRotation(glm::vec3 rotation);

	void Rotate(glm::vec3 delta);

	void SetTranslation(glm::vec3 translation);
	void Translate(glm::vec3 delta);

	void SetRotationSpeed(float rotation_speed);
	void SetMovementSpeed(float movement_speed);
	void Update(std::chrono::milliseconds delta_time);

private:
	float fov = 90.0;
    float z_near = 0.1;
    float z_far = 256.0;

	void UpdateViewMatrix();
};

}
