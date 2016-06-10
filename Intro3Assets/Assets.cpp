/*!*********************************************************************************************************************
\File         OGLESIntroducingPVRApi.cpp
\Title        Introducing the PowerVR Framework
\Author       PowerVR by Imagination, Developer Technology Team
\Copyright    Copyright (c) Imagination Technologies Limited.
\brief		  Shows how to use the PVRApi library together with loading models from POD files and rendering them with effects from PFX files.
***********************************************************************************************************************/
#include "PVRShell/PVRShell.h"
#include "PVRApi/PVRApi.h"

#include "PVRCore/PVRCore.h"
#include "PVRUIRenderer/PVRUIRenderer.h"


class OGLESIntroducingPVRApi : public pvr::Shell
{

public:
	virtual pvr::Result::Enum initApplication();
	virtual pvr::Result::Enum initView();
	virtual pvr::Result::Enum releaseView();
	virtual pvr::Result::Enum quitApplication();
	virtual pvr::Result::Enum renderFrame(); 
	
private:
	glm::float32 gnomeRotate;
	pvr::Multi<pvr::api::Fbo> onscreenFBOArray;
	pvr::Multi<pvr::api::CommandBuffer> onscreenCBArray;

	pvr::api::GraphicsPipeline onscreenOpaqSamplePipeline;
	pvr::api::Fbo offscreenFBO;

	pvr::api::BufferView gnomeRotateUBO;

	pvr::api::AssetStore assets;

	pvr::api::GraphicsPipeline gnomeOpaqPipeline;

	pvr::api::Buffer gnomeVBO;
	pvr::api::Buffer gnomeIBO;
	pvr::api::TextureView gnomeTex;
	
	
};

pvr::Result::Enum OGLESIntroducingPVRApi::initApplication() {
	return pvr::Result::Success;
};

