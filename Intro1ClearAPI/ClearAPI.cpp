
#include "PVRShell/PVRShell.h"
#include "PVRApi/PVRApi.h"

class OGLESIntroducingPVRApi : public pvr::Shell
{
public:
	pvr::Multi<pvr::api::Fbo> onscreenFB;
	pvr::Multi<pvr::api::CommandBuffer> onscreenCBArray;

	virtual pvr::Result::Enum initApplication(){return pvr::Result::Success;};
	virtual pvr::Result::Enum initView();
	virtual pvr::Result::Enum releaseView(){return pvr::Result::Success;};
	virtual pvr::Result::Enum quitApplication(){return pvr::Result::Success;};
	virtual pvr::Result::Enum renderFrame();
};

pvr::Result::Enum OGLESIntroducingPVRApi::initView() {

	onscreenFB = getGraphicsContext()->createOnScreenFboSet();

	for (int i = 0 ; i < getSwapChainLength(); i++){
		onscreenCBArray.add(getGraphicsContext()->createCommandBufferOnDefaultPool());

		auto & cb = onscreenCBArray[i];
		auto & frameBuffer = onscreenFB[i];

		cb->beginRecording();
		cb->beginRenderPass(frameBuffer, pvr::Rectanglei(0, 0, getWidth(), getHeight()),false, 
			glm::vec4(123.0 / 255.0, 172.0 / 255.0, 189.0 / 255.0, 1.0));
		cb->endRenderPass();
		cb->endRecording();

		
	}

	return pvr::Result::Success;
};

pvr::Result::Enum OGLESIntroducingPVRApi::renderFrame(){

	onscreenCBArray[getSwapChainIndex()]->submit();

	return pvr::Result::Success;
}

std::auto_ptr<pvr::Shell> pvr::newDemo(){ return std::auto_ptr<pvr::Shell>(new OGLESIntroducingPVRApi()); }
