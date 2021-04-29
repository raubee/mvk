#include "SwapchainFrame.h"
#include "UniformBufferObject.h"

using namespace mvk;

void SwapchainFrame::create(const vk::Device device,
                            const vk::Image image,
                            const vk::RenderPass renderPass,
                            const vk::Format swapchainFormat,
                            const vk::Extent2D swapchainExtent,
                            const vk::ImageView depthImage)
{
	createImageView(device, image, swapchainFormat);
	createFramebuffer(device, depthImage, renderPass, swapchainExtent);
}

void SwapchainFrame::release(const vk::Device device) const
{
	if (framebuffer)
		device.destroy(framebuffer);

	if (imageView)
		device.destroy(imageView);
}

void SwapchainFrame::createImageView(const vk::Device device,
                                     vk::Image image,
                                     vk::Format format)
{
	const auto imageViewCreateInfo = vk::ImageViewCreateInfo{
		.image = image,
		.viewType = vk::ImageViewType::e2D,
		.format = format,
		.components = {
			.r = vk::ComponentSwizzle::eIdentity,
			.g = vk::ComponentSwizzle::eIdentity,
			.b = vk::ComponentSwizzle::eIdentity,
			.a = vk::ComponentSwizzle::eIdentity,
		},
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	imageView = device.createImageView(imageViewCreateInfo);
}

void SwapchainFrame::createFramebuffer(const vk::Device device,
                                       const vk::ImageView depthImageView,
                                       const vk::RenderPass renderPass,
                                       const vk::Extent2D extent)
{
	std::array<vk::ImageView, 2> attachments = {imageView, depthImageView};

	const vk::FramebufferCreateInfo framebufferCreateInfo = {
		.renderPass = renderPass,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.width = extent.width,
		.height = extent.height,
		.layers = 1
	};

	framebuffer = device.createFramebuffer(framebufferCreateInfo);
}