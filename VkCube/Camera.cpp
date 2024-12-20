#include "Camera.h"

#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

FCamera GCamera;

FCamera::FCamera()
	: FOV(90.0f)
	, Transform()
	, Pitch(0.0f)
	, Yaw(0.0f)
	, Roll(0.0f)
{

}

FCamera::~FCamera()
{

}

void FCamera::SetPitch(float InPitch)
{
	Pitch = InPitch;
}

void FCamera::SetYaw(float InYaw)
{
	Yaw = InYaw;
}

void FCamera::SetRoll(float InRoll)
{
	Roll = InRoll;
}

glm::mat4 FCamera::GetViewMatrix() const
{
	glm::mat4 RotationMatrix = glm::toMat4(glm::quat(glm::vec3(Pitch, Yaw, Roll)));
	glm::mat4 TranslationMatrix = glm::translate(glm::mat4(1.0f), Transform.GetTranslation());

	return glm::inverse(TranslationMatrix * RotationMatrix);
}
