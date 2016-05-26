#pragma once

#include "Animation.h"
#include "Configurable.h"
#include "BitMultimedia.h"
#include "CinderOpenCV.h"
#include "cinder/gl/Fbo.h"

struct VideoInfoPixel
{
	std::string title;
	Bit::Video video;
};

#define RES_PIXELEFFECT_VERT "shaders/pixeleffect1.vert"
#define RES_PIXELEFFECT_FRAG "shaders/pixeleffect1.frag"


class PixelEffect1 : public Animation
{
public:
	void readConfig(Bit::JsonTree* tree);
	void readParams(Bit::JsonTree* tree, Bit::ParamsRef params);

	std::string getConfigName(){ return "PixelEffect1"; }
	std::string getParamsName(){ return "PixelEffect1"; }
	void setup();
	void update();
	void draw();
	void reset();
	void stop();
	bool is2D(){ return true; };

	void drawSmallVideo();

private:
	void PixelEffect1::drawAt(ci::Vec2f midPoint, float Width, float Height, ci::ColorAf color, int &index, int rotationType);
	void setVideo(int index);

	std::vector<VideoInfoPixel>	videoInfos_;
	int videoIndex_;

	ci::Surface videoImage_;
	ci::Surface distortedImage_;

	ci::Vec2f processingSize_;

	cv::Mat prevImage_;
	float motionBlurFactor_;

	void playPauseVideo();
	void restartVideo();

	int blurBlockSize_;

	bool triangleEffect_;
	bool useShader_;

	int blockWidth_;
	int blockHeight_;
	int borderWidth_;
	int cutMeanThreshold_;
	int colorShade_;

	Vec2f smallVideoPosition_;
	Vec2f smallVideoSize_;

	gl::GlslProgRef	mShader_;

	std::vector< ci::Vec3f > vertices_;
	std::vector< uint32_t > indices_;
	std::vector< ci::ColorA > colors_;

	gl::GlslProgRef	gaussianShader_;
	gl::Fbo buffer_;
};