#include "World.h"
#include "Actor.h"
#include "Engine.h"
#include "CameraActor.h"
#include "LightActor.h"
#include "MeshActor.h"

#include "VulkanContext.h"
#include "VulkanScene.h"
#include "VulkanModel.h"
#include "VulkanLight.h"
#include "VulkanCamera.h"
#include "VulkanMesh.h"
#include "VulkanTexture.h"

FWorld::FWorld()
	: CameraActor(nullptr)
	, LightActor(nullptr)
	, RenderScene(nullptr)
{
	CameraActor = SpawnActor<ACameraActor>();
	LightActor = SpawnActor<ALightActor>();
}

FWorld::~FWorld()
{
	for (AActor* Actor : Actors)
	{
		if (Actor != nullptr)
		{
			Actor->Deinitialize();
			delete Actor;
		}
	}
}

void FWorld::DestroyActor(AActor* InActor)
{
	for (int Idx = 0; Idx < Actors.size(); ++Idx)
	{
		if (Actors[Idx] == InActor)
		{
			InActor->Deinitialize();
			Actors.erase(Actors.begin() + Idx);

			delete InActor;
			break;
		}
	}
}

void FWorld::Tick(float DeltaTime)
{
	if (RenderScene == nullptr)
	{
		GenerateRenderScene();
	}

	for (AActor* Actor : Actors)
	{
		if (Actor == nullptr)
		{
			continue;
		}

		Actor->Tick(DeltaTime);
	}

	UpdateRenderScene();
}

const std::vector<class AActor*>& FWorld::GetActors()
{
	return Actors;
}

ACameraActor* FWorld::GetCamera() const
{
	return CameraActor;
}

ALightActor* FWorld::GetLight() const
{
	return LightActor;
}

FVulkanScene* FWorld::GetRenderScene() const
{
	return RenderScene;
}

void FWorld::GenerateRenderScene()
{
	assert(GEngine != nullptr);

	FVulkanContext* RenderContext = GEngine->GetRenderContext();
	if (RenderContext == nullptr)
	{
		return;
	}

	RenderScene = RenderContext->CreateObject<FVulkanScene>();

	for (AActor* Actor : Actors)
	{
		if (Actor == nullptr)
		{
			continue;
		}

		if (Actor->GetTypeId() != AMeshActor::StaticTypeId())
		{
			continue;
		}

		AMeshActor* MeshActor = Cast<AMeshActor>(Actor);
		if (MeshActor == nullptr)
		{
			continue;
		}

		FMesh* MeshAsset = MeshActor->GetMeshAsset();
		if (MeshAsset == nullptr)
		{
			continue;
		}

		FVulkanModel* NewModel = RenderContext->CreateObject<FVulkanModel>();

		RenderScene->AddModel(NewModel);
		RenderModelMap[Actor] = NewModel;

		FVulkanMesh* NewMesh = RenderContext->CreateObject<FVulkanMesh>();
		NewMesh->Load(MeshAsset);

		FTextureSource* BaseColorTexture = MeshActor->GetBaseColorTexture();
		if (BaseColorTexture != nullptr)
		{
			FVulkanTexture* NewTexture = RenderContext->CreateObject<FVulkanTexture>();
			NewTexture->LoadSource(BaseColorTexture);
			NewMesh->SetBaseColorTexture(NewTexture);
		}

		FTextureSource* NormalTexture = MeshActor->GetNormalTexture();
		if (NormalTexture != nullptr)
		{
			FVulkanTexture* NewTexture = RenderContext->CreateObject<FVulkanTexture>();
			NewTexture->LoadSource(NormalTexture);
			NewMesh->SetNormalTexture(NewTexture);
		}

		NewModel->SetMesh(NewMesh);
	}
}

void FWorld::UpdateRenderScene()
{	
	assert(GEngine != nullptr);

	if (RenderScene == nullptr)
	{
		return;
	}

	if (LightActor != nullptr)
	{
		FVulkanLight Light;
		Light.Position = LightActor->GetLocation();
		Light.Ambient = LightActor->GetAmbient();
		Light.Diffuse = LightActor->GetDiffuse();
		Light.Specular = LightActor->GetSpecular();
		Light.Attenuation = LightActor->GetAttenuation();
		Light.Shininess = LightActor->GetShininess();

		RenderScene->SetLight(Light);
	}

	if (CameraActor != nullptr)
	{
		FVulkanCamera Camera;
		Camera.Position = CameraActor->GetLocation();
		Camera.FOV = CameraActor->GetFOV();
		Camera.Far = CameraActor->GetFar();
		Camera.Near = CameraActor->GetNear();
		Camera.View = CameraActor->GetViewMatrix();

		RenderScene->SetCamera(Camera);
	}

	for (AActor* Actor : Actors)
	{
		if (Actor == nullptr)
		{
			continue;
		}

		auto ModelItr = RenderModelMap.find(Actor);
		if (ModelItr == RenderModelMap.end())
		{
			continue;
		}

		FVulkanModel* Model = ModelItr->second;
		if (Model == nullptr)
		{
			continue;
		}

		Model->SetModelMatrix(Actor->GetCachedModelMatrix());
	}
}
