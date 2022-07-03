#include "camera.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

bool vkpg::Camera::IsMoving() const
{
	return keys.left || keys.right || keys.up || keys.down;
}

float vkpg::Camera::GetNearClip() const
{
	return z_near;
}

float vkpg::Camera::GetFarClip() const
{
	return z_far;
}

void vkpg::Camera::SetPerspective(float fov, float aspect, float znear, float zfar)
{
	this->fov = fov;
	this->z_near = znear;
	this->z_far = zfar;
	matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	if(flip_y)
	{
		matrices.perspective[1][1] *= -1.0f;
	}
};

void vkpg::Camera::UpdateAspectRatio(float aspect)
{
	matrices.perspective = glm::perspective(glm::radians(fov), aspect, z_near, z_far);
	if(flip_y)
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
	if(type == CameraType::firstperson)
	{
		if(IsMoving())
		{
			UpdateCameraFrontAndRight();

			float move_speed = std::chrono::duration_cast<std::chrono::seconds>(delta_time).count() * movement_speed;

			if(keys.up && !keys.down)
			{
				position += front * move_speed;
			}

			if(keys.down && !keys.up)
			{
				position -= front * move_speed;
			}

			if(keys.left && !keys.right)
			{

				position -= right * move_speed;
			}

			if(keys.right && !keys.left)
			{
				position += right * move_speed;
			}

			UpdateViewMatrix();
		}
	}
}

void vkpg::Camera::UpdateCameraFrontAndRight()
{
	auto pitch = glm::radians(rotation.x);
	auto yaw = glm::radians(rotation.y);

	front.x = -std::cos(pitch) * std::sin(yaw);
	front.y = std::sin(pitch);
	front.z = std::cos(pitch) * std::cos(yaw);
	front = glm::normalize(front);

	right = glm::cross(front, up);
	right = glm::normalize(right);
}

void vkpg::Camera::UpdateViewMatrix()
{
	glm::mat4 rotM = glm::mat4(1.0f);

	rotM = glm::rotate(rotM, glm::radians(rotation.x * (flip_y ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	//rotM = glm::rotate(rotM, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::vec3 translation = position;
	if(flip_y)
	{
		translation.y *= -1.0f;
	}
	glm::mat4 transM = glm::translate(glm::mat4(1.0f), translation);

	if(type == CameraType::firstperson)
	{
		matrices.view = rotM * transM;
	}
	else
	{
		matrices.view = transM * rotM;
	}



//	matrices.view = glm::lookAt(
//	    glm::vec3(2, 1, 1), // Камера находится в мировых координатах (4,3,3)
//	    glm::vec3(0, 0, 0), // И направлена в начало координат
//	    glm::vec3(0, 0, -1)  // "Голова" находится сверху
//	);


	UpdateCameraFrontAndRight();






//	auto pitch = rotation.x;
//	auto yaw = rotation.y;
//	auto roll = rotation.z;

//	front = glm::normalize(glm::vec3
//	{
//	    glm::cos(glm::radians(pitch)) * glm::sin(glm::radians(yaw)),
//	    -glm::sin(glm::radians(pitch)),
//	    glm::cos(glm::radians(pitch)) * glm::cos(glm::radians(yaw))
//	});

//	right = glm::normalize(glm::vec3
//	{
//	    glm::cos(glm::radians(yaw)),
//	    0.f,
//	    -glm::sin(glm::radians(yaw))
//	});

//	up = glm::normalize(glm::vec3
//	{
//	    glm::sin(glm::radians(pitch)) * glm::sin(glm::radians(yaw)),
//	    glm::cos(glm::radians(pitch)),
//	    glm::sin(glm::radians(pitch)) * glm::cos(glm::radians(yaw))
//	});

//	matrices.view = glm::lookAt(position, position + front, up);

	updated = true;
};
