#pragma once

#include "Animation.h"
#include "Configurable.h"
#include "BitMultimedia.h"
#include "CinderOpenCV.h"
#include "cinder/gl/Fbo.h"

enum Filter
{
	DISTORTION_FILTER = 0,
	BLUR_FILTER = 1
};

struct VideoInfo
{
	std::string title;
	Bit::Video video;
};

class DistortionAnimation : public Animation
{
public:
	void readConfig(Bit::JsonTree* tree);
	void readParams(Bit::JsonTree* tree, Bit::ParamsRef params);

	std::string getConfigName(){ return "Intermission"; }
	std::string getParamsName(){ return "Intermission"; }
	void setup();
	void update();
	void draw();
	void reset();
	void stop();
	bool is2D(){ return true; };

	void drawSmallVideo();

private:
	void setCurrentFilter(Filter filter);
	void drawRectangleAt(ci::Vec2f midPoint, float lineWidth, float lineHeight, float angle, ci::ColorAf color, int &index);
	void drawMotionBlurAt(ci::Vec2f midPoint, float lineWidth, float lineHeight, float blurAmount, ci::ColorAf color, int &index);
	void setVideo(int index); 

	std::vector<VideoInfo>	videoInfos_;
	int videoIndex_;

	ci::Surface videoImage_;
	ci::Surface distortedImage_;
	
	ci::Vec2f processingSize_;

	cv::Mat imageBuffer_;

	void playPauseVideo();
	void restartVideo();

	int blurBlockSize_;

	int blockWidth_;
	int blockHeight_;

	float linesAngle_;
	bool lineEffect_;

	float blurAmount_;

	Vec2f smallVideoPosition_;
	Vec2f smallVideoSize_;

	Filter currentFilter_;

	std::vector< ci::Vec3f > vertices_;
	std::vector< uint32_t > indices_;
	std::vector< ci::ColorA > colors_;

	gl::GlslProgRef	gaussianShader_;
	gl::Fbo buffer_;
};