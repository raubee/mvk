#include "AppBase.h"
#include "Model.h"
#include "Texture2D.h"
#include "BaseMaterial.h"
#include "GraphicPipeline.h"

class ObjViewer : public mvk::AppBase
{
	mvk::Skybox skybox;

	struct Textures
	{
		mvk::Texture2D albedo;
		mvk::Texture2D normal;
		mvk::Texture2D roughness;
	}
	textures;

	struct Materials
	{
		mvk::BaseMaterial standard;
	}
	materials;

	struct Models
	{
		mvk::Model ganesh;
	}
	models;

	struct GraphicPipelines
	{
		mvk::GraphicPipeline standard;
	}
	pipelines;

public:
	ObjViewer(): AppBase(mvk::AppInfo{
		.appName = "ObjViewer"
	})
	{
		scene.camera.setPerspective(45.0f, float(width) / float(height),
		                            0.1f, 100.0f);

		/*scene.camera.setType(Camera::CameraType::FLY);
		scene.camera.setPosition(glm::vec3(0.0f, 0.0f, -2.0f));*/
		scene.camera.setType(Camera::CameraType::ORBIT);
		scene.camera.setLookAt(glm::vec3(0.0f, 0.5f, 0.0f));
		scene.camera.setDistance(1.5f);

		const std::array<std::string, 6> skyboxTexturePaths = {
			"assets/textures/skybox/right.jpg",
			"assets/textures/skybox/left.jpg",
			"assets/textures/skybox/top.jpg",
			"assets/textures/skybox/bottom.jpg",
			"assets/textures/skybox/front.jpg",
			"assets/textures/skybox/back.jpg"
		};

		skybox.create(&device, transferQueue, renderPass.renderPass,
		              skyboxTexturePaths);

		scene.setup(&device, &skybox);

		const auto modelPath = "assets/models/ganesha/ganesha.obj";
		const auto albedoPath =
			"assets/models/ganesha/textures/Ganesha_BaseColor.jpg";
		const auto normalPath =
			"assets/models/ganesha/textures/Ganesha_Normal.jpg";
		const auto roughnessPath =
			"assets/models/ganesha/textures/Ganesha_Roughness.jpg";

		models.ganesh.loadFromFile(&device, transferQueue, modelPath);

		textures.albedo.loadFromFile(&device, transferQueue, albedoPath,
		                             vk::Format::eR8G8B8A8Unorm);

		textures.normal.loadFromFile(&device, transferQueue, normalPath,
		                             vk::Format::eR8G8B8A8Unorm);

		textures.roughness.loadFromFile(&device, transferQueue, roughnessPath,
		                                vk::Format::eR8G8B8A8Unorm);

		mvk::BaseMaterial::BaseMaterialDescription description{
			.constants{
				.baseColorTextureSet = 0,
				.normalTextureSet = 0,
			},
			.baseColor = &textures.albedo,
			.normal = &textures.normal,
			.metallicRoughness = &textures.roughness,
		};

		materials.standard.load(&device, description);

		const std::vector<vk::VertexInputBindingDescription>
			bindingDescription{
				mvk::Vertex::getBindingDescription()
			};

		const auto& attributeDescriptions =
			mvk::Vertex::getAttributeDescriptions();

		const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{
			scene.descriptorSetLayout,
			mvk::Model::getDescriptorSetLayout(&device),
			mvk::BaseMaterial::getDescriptorSetLayout(&device)
		};

		const std::vector<vk::PushConstantRange> pushConstantRanges{
			materials.standard.getPushConstantRange()
		};

		const auto shaderStageInfo =
			materials.standard.getPipelineShaderStageCreateInfo();

		const mvk::GraphicPipelineCreateInfo opaquePipelineCreateInfo{
			.vertexInputBindingDescription = bindingDescription,
			.vertexInputAttributeDescription = attributeDescriptions,
			.renderPass = renderPass.renderPass,
			.shaderStageCreateInfos = shaderStageInfo,
			.descriptorSetLayouts = descriptorSetLayouts,
			.pushConstantRanges = pushConstantRanges,
			.frontFace = vk::FrontFace::eCounterClockwise,
		};

		pipelines.standard.build(&device, opaquePipelineCreateInfo);
	}

	~ObjViewer()
	{
		skybox.release();
		textures.albedo.release();
		textures.normal.release();
		textures.roughness.release();
		models.ganesh.release();
		materials.standard.release();
		pipelines.standard.release();
	}

	void buildCommandBuffer(const vk::CommandBuffer commandBuffer,
	                        const vk::Framebuffer framebuffer) override
	{
		const auto extent = swapchain.getSwapchainExtent();

		const vk::CommandBufferBeginInfo commandBufferBeginInfo{};
		commandBuffer.begin(commandBufferBeginInfo);

		const std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
		std::array<vk::ClearValue, 2> clearValues{};
		clearValues[0].setColor(clearColor);
		clearValues[1].setDepthStencil({1.0f, 0});

		const vk::RenderPassBeginInfo renderPassBeginInfo = {
			.renderPass = renderPass.renderPass,
			.framebuffer = framebuffer,
			.renderArea = {
				.offset = {0, 0},
				.extent = extent
			},
			.clearValueCount = static_cast<uint32_t>(clearValues.size()),
			.pClearValues = clearValues.data()
		};

		commandBuffer.beginRenderPass(renderPassBeginInfo,
		                              vk::SubpassContents::eInline);

		const vk::Viewport viewport = {
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(extent.width),
			.height = static_cast<float>(extent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		commandBuffer.setViewport(0, viewport);

		const vk::Rect2D scissor = {
			.offset = {0, 0},
			.extent = extent
		};

		commandBuffer.setScissor(0, scissor);

		scene.renderSkybox(commandBuffer);

		const auto graphicPipeline = pipelines.standard;
		const auto pipelineLayout = graphicPipeline.getPipelineLayout();
		const auto pipeline = graphicPipeline.getPipeline();

		for (const auto& node : models.ganesh.nodes)
		{
			std::vector<vk::DescriptorSet> descriptorSets = {
				scene.getDescriptorSet(0),
				node->getDescriptorSet(),
				materials.standard.getDescriptorSet()
			};

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
			                           pipeline);

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			                                 pipelineLayout, 0,
			                                 static_cast<uint32_t>(
				                                 descriptorSets.size()),
			                                 descriptorSets.data(), 0, nullptr);

			commandBuffer.pushConstants(pipelineLayout,
			                            vk::ShaderStageFlagBits::eFragment,
			                            0, sizeof(mvk::BaseMaterial::
				                            PushConstants),
			                            &materials.standard.constants);

			const auto vertexBuffer = models.ganesh.vertexBuffer;
			const auto indexBuffer = models.ganesh.indexBuffer;

			vk::DeviceSize offsets[] = {0};

			commandBuffer.
				bindVertexBuffers(0, 1, &vertexBuffer.buffer, offsets);

			commandBuffer.bindIndexBuffer(indexBuffer.buffer, 0,
			                              vk::IndexType::eUint32);


			const auto size = node->indexCount;
			const auto indexStart = node->startIndex;

			commandBuffer.drawIndexed(size, 1, indexStart, 0, 0);
		}

		commandBuffer.endRenderPass();
		commandBuffer.end();
	}
};

ObjViewer* objViewer;

int main()
{
	objViewer = new ObjViewer();
	objViewer->run();
	delete objViewer;
	return EXIT_SUCCESS;
}
