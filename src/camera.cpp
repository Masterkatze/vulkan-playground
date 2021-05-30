#include "camera.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

void vkpg::Camera::UpdateViewMatrix()
{
	glm::mat4 rotM = glm::mat4(1.0f);
	glm::mat4 transM;

	rotM = glm::rotate(rotM, glm::radians(rotation.x * (flip_y ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::vec3 translation = position;
	if (flip_y)
	{
		translation.y *= -1.0f;
	}
	transM = glm::translate(glm::mat4(1.0f), translation);

	if (type == CameraType::firstperson)
	{
		matrices.view = rotM * transM;
	}
	else
	{
		matrices.view = transM * rotM;
	}

	view_position = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

	updated = true;
};


bool vkpg::Camera::IsMoving()
{
	return keys.left || keys.right || keys.up || keys.down;
}

float vkpg::Camera::GetNearClip()
{
	return z_near;
}

float vkpg::Camera::GetFarClip()
{
	return z_far;
}

void vkpg::Camera::SetPerspective(float fov, float aspect, float znear, float zfar)
{
	this->fov = fov;
	this->z_near = znear;
	this->z_far = zfar;
	matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	if (flip_y)
	{
		matrices.perspective[1][1] *= -1.0f;
	}
};

void vkpg::Camera::UpdateAspectRatio(float aspect)
{
	matrices.perspective = glm::perspective(glm::radians(fov), aspect, z_near, z_far);
	if (flip_y)
	{
		matrices.perspective[1][1] *= -1.0f;
	}
}

void vkpg::Camera::SetPosition(glm::vec3 position)
{
	this->position = position;
	UpdateViewMatrix();
}

void vkpg::Camera::SetRotation(glm::vec3 rotation)
{
	this->rotation = rotation;
	UpdateViewMatrix();
}

void vkpg::Camera::Rotate(glm::vec3 delta)
{
	this->rotation += delta;
	UpdateViewMatrix();
}

void vkpg::Camera::SetTranslation(glm::vec3 translation)
{
	this->position = translation;
	UpdateViewMatrix();
};

void vkpg::Camera::Translate(glm::vec3 delta)
{
	this->position += delta;
	UpdateViewMatrix();
}

void vkpg::Camera::SetRotationSpeed(float rotation_speed)
{
	this->rotation_speed = rotation_speed;
}

void vkpg::Camera::SetMovementSpeed(float movement_speed)
{
	this->movement_speed = movement_speed;
}

void vkpg::Camera::Update(std::chrono::milliseconds delta_time)
{
	updated = false;
	if (type == CameraType::firstperson)
	{
		if (IsMoving())
		{
			glm::vec3 camera_front;
			camera_front.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
			camera_front.y = sin(glm::radians(rotation.x));
			camera_front.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
			camera_front = glm::normalize(camera_front);

			float move_speed = std::chrono::duration_cast<std::chrono::seconds>(delta_time).count() * movement_speed;

			if (keys.up)
				position += camera_front * move_speed;

			if (keys.down)
				position -= camera_front * move_speed;

			if (keys.left)
				position -= glm::normalize(glm::cross(camera_front, glm::vec3(0.0f, 1.0f, 0.0f))) * move_speed;

			if (keys.right)
				position += glm::normalize(glm::cross(camera_front, glm::vec3(0.0f, 1.0f, 0.0f))) * move_speed;

			UpdateViewMatrix();
		}
	}
}