pvr::Result::Enum OGLESIntroducingPVRApi::initView() {

	// Short hand variable for graphics context object
	pvr::GraphicsContext gc = getGraphicsContext();

	// Get our on screen frame buffers - we create a command buffer for each
	onscreenFBOArray = gc->createOnScreenFboSet();


	/* Create our pipeline parameter object which will be passed to createPipeline

	We need to provide:

	Render pass information
	Vertex layout information
	Shaders
	Shader layout information through descriptor set layouts

	With a valid pipeline created, to issue a draw we require:

	A vertex buffer matching the vertex layout to bind data to draw with
	Descriptor sets matching descript set layouts to bind data to our shaders
	Note that to handle pre ES 3.1 paths we also need to provide paths to update uniforms without descriptor sets (no UBO)

	*/

	// Create a pipeline

	pvr::api::GraphicsPipelineCreateParam gnomeOpaqPipelineParam = pvr::api::GraphicsPipelineCreateParam();

	// Load gnome asset

	// Asset manager initialisation 
	assets.init(*this); 

	assets.getTextureWithCaching(getGraphicsContext(), pvr::StringHash("gnome_tex_small.pvr"), &gnomeTex, NULL);

	pvr::assets::ModelHandle gnomeModelH = pvr::assets::Model::createWithReader(pvr::assets::PODReader(getAssetStream("gnome0.pod")));

	// This call creates interleaved VBO and IBO buffers in one function call
	pvr::utils::createSingleBuffersFromMesh(getGraphicsContext(), gnomeModelH->getMesh(0), gnomeVBO, gnomeIBO);

	// We then specify how the attributes match up in our shader

	// For GLES 3.1+ we use the layout specifier 
	pvr::utils::VertexBindings attributeBindings[] =
	{
		{ "POSITION", 0 },
		{ "NORMAL", 1 },
		{ "UV0", 2 },
	};

	// We also provide name based binding for Pre GLES 3.1
	pvr::utils::VertexBindings_Name attributeBindingNames[] =
	{
		{ "POSITION", "inVertex" },
		{ "NORMAL", "inNormal" },
		{ "UV0", "inUV" },
	};


	// We call the appropriate version of createInputAssemblyFromMesh which configures the vertex input format for us
	if (getApiType() == pvr::Api::Enum::Vulkan || getApiType() == pvr::Api::Enum::OpenGLES31)
	{
		pvr::utils::createInputAssemblyFromMesh(gnomeModelH->getMesh(0), attributeBindings, 3, gnomeOpaqPipelineParam);
	}else {
		pvr::utils::createInputAssemblyFromMesh(gnomeModelH->getMesh(0), attributeBindingNames, 3, gnomeOpaqPipelineParam);
	}

	// Set our render pass
	gnomeOpaqPipelineParam.renderPass = onscreenFBOArray[0]->getRenderPass();


	/*
		Create our shader passing in our API version - the #version string is replaced in shader source with API glsl version
		By using __VERSION__ in the source we can write a single shader that works for Vulkan, ES 2.0/3.0/3.1
	*/
	pvr::api::Shader gnomeVertShader = gc->createShader(*getAssetStream("gnome_shader.vert"), pvr::types::ShaderType::VertexShader, 0, 0, getApiType());
	pvr::api::Shader gnomeFragShader = gc->createShader(*getAssetStream("gnome_shader.frag"), pvr::types::ShaderType::FragmentShader, 0, 0, getApiType());

	gnomeOpaqPipelineParam.fragmentShader.setShader(gnomeFragShader);
	gnomeOpaqPipelineParam.vertexShader.setShader(gnomeVertShader);


	// Set our blend state and depth/stencil test
	gnomeOpaqPipelineParam.colorBlend.setAttachmentState(0, pvr::api::pipelineCreation::ColorBlendAttachmentState());
	gnomeOpaqPipelineParam.depthStencil.setDepthTestEnable(true);
	gnomeOpaqPipelineParam.depthStencil.setDepthWrite(true);
	gnomeOpaqPipelineParam.depthStencil.setStencilTest(false);

	// Create a pipeline and descriptor set layout which contains a UBO and a ImageSampler for our texture
	pvr::api::DescriptorSetLayout sampleUBODSLayout = gc->createDescriptorSetLayout(pvr::api::DescriptorSetLayoutCreateParam()
		.setBinding(0, pvr::types::DescriptorType::UniformBuffer, 1, pvr::types::ShaderStageFlags::Vertex)
		.setBinding(1, pvr::types::DescriptorType::CombinedImageSampler, 1, pvr::types::ShaderStageFlags::Fragment));

	pvr::api::PipelineLayout sampleUBOPLLayout = gc->createPipelineLayout(pvr::api::PipelineLayoutCreateParam().addDescSetLayout(sampleUBODSLayout));

	gnomeOpaqPipelineParam.pipelineLayout = sampleUBOPLLayout;

	// Create our rotation UBO for Vulkan/OpenGL ES 3.1+

	gnomeRotateUBO = gc->createBufferAndView(4, pvr::types::BufferBindingUse::UniformBuffer);

	gnomeRotate = 0.0;
	gnomeRotateUBO->update(&gnomeRotate, 0, 4);

	// Create our texture descriptor set
	pvr::api::DescriptorSet samplerUboDescSet = gc->createDescriptorSetOnDefaultPool(sampleUBODSLayout);
	
	pvr::api::Sampler simpleSampler = gc->createSampler(pvr::api::SamplerCreateParam(pvr::types::SamplerFilter::Linear,
		pvr::types::SamplerFilter::Linear,
		pvr::types::SamplerFilter::None)
		);

	// Create texture and UBO descript set update - we pass in the uniform name for the sampler to support pre ES 3.1 paths
	samplerUboDescSet->update(pvr::api::DescriptorSetUpdate().setUbo(0, gnomeRotateUBO).setCombinedImageSampler(1, gnomeTex, simpleSampler, "sTexture"));

	// Create our pipeline
	gnomeOpaqPipeline = gc->createGraphicsPipeline(gnomeOpaqPipelineParam);

	// Retrieve the uniform location for pre ES 3.1 path
	int uniformLoc = gnomeOpaqPipeline->getUniformLocation("scale");

	for (int i = 0 ; i< getPlatformContext().getSwapChainLength(); i++){

		// Create a command buffer for each frame buffer in swap chain
		onscreenCBArray.add(gc->createCommandBufferOnDefaultPool());
		pvr::api::CommandBuffer cb = onscreenCBArray[i];
		pvr::api::Fbo fb = onscreenFBOArray[i];

		// Begin recording to the command buffer
		cb->beginRecording();
		cb->beginRenderPass(fb, pvr::Rectanglei(0, 0, getWidth(), getHeight()), false,
			glm::vec4(123.0 / 255.0, 150.0 / 255.0, 189.0 / 255.0, 1.0));
			
		cb->bindPipeline(gnomeOpaqPipeline);

		// Bind our descriptor set for our UBO and texture 
		cb->bindDescriptorSet(gnomeOpaqPipeline->getPipelineLayout(), 0, samplerUboDescSet);

		// For pre ES 3.1 API set the uniform pointer
		if (!(getApiType() == pvr::Api::Enum::Vulkan || getApiType() == pvr::Api::Enum::OpenGLES31)) {
			cb->setUniformPtr(uniformLoc, 1, &gnomeRotate);
		}
			
		// Bind our vertex and index buffer and issue our draw
		cb->bindVertexBuffer(gnomeVBO, 0, 0);
		cb->bindIndexBuffer(gnomeIBO, 0, gnomeModelH->getMesh(0).getFaces().getDataType());
		cb->drawIndexed(0, gnomeModelH->getMesh(0).getNumIndices());
			
		cb->endRenderPass();
		cb->endRecording();

	}

	return pvr::Result::Success;
};
pvr::Result::Enum OGLESIntroducingPVRApi::releaseView() {
	return pvr::Result::Success;
};
pvr::Result::Enum OGLESIntroducingPVRApi::quitApplication() {
	return pvr::Result::Success;
};
pvr::Result::Enum OGLESIntroducingPVRApi::renderFrame() {
	gnomeRotate += 0.03;

	//glm::float32 abf(fabs(gnomeRotate);
	gnomeRotateUBO->update(&gnomeRotate, 0, 4);

	onscreenCBArray[getPlatformContext().getSwapChainIndex()]->submit();
	return pvr::Result::Success;
};



/*!*********************************************************************************************************************
\brief	This function must be implemented by the user of the shell. The user should return its pvr::Shell object defining the behaviour of the application.
\return Return an auto ptr to the demo supplied by the user
***********************************************************************************************************************/
std::auto_ptr<pvr::Shell> pvr::newDemo(){ return std::auto_ptr<pvr::Shell>(new OGLESIntroducingPVRApi()); }
