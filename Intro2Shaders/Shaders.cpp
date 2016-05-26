
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
	pvr::api::BufferView rotateUbo;
	glm::float32 rotateValue;
};




pvr::Result::Enum OGLESIntroducingPVRApi::initView() {

	// Short hand variable for graphics context object
	gc = getGraphicsContext();

	// Get our on screen frame buffers - we create a command buffer for each
	onscreenFBArray = gc->createOnScreenFboSet();

	// Setup our vertex buffer with a triangle

	vertexBuffer = gc->createBuffer(sizeof(glm::float32) * 3 * 3, pvr::types::BufferBindingUse::VertexBuffer);

	// Co-ordinate systems differ between Vulkan and OpenGL ES (y axis flipped)
	// Note that asset loading code in part 3 abstracts this for us - only needed for raw vertex data

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

	float * triangleVertices = 
		(pvr::Api::Enum::Vulkan == gc->getApiType()) ? vulkanTriangle: glTriangle;

	vertexBuffer->update(triangleVertices, 0, sizeof(vulkanTriangle));


	/* Create our pipeline parameter object which will be passed to createPipeline

	As in the blog post to create a pipeline we need to provide:

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

	pvr::api::GraphicsPipelineCreateParam opaquePipeParameters = pvr::api::GraphicsPipelineCreateParam();

	// Set up the vertex layout for our pipeline

	opaquePipeParameters.vertexInput.addVertexAttribute(0, pvr::api::VertexAttributeInfo(0, pvr::types::DataType::Float32, 3, 0, "inVertex"));

	opaquePipeParameters.vertexInput.setInputBinding(0, sizeof(glm::float32) * 3);

	opaquePipeParameters.inputAssembler.setPrimitiveTopology(pvr::types::PrimitiveTopology::TriangleList);


	// Set up shaders for our pipeline 

	// Build system automatically compiles shader to spir-v binary for us on Vulkan
	pvr::api::Shader vert = gc->createShader(
		*getAssetStream("shader.vert"),
		pvr::types::ShaderType::VertexShader);

	pvr::api::Shader frag = gc->createShader(
		*getAssetStream("shader.frag"),
		pvr::types::ShaderType::FragmentShader);

	opaquePipeParameters.fragmentShader.setShader(frag);
	opaquePipeParameters.vertexShader.setShader(vert);
		
	pvr::api::DescriptorSet uboDescriptorSet;

	// Set up our uniform for rotating the triangle - we use a different path for pre ES 3.1 by checking getApiType() for Vulkan
	if (pvr::Api::Enum::Vulkan == gc->getApiType()) {

		// Create a UBO
		rotateUbo = gc->createBufferAndView(4, pvr::types::BufferBindingUse::UniformBuffer);
		rotateUbo->update(&rotateValue, 0, 4);

		// Create a descriptor set layout for the uniform
		pvr::api::DescriptorSetLayout uboDescriptSetLayout = gc->createDescriptorSetLayout(pvr::api::DescriptorSetLayoutCreateParam().setBinding(0, pvr::types::DescriptorType::UniformBuffer, 1, pvr::types::ShaderStageFlags::Vertex));

		// Attach this layout to the pipeline
		pvr::api::PipelineLayout uboPipelineLayout = gc->createPipelineLayout(pvr::api::PipelineLayoutCreateParam().addDescSetLayout(uboDescriptSetLayout));

		opaquePipeParameters.pipelineLayout = uboPipelineLayout;

		// Create the descriptor set we will bind to this layout when drawing
		uboDescriptorSet = gc->createDescriptorSetOnDefaultPool(uboDescriptSetLayout);

		// Point the descriptor set to point to our UBO
		uboDescriptorSet->update(pvr::api::DescriptorSetUpdate().setUbo(0, rotateUbo));

		// Set an initial value for the UBO
		rotateUbo->update(&rotateValue, 0, 4);
	}else{
		// No descriptor sets required in this case on pre ES 3.1 path so we use a default pipeline layout (empty)
		opaquePipeParameters.pipelineLayout = pvr::api::PipelineLayout();
	}
	
	// Set up render pass and blend state 
	opaquePipeParameters.colorBlend.setAttachmentState(0, pvr::api::pipelineCreation::ColorBlendAttachmentState()); 
	opaquePipeParameters.renderPass = onscreenFBArray[0]->getRenderPass();

	// With the pipeline fully configured we now create the pipeline object
	opaquePipeline = gc->createGraphicsPipeline(opaquePipeParameters);


	// For pre ES3.1 we now have linked shaders so can call getUniformLocation to set up the rotate uniform
	int rotateLoc = 0;

	if (!(gc->getApiType() == pvr::Api::Enum::Vulkan)) {
		rotateLoc = opaquePipeline->getUniformLocation("rotate");
		
	}
	

	for (int i = 0 ; i < gc->getPlatformContext().getSwapChainLength(); i++){
			// Create a command buffer for each frame buffer in swap chain
			onscreenCBArray.add(gc->createCommandBufferOnDefaultPool()); 
			pvr::api::CommandBuffer & cb= onscreenCBArray[i];

			// Begin recording to the command buffer
			cb->beginRecording();

			// Begin the render pass using our FBO object - this contains the render pass settings
			cb->beginRenderPass(onscreenFBArray[i], 
				pvr::Rectanglei(0, 0, getWidth(), getHeight()),
				false);

			// Bind the pipeline object
			cb->bindPipeline(opaquePipeline);

			// Update our uniform for Vulkan and ES 2.0/3.0/3.1 paths
			if (gc->getApiType() == pvr::Api::Enum::Vulkan) {
				cb->bindDescriptorSet(opaquePipeline->getPipelineLayout(), 0, uboDescriptorSet);
			}else{
				// This function will poll the pointed to memory to set the uniform for each draw
				cb->setUniformPtr(rotateLoc, 1, &rotateValue);
			}
			
			// Bind our vertex buffer
			cb->bindVertexBuffer(vertexBuffer, 0, 0);

			// Draw our triangle 
			cb->drawArrays(0, 3,0,1);

			cb->endRenderPass();
			cb->endRecording();
	}

	return pvr::Result::Success;
};

pvr::Result::Enum OGLESIntroducingPVRApi::renderFrame() {

	// ES 2.0+ uniform will automatically be polled - we just change the underlying memory
	rotateValue += 0.03;

	// For Vulkan UBO we update the buffer 
	if (pvr::Api::Enum::Vulkan == gc->getApiType()) {
		rotateUbo->update(&rotateValue, 0, 4);
	}

	// Submit the required command buffer for the next surface to be displayed
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
