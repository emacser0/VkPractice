#include "MeshActorBase.h"

#include "VulkanModel.h"

AMeshActorBase::AMeshActorBase()
	: AActor()
	, MeshAsset(nullptr)
	, RenderModel(nullptr)
{
}

void AMeshActorBase::UpdateRenderModel()
{	
	if (RenderModel == nullptr)
	{
		CreateRenderModel();
	}

	RenderModel->SetModelMatrix(GetCachedModelMatrix());
}