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
	pvr::GraphicsContext gc;
	pvr::Multi<pvr::api::Fbo> onscreenFBArray;
	pvr::Multi<pvr::api::CommandBuffer> onscreenCBArray;
	pvr::api::GraphicsPipeline opaquePipeline;
	pvr::api::Buffer vertexBuffer;
};




pvr::Result::Enum OGLESIntroducingPVRApi::initView() {
	gc = getGraphicsContext();
	onscreenFBArray = gc->createOnScreenFboSet();

	pvr::api::GraphicsPipelineCreateParam opaquePipeParameters = pvr::api::GraphicsPipelineCreateParam();

	pvr::api::Shader vert = gc->createShader(
		*getAssetStream("shader.vert"),
		pvr::types::ShaderType::VertexShader);

	pvr::api::Shader frag = gc->createShader(
		*getAssetStream("shader.frag"),
		pvr::types::ShaderType::FragmentShader);

	opaquePipeParameters.fragmentShader.setShader(frag);
	opaquePipeParameters.vertexShader.setShader(vert);

	// Setup triangle

	vertexBuffer = gc->createBuffer(sizeof(glm::float32) * 3 * 3, pvr::types::BufferBindingUse::VertexBuffer);

	float vertPos = 0.8;

	float vulkanTriangle[] = {
		0.0, -vertPos, 0.0,
		-vertPos, vertPos, 0.0,
		vertPos, vertPos, 0.0,
	};

	float glTriangle[] = {
		0.0, vertPos, 0.0,
		-vertPos, -vertPos, 0.0,
		vertPos, -vertPos, 0.0
	};

	float * triangleVertices = (pvr::Api::Enum::Vulkan == gc->getApiType()) ? vulkanTriangle: glTriangle;


	vertexBuffer->update(triangleVertices, 0, sizeof(vulkanTriangle));

	opaquePipeParameters.vertexInput.addVertexAttribute(0, pvr::api::VertexAttributeInfo(0, pvr::types::DataType::Float32, 3, 0, "inVertex"));

	opaquePipeParameters.vertexInput.setInputBinding(0, sizeof(glm::float32)*3);

	opaquePipeParameters.inputAssembler.setPrimitiveTopology(pvr::types::PrimitiveTopology::TriangleList);


	// Set render pass and blend state 

	opaquePipeParameters.colorBlend.setAttachmentState(0, pvr::api::pipelineCreation::ColorBlendAttachmentState()); 

	opaquePipeParameters.renderPass = onscreenFBArray[0]->getRenderPass();

	opaquePipeParameters.pipelineLayout = gc->createPipelineLayout(pvr::api::PipelineLayoutCreateParam()); 

	opaquePipeline = gc->createGraphicsPipeline(opaquePipeParameters);

	for (int i = 0 ; i < gc->getPlatformContext().getSwapChainLength(); i++){
			onscreenCBArray.add(gc->createCommandBufferOnDefaultPool());
			pvr::api::CommandBuffer & cb= onscreenCBArray[i];
			cb->beginRecording();
			cb->beginRenderPass(onscreenFBArray[i], 
				pvr::Rectanglei(0, 0, getWidth(), getHeight()), 
				false, 
				glm::vec4(123.0/255.0, 172.0/255.0, 189.0/255.0, 1.0));

			cb->bindPipeline(opaquePipeline);
			cb->bindVertexBuffer(vertexBuffer, 0, 0);
			cb->drawArrays(0, 3,0,1);

			cb->endRenderPass();
			cb->endRecording();
	}

	return pvr::Result::Success;
};

pvr::Result::Enum OGLESIntroducingPVRApi::renderFrame() {

	onscreenCBArray[gc->getPlatformContext().getSwapChainIndex()]->submit();

	return pvr::Result::Success;
};



pvr::Result::Enum OGLESIntroducingPVRApi::releaseView() {
	return pvr::Result::Success;
};
pvr::Result::Enum OGLESIntroducingPVRApi::quitApplication() {
	return pvr::Result::Success;
};

pvr::Result::Enum OGLESIntroducingPVRApi::initApplication() {
	return pvr::Result::Success;
};


/*!*********************************************************************************************************************
\brief	This function must be implemented by the user of the shell. The user should return its pvr::Shell object defining the behaviour of the application.
\return Return an auto ptr to the demo supplied by the user
***********************************************************************************************************************/
std::auto_ptr<pvr::Shell> pvr::newDemo(){ return std::auto_ptr<pvr::Shell>(new OGLESIntroducingPVRApi()); }
