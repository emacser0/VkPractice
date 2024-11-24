#include "Config.h"
#include "Mesh.h"
#include "AssetManager.h"
#include "TextureSource.h"
#include "Widget.h"

#include "VulkanContext.h"
#include "VulkanModel.h"
#include "VulkanScene.h"
#include "VulkanMeshRenderer.h"
#include "VulkanSkyRenderer.h"
#include "VulkanUIRenderer.h"

#include "Engine.h"
#include "World.h"
#include "Actor.h"
#include "CameraActor.h"
#include "PointLightActor.h"
#include "DirectionalLightActor.h"
#include "MeshActor.h"
#include "SkyActor.h"

#include <ctime>
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <cstdlib>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

#include "imgui/imgui.h"

glm::vec3 CameraMoveDelta(0.0f);

double PrevMouseX, PrevMouseY;

void OnMouseButtonEvent(GLFWwindow* Window, int Button, int Action, int Mods)
{
	if (Button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (Action == GLFW_PRESS)
		{
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwGetCursorPos(Window, &PrevMouseX, &PrevMouseY);
		}
		else if (Action == GLFW_RELEASE)
		{
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

void OnMouseWheelEvent(GLFWwindow* Window, double XOffset, double YOffset)
{
	FWorld* World = GEngine->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (abs(YOffset) >= DBL_EPSILON)
	{
		ACameraActor* Camera = World->GetCamera();
		if (Camera == nullptr)
		{
			return;
		}

		glm::mat4 RotationMatrix = glm::toMat4(Camera->GetRotation());

		glm::vec4 MoveVector(0.0f, 0.0f, -YOffset, 1.0f);

		float CameraMoveSpeed;
		GConfig->Get("CameraMoveSpeed", CameraMoveSpeed);

		Camera->SetLocation(Camera->GetLocation() + glm::vec3(RotationMatrix * MoveVector) * CameraMoveSpeed * 0.1f);
	}
}

void OnKeyEvent(GLFWwindow* Window, int Key, int ScanCode, int Action, int Mods)
{
	if (Key == GLFW_KEY_W)
	{
		if (Action == GLFW_PRESS)
		{
			CameraMoveDelta.z = -1.0f;
		}
		else if (Action == GLFW_RELEASE && CameraMoveDelta.z == -1.0f)
		{
			CameraMoveDelta.z = 0.0f;
		}
	}
	else if (Key == GLFW_KEY_S)
	{
		if (Action == GLFW_PRESS)
		{
			CameraMoveDelta.z = 1.0f;
		}
		else if (Action == GLFW_RELEASE && CameraMoveDelta.z == 1.0f)
		{
			CameraMoveDelta.z = 0.0f;
		}
	}
	else if (Key == GLFW_KEY_A)
	{
		if (Action == GLFW_PRESS)
		{
			CameraMoveDelta.x = -1.0f;
		}
		else if (Action == GLFW_RELEASE && CameraMoveDelta.x == -1.0f)
		{
			CameraMoveDelta.x = 0.0f;
		}
	}
	else if (Key == GLFW_KEY_D)
	{
		if (Action == GLFW_PRESS)
		{
			CameraMoveDelta.x = 1.0f;
		}
		else if (Action == GLFW_RELEASE && CameraMoveDelta.x == 1.0f)
		{
			CameraMoveDelta.x = 0.0f;
		}
	}
	else if (Key == GLFW_KEY_LEFT_CONTROL)
	{
		if (Action == GLFW_PRESS)
		{
			CameraMoveDelta.y = 1.0f;
		}
		else if (Action == GLFW_RELEASE && CameraMoveDelta.y == 1.0f)
		{
			CameraMoveDelta.y = 0.0f;
		}
	}
	else if (Key == GLFW_KEY_SPACE)
	{
		if (Action == GLFW_PRESS)
		{
			CameraMoveDelta.y = -1.0f;
		}
		else if (Action == GLFW_RELEASE && CameraMoveDelta.y == -1.0f)
		{
			CameraMoveDelta.y = 0.0f;
		}
	}
}

void Update(float InDeltaTime); 

int RandRange(int Min, int Max)
{
	return rand() % (Max - Min + 1) + Min;
}

FVulkanMeshRenderer* MeshRenderer;
FVulkanSkyRenderer* SkyRenderer;

APointLightActor* PointLight;
ADirectionalLightActor* DirectionalLight;

class FMainWidget : public FWidget
{
public:
	FMainWidget();
	virtual ~FMainWidget() { }

	virtual void Draw();
	void OnShaderItemSelected(const std::string& NewSelectedItem);

private:
	bool bInitialized;
	std::vector<std::string> ShaderItems;
	std::string CurrentShaderItem;

	glm::vec3 PointLightPosition;
	glm::vec4 PointAmbient;
	glm::vec4 PointDiffuse;
	glm::vec4 PointSpecular;
	glm::vec4 PointAttenuation;
	float PointShininess;

	glm::vec3 DirDirection;
	glm::vec4 DirAmbient;
	glm::vec4 DirDiffuse;
	glm::vec4 DirSpecular;
	glm::vec4 DirAttenuation;
	float DirShininess;

	bool bShowTBN;
};

FMainWidget::FMainWidget()
	: bInitialized(false)
	, ShaderItems({ "phong", "blinn_phong" })
	, CurrentShaderItem(ShaderItems[1])
	, bShowTBN(false)
{
	PointLightPosition = glm::vec3(1.0f);
	PointAmbient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
	PointDiffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	PointSpecular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	PointAttenuation = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);
	PointShininess = 32;

	DirDirection = glm::vec3(0.0f, 1.0f, 0.0f);
	DirAmbient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
	DirDiffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	DirSpecular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	DirAttenuation = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);
	DirShininess = 32;
}

void FMainWidget::Draw()
{
	ImGui::Begin("Properties");

	if (bInitialized == false)
	{
		ImGui::SetWindowPos(ImVec2(20, 20));
		ImGui::SetWindowSize(ImVec2(300, 300));
		bInitialized = true;
	}

	if (ImGui::BeginCombo("Shader", CurrentShaderItem.c_str()))
	{
		for (const std::string& Item : ShaderItems)
		{
			bool bIsSelected = CurrentShaderItem == Item;
			if (ImGui::Selectable(Item.c_str(), bIsSelected))
			{
				CurrentShaderItem = Item;
				OnShaderItemSelected(CurrentShaderItem.c_str());
			}

			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::Text("Point Light");

	ImGui::InputFloat3("Point Position", &PointLightPosition[0]);
	ImGui::SliderFloat4("Point Ambient", &PointAmbient[0], 0.0f, 1.0f);
	ImGui::SliderFloat4("Point Diffuse", &PointDiffuse[0], 0.0f, 1.0f);
	ImGui::SliderFloat4("Point Specular", &PointSpecular[0], 0.0f, 1.0f);
	ImGui::SliderFloat4("Point Attenuation", &PointAttenuation[0], 0.0f, 1.0f);
	ImGui::SliderFloat("Point Shininess", &PointShininess, 1.0f, 128.0f);

	if (PointLight != nullptr)
	{
		PointLight->SetLocation(PointLightPosition);
		PointLight->SetAmbient(PointAmbient);
		PointLight->SetDiffuse(PointDiffuse);
		PointLight->SetSpecular(PointSpecular);
		PointLight->SetAttenuation(PointAttenuation);
		PointLight->SetShininess(PointShininess);
	}

	ImGui::Text("Directional Light");

	ImGui::InputFloat3("Directional Direction", &DirDirection[0]);
	ImGui::SliderFloat4("Directional Ambient", &DirAmbient[0], 0.0f, 1.0f);
	ImGui::SliderFloat4("Directional Diffuse", &DirDiffuse[0], 0.0f, 1.0f);
	ImGui::SliderFloat4("Directional Specular", &DirSpecular[0], 0.0f, 1.0f);
	ImGui::SliderFloat4("Directional Attenuation", &DirAttenuation[0], 0.0f, 1.0f);
	ImGui::SliderFloat("Directional Shininess", &DirShininess, 1.0f, 128.0f);

	if (DirectionalLight != nullptr)
	{
		DirectionalLight->SetDirection(DirDirection);
		DirectionalLight->SetAmbient(DirAmbient);
		DirectionalLight->SetDiffuse(DirDiffuse);
		DirectionalLight->SetSpecular(DirSpecular);
		DirectionalLight->SetAttenuation(DirAttenuation);
		DirectionalLight->SetShininess(DirShininess);
	}

	ImGui::Checkbox("Show TBN", &bShowTBN);
	if (MeshRenderer != nullptr)
	{
		MeshRenderer->SetEnableTBNVisualization(bShowTBN);
	}

	ImGui::End();
}

void FMainWidget::OnShaderItemSelected(const std::string& NewSelectedItem)
{
	if (NewSelectedItem == "phong")
	{
		MeshRenderer->SetPipelineIndex(0);
	}
	else if (NewSelectedItem == "blinn_phong")
	{
		MeshRenderer->SetPipelineIndex(1);
	}
}

void Run(int argc, char** argv)
{
	srand(time(0));

	FConfig::Startup();

	std::string SolutionDirectory = SOLUTION_DIRECTORY;
	std::string ProjectDirectory = SolutionDirectory + PROJECT_NAME "/";

	GConfig->Set("ApplicationName", PROJECT_NAME);
	GConfig->Set("EngineName", "No Engine");
	GConfig->Set("WindowWidth", 800);
	GConfig->Set("WindowHeight", 600);
	GConfig->Set("WindowTitle", PROJECT_NAME);
	GConfig->Set("TargetFPS", 60.0f);
	GConfig->Set("MaxConcurrentFrames", 2);
	GConfig->Set("MouseSensitivity", 0.5f);
	GConfig->Set("CameraMoveSpeed", 1.0f);
	GConfig->Set("ShaderDirectory", ProjectDirectory + "shaders/");
	GConfig->Set("ImageDirectory", SolutionDirectory + "resources/images/");
	GConfig->Set("MeshDirectory", SolutionDirectory + "resources/meshes/");

	FEngine::Init();

	for (const auto& Entry : std::filesystem::directory_iterator(ProjectDirectory + "Shaders"))
	{
		std::string Filename = Entry.path().string();
		std::string Extension = Entry.path().extension().string();
		if (Extension == ".vert" || Extension == ".frag" || Extension == ".geom")
		{
			std::string Command = "glslang -g -V ";
			Command += Filename;
			Command += " -o ";
			Command += Filename + ".spv";

			system(Command.c_str());
		}
	}

	GLFWwindow* Window = GEngine->GetWindow();

	glfwSetMouseButtonCallback(Window, OnMouseButtonEvent);
	glfwSetScrollCallback(Window, OnMouseWheelEvent);
	glfwSetKeyCallback(Window, OnKeyEvent);

	FVulkanUIRenderer* UIRenderer = GEngine->GetUIRenderer();
	UIRenderer->Ready();

	std::shared_ptr<FWidget> MainWidget = std::make_shared<FMainWidget>();
	UIRenderer->AddWidget(MainWidget);

	std::string MeshDirectory;
	GConfig->Get("MeshDirectory", MeshDirectory);

	std::string ImageDirectory;
	GConfig->Get("ImageDirectory", ImageDirectory);

	FMesh* SphereMeshAsset = FAssetManager::CreateAsset<FMesh>();
	SphereMeshAsset->Load(MeshDirectory + "sphere.fbx");

	FTextureSource* BrickBaseColorTextureSource = FAssetManager::CreateAsset<FTextureSource>();
	BrickBaseColorTextureSource->Load(ImageDirectory + "Brick_BaseColor.jpg");

	FTextureSource* BrickNormalTextureSource = FAssetManager::CreateAsset<FTextureSource>();
	BrickNormalTextureSource->Load(ImageDirectory + "Brick_Normal.png");

	FTextureSource* WhiteTextureSource = FAssetManager::CreateAsset<FTextureSource>();
	WhiteTextureSource->Load(ImageDirectory + "white.png");

	std::vector<FTextureSource*> EarthTextureSources(6);
	for (int Idx = 0; Idx < 6; ++Idx)
	{
		EarthTextureSources[Idx] = FAssetManager::CreateAsset<FTextureSource>();
		EarthTextureSources[Idx]->Load(ImageDirectory + "Skybox_" + std::string(1, '0' + Idx) + ".jpg");
	}

	FWorld* World = GEngine->GetWorld();

	PointLight = World->SpawnActor<APointLightActor>();
	DirectionalLight = World->SpawnActor<ADirectionalLightActor>();

	AMeshActor* LightSourceActor = World->SpawnActor<AMeshActor>();
	LightSourceActor->SetMeshAsset(SphereMeshAsset);
	LightSourceActor->SetBaseColorTexture(WhiteTextureSource);
	LightSourceActor->SetNormalTexture(BrickNormalTextureSource);
	LightSourceActor->SetScale(glm::vec3(0.1f, 0.1f, 0.1f));

	AMeshActor* SphereActor = World->SpawnActor<AMeshActor>();
	SphereActor->SetMeshAsset(SphereMeshAsset);
	SphereActor->SetBaseColorTexture(BrickBaseColorTextureSource);
	SphereActor->SetNormalTexture(BrickNormalTextureSource);
	SphereActor->SetLocation(glm::vec3(0.0f, 0.0f, -2.0f));
	SphereActor->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

	AMeshActor* SphereActor2 = World->SpawnActor<AMeshActor>();
	SphereActor2->SetMeshAsset(SphereMeshAsset);
	SphereActor2->SetBaseColorTexture(BrickBaseColorTextureSource);
	SphereActor2->SetNormalTexture(BrickNormalTextureSource);
	SphereActor2->SetLocation(glm::vec3(4.0f, 0.0f, -2.0f));
	SphereActor2->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

	FVulkanContext* RenderContext = GEngine->GetRenderContext();
	MeshRenderer = RenderContext->CreateObject<FVulkanMeshRenderer>();
	MeshRenderer->SetPipelineIndex(1);

	ASkyActor* SkyActor = World->GetSky();
	SkyActor->SetMeshAsset(SphereMeshAsset);
	SkyActor->SetCubemap(EarthTextureSources);

	SkyRenderer = RenderContext->CreateObject<FVulkanSkyRenderer>();

	float TargetFPS;
	GConfig->Get("TargetFPS", TargetFPS);

	clock_t PreviousFrameTime = clock();
	float MaxFrameTime = 1000.0f / TargetFPS;

	float TotalFrameTime = 0.0f;
	int TotalFrameCount = 0;

	while (!glfwWindowShouldClose(Window))
	{
		clock_t CurrentFrameTime = clock();
		float DeltaTime = static_cast<float>(CurrentFrameTime - PreviousFrameTime) / CLOCKS_PER_SEC;

		glfwPollEvents();
		Update(DeltaTime);

		GEngine->Tick(DeltaTime);

		MeshRenderer->PreRender();
		SkyRenderer->PreRender();

		RenderContext->BeginRender();
		SkyRenderer->Render();
		MeshRenderer->Render();
		UIRenderer->Render();
		RenderContext->EndRender();

		PreviousFrameTime = CurrentFrameTime;

		TotalFrameTime += DeltaTime;
		++TotalFrameCount;

		if (TotalFrameTime >= 5.0f)
		{
			std::cout << "Average Frame: " << TotalFrameCount / TotalFrameTime << std::endl;
			TotalFrameTime = 0.0f;
			TotalFrameCount = 0;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds((int)(MaxFrameTime)));
	}

	RenderContext->WaitIdle();

	FEngine::Exit();
	FConfig::Shutdown();
}

void Update(float InDeltaTime)
{
	GLFWwindow* Window = GEngine->GetWindow();
	if (glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS)
	{
		return;
	}

	FWorld* World = GEngine->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	glm::vec3 RotationDelta(0.0f);

	double MouseX, MouseY;
	glfwGetCursorPos(Window, &MouseX, &MouseY);

	ACameraActor* Camera = World->GetCamera();
	if (Camera)
	{
		double MouseDeltaX = MouseX - PrevMouseX;
		double MouseDeltaY = MouseY - PrevMouseY;

		float MouseSensitivity;
		GConfig->Get("MouseSensitivity", MouseSensitivity);

		float PitchAmount = MouseDeltaY * MouseSensitivity * InDeltaTime;
		float YawAmount = -MouseDeltaX * MouseSensitivity * InDeltaTime;

		if (abs(PitchAmount) > FLT_EPSILON || abs(YawAmount) > FLT_EPSILON)
		{
			glm::quat PitchRotation = glm::angleAxis(PitchAmount, glm::vec3(1.0f, 0.0f, 0.0f));
			glm::quat YawRotation = glm::angleAxis(YawAmount, glm::vec3(0.0f, 1.0f, 0.0f));

			Camera->SetRotation(YawRotation * Camera->GetRotation() * PitchRotation);
		}

		PrevMouseX = MouseX;
		PrevMouseY = MouseY;

		glm::mat4 RotationMatrix = glm::toMat4(Camera->GetRotation());

		glm::vec4 MoveVector = RotationMatrix * glm::vec4(glm::normalize(CameraMoveDelta), 1.0f);
		glm::vec3 FinalMoveDelta(MoveVector);
		FinalMoveDelta = glm::normalize(FinalMoveDelta);

		float CameraMoveSpeed;
		GConfig->Get("CameraMoveSpeed", CameraMoveSpeed);

		if (glm::length(FinalMoveDelta) > FLT_EPSILON)
		{
			Camera->SetLocation(Camera->GetLocation() + FinalMoveDelta * CameraMoveSpeed * InDeltaTime);
		}
	}
}

int main(int argc, char** argv)
{
	try
	{
		Run(argc, argv);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

