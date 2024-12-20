#pragma once

#include "VulkanObject.h"
#include "VulkanImage.h"

#include "vulkan/vulkan.h"

#include <vector>

class FVulkanTexture : public FVulkanObject
{
public:
	FVulkanTexture(FVulkanContext* InContext);

	virtual void Destroy() override;

	void Load(class UTexture2D* InTexture);
	void Load(class UTextureCube* InTexture);

	void Unload();

	uint32_t GetWidth() const { return Width; }
	uint32_t GetHeight() const { return Height; }
	VkFormat GetFormat() const { return Format; }
	FVulkanImage* GetImage() const { return Image; }

	void SetFormat(VkFormat InFormat) { Format = InFormat; }

private:
	class FVulkanImage* Image;

	uint32_t Width;
	uint32_t Height;
	uint32_t Channel;
	uint32_t Depth;
	VkFormat Format;
};
