#include "DistortionAnimation.h"
#include "cinder/ip/Resize.h"
#include "cinder/gl/Vbo.h"

//block size in px
#define LINE_WIDTH_MAX 10
#define LINE_WIDTH_MIN 2

#define BG_COLOR cv::Scalar(255, 255, 255)
#define LINE_COLOR cv::Scalar(225, 170, 36)

#define BRIGHT_IMAGE_OFFSET cv::Scalar(127, 127, 127)

#define RES_PASSTHRU_VERT	"shaders/gaussianblur.vert"
#define RES_BLUR_FRAG		"shaders/gaussianblur.frag"

using namespace ci;

void DistortionAnimation::readConfig(Bit::JsonTree* tree)
{
	Bit::JsonTree *videoListTree = tree->getChildPtr("videos");
	for (Bit::JsonTree::Iter videoListNode = videoListTree->begin(); videoListNode != videoListTree->end(); videoListNode++)
	{
		VideoInfo info;
		info.title = Bit::JsonTree::toBitJsonTreePtr(*videoListNode)->getChildPtr("title")->getValueT<std::string>();
		info.video.readConfig(Bit::JsonTree::toBitJsonTreePtr(*videoListNode));
		videoInfos_.push_back(info);
	}

	processingSize_.x = tree->getChildPtr("processingSize.width")->getValueT<float>();
	processingSize_.y = tree->getChildPtr("processingSize.height")->getValueT<float>();

	smallVideoPosition_ = tree->getChildPtr("smallVideoPosition")->getValueT<Vec2f>();

	blurBlockSize_ = tree->getChildPtr("blurBlockSize")->getValueT<float>();

	float smallVideoWidth = tree->getChildPtr("smallVideoWidth")->getValueT<float>();
	smallVideoSize_ = Vec2f(smallVideoWidth, smallVideoWidth * 1080.f / 1920.f);
}

void DistortionAnimation::readParams(Bit::JsonTree* tree, Bit::ParamsRef params)
{
	params->addButton("Line Distortion Filter", std::bind(&DistortionAnimation::setCurrentFilter, this, DISTORTION_FILTER), "group='" + tree->getKey() + "'");
	params->addButton("Blur Enhancement Filter", std::bind(&DistortionAnimation::setCurrentFilter, this, BLUR_FILTER), "group='" + tree->getKey() + "'");
	params->addSeparator("", "group='" + tree->getKey() + "'");
	
	for (int i = 0; i < videoInfos_.size(); i++)
	{
		params->addButton("Play " + videoInfos_[i].title, std::bind(&DistortionAnimation::setVideo, this, i), "group='" + tree->getKey() + "'");
	}
	params->addSeparator("", "group='" + tree->getKey() + "'");
	params->addButton("Play/Pause Video", std::bind(&DistortionAnimation::playPauseVideo, this), "group='" + tree->getKey() + "'");
	params->addButton("Play Video From Start", std::bind(&DistortionAnimation::restartVideo, this), "group='" + tree->getKey() + "'");

	params->addSeparator("", "group='" + tree->getKey() + "'");
	params->addParam<bool>(tree->getChildPtr("lineEffect"), lineEffect_, "Enable Line Effect");
	params->addParam<int>(tree->getChildPtr("blockWidth"), blockWidth_, "Line Width");
	params->addParam<int>(tree->getChildPtr("blockWidth"), blockHeight_, "Line Height");
	params->addParam<float>(tree->getChildPtr("linesAngle"), linesAngle_, "Line Angle");
	params->addSeparator("", "group='" + tree->getKey() + "'");
	params->addParam<float>(tree->getChildPtr("blurAmount"), blurAmount_, "Blur Distance");
}

void DistortionAnimation::setup()
{
	currentFilter_ = DISTORTION_FILTER;

	for (int i = 0; i < videoInfos_.size(); i++)
	{
		videoInfos_[i].video.setup();
	}

	videoIndex_ = 0;

	gaussianShader_ = gl::GlslProg::create(ci::app::loadAsset(RES_PASSTHRU_VERT), ci::app::loadAsset(RES_BLUR_FRAG));

	gl::Fbo::Format msaaFormat;
	msaaFormat.setSamples(5);
	buffer_ = gl::Fbo(app::getWindowIndex(0)->getWidth(), app::getWindowIndex(0)->getHeight(), msaaFormat);
}

void DistortionAnimation::drawRectangleAt(ci::Vec2f midPoint, float lineWidth, float lineHeight, float angle, ci::ColorAf color, int &index)
{
	float angleR = angle * M_PI / 180;

	if (abs(angle) < 45)
	{
		lineHeight = lineHeight / cos(angleR);
		lineWidth = lineWidth * cos(angleR);
	}
	else
	{
		lineHeight = lineHeight / cos(M_PI / 2 - angleR);
		lineWidth = lineWidth * cos(M_PI / 2 - angleR);
	}

	Rectf rect = Rectf(- lineWidth / 2, - lineHeight / 2, lineWidth / 2, lineHeight / 2);

	Vec2f ul = rect.getUpperLeft();
	ul.rotate(angleR);
	ul += midPoint;

	Vec2f ur = rect.getUpperRight();
	ur.rotate(angleR);
	ur += midPoint;

	Vec2f br = rect.getLowerRight();
	br.rotate(angleR);
	br += midPoint;

	Vec2f bl = rect.getLowerLeft();
	bl.rotate(angleR);
	bl += midPoint;

	vertices_.push_back(Vec3f(ul));
	vertices_.push_back(Vec3f(ur));
	vertices_.push_back(Vec3f(br));
	vertices_.push_back(Vec3f(bl));

	for (int k = 0; k < 4; k++)
	{
		colors_.push_back(color);
		indices_.push_back(index++);
	}
}

void DistortionAnimation::drawMotionBlurAt(ci::Vec2f midPoint, float lineWidth, float lineHeight, float blurAmount, ci::ColorAf color, int &index)
{
	float blurx = roundf(blurAmount / blurBlockSize_);
	float blury = roundf(10 * blurAmount / blurBlockSize_);

	Rectf rect = Rectf(-lineWidth / 2 * blurx, -lineHeight / 2 * blury, lineWidth / 2 * blurx, lineHeight / 2 * blury);

	Vec2f ul = rect.getUpperLeft();
	ul += midPoint;

	Vec2f ur = rect.getUpperRight();
	ur += midPoint;

	Vec2f br = rect.getLowerRight();
	br += midPoint;

	Vec2f bl = rect.getLowerLeft();
	bl += midPoint;

	vertices_.push_back(Vec3f(ul));
	vertices_.push_back(Vec3f(ur));
	vertices_.push_back(Vec3f(br));
	vertices_.push_back(Vec3f(bl));

	for (int k = 0; k < 4; k++)
	{
		colors_.push_back(ColorAf(color, 1.0 / (blurx * blury)));
		indices_.push_back(index++);
	}
}

void DistortionAnimation::update()
{
	if (blockWidth_ < 8)
		blockWidth_ = 8;

	if (blockHeight_ < 8)
		blockHeight_ = 8;

	float lineWidthMin = 0;
	float lineWidthMax = blockWidth_;

	videoImage_ = videoInfos_[videoIndex_].video.getSurface();

	vertices_.clear();
	indices_.clear();
	colors_.clear();

	int index = 0;

	if (!videoImage_)
		return;
	
	Vec2i blockSize;

	if (currentFilter_ == DISTORTION_FILTER)
		blockSize = Vec2i(blockWidth_, blockHeight_);
	else //BLUR_FILTER
		blockSize = Vec2i(blurBlockSize_, blurBlockSize_);

	cv::Mat image = toOcv(videoImage_);

	cv::Size windowSize = toOcv(app::getWindowIndex(0)->getBounds().getSize()) + 2 * toOcv(blockSize);

	cv::Mat downSampled;
	cv::resize(image, downSampled, cv::Size(windowSize.width / blockSize.x, windowSize.height / blockSize.y), CV_INTER_LANCZOS4);

	//fill the info from image
	if (imageBuffer_.empty())
		image.copyTo(imageBuffer_);

	cv::Scalar bgColor = cv::mean(downSampled);

	for (int i = 0; i < windowSize.width / blockSize.x; i++)
	{
		for (int j = 0; j < windowSize.height / blockSize.y; j++)
		{
			cv::Scalar meanColor = downSampled.at<cv::Vec4b>(j, i);
			cv::Point2f midPoint = cv::Point2f((i - 1) * blockSize.x + blockSize.x / 2.f, (j - 1) * blockSize.y + blockSize.y / 2.f);

			if (currentFilter_ == DISTORTION_FILTER)
			{
				//Compute sum of values
				float meanVal = 0.333 * (meanColor.val[0] + meanColor.val[1] + meanColor.val[2]);

				if (meanVal < 0)
					meanVal = 0;
				if (meanVal > 255)
					meanVal = 255;

				float line_width = lineWidthMin + (meanVal / 255.0) * (lineWidthMax - lineWidthMin);
			
				if (!lineEffect_)
					line_width = 2;

				//cv::Scalar brightColor = meanColor + cv::Scalar(BRIGHT_IMAGE_OFFSET);
				cv::Scalar brightColor = bgColor + BRIGHT_IMAGE_OFFSET;
				cv::Scalar darkColor = meanColor;
				
				drawRectangleAt(fromOcv(midPoint), blockSize.x, blockSize.y, linesAngle_, ci::ColorAf(brightColor.val[2] / 255.0, brightColor.val[1] / 255.0, brightColor.val[0] / 255.0, 1), index);
				drawRectangleAt(fromOcv(midPoint), blockSize.x - line_width, blockSize.y, linesAngle_, ci::ColorAf(darkColor.val[2] / 255.0, darkColor.val[1] / 255.0, darkColor.val[0] / 255.0, 1), index);
			}
			else //BLUR_FILTER
			{
				drawMotionBlurAt(fromOcv(midPoint), blockSize.x, blockSize.y, blurAmount_, ci::ColorAf(meanColor.val[2] / 255.0, meanColor.val[1] / 255.0, meanColor.val[0] / 255.0, 1), index);
			}
		}
	}

	//for (int i = 0; i < image.cols; i++)
	//{
	//	for (int j = 0; j < image.rows; j++)
	//	{
	//		cv::Scalar meanColor = image.at<cv::Vec4b>(j, i);
	//		
	//		cv::Point midPoint = cv::Point(i, j);

	//		drawMotionBlurAt(fromOcv(midPoint), blockWidth_, blockHeight_, ci::ColorAf(meanColor.val[2] / 255.0, meanColor.val[1] / 255.0, meanColor.val[0] / 255.0, 1), index);
	//	}
	//}

	//distortedImage_ = fromOcv(imageBuffer_);
}

void DistortionAnimation::draw()
{
	if (!videoImage_)
		return;

	gl::pushMatrices();

	
	if (currentFilter_ == DISTORTION_FILTER)
	{
		gl::VboMesh::Layout layout;
		layout.setStaticPositions();
		layout.setStaticIndices();
		layout.setStaticColorsRGBA();

		gl::VboMesh mesh(vertices_.size(), indices_.size(), layout, GL_QUADS);
		mesh.bufferPositions(vertices_);
		mesh.bufferIndices(indices_);
		mesh.bufferColorsRGBA(colors_);

		gl::enableAlphaBlending();
		gl::draw(mesh);
		gl::disableAlphaBlending();
	}
	else //BLUR_FILTER
	{
		{
			gl::SaveFramebufferBinding bindingSaver;
			gl::enableAlphaBlending();

			gl::VboMesh::Layout layout;
			layout.setStaticPositions();
			layout.setStaticIndices();
			layout.setStaticColorsRGBA();

			gl::VboMesh mesh(vertices_.size(), indices_.size(), layout, GL_QUADS);
			mesh.bufferPositions(vertices_);
			mesh.bufferIndices(indices_);
			mesh.bufferColorsRGBA(colors_);

			buffer_.bindFramebuffer();
			gl::clear(Colorf::black());
			gl::color(Colorf::white());
			gl::draw(mesh);
			buffer_.unbindFramebuffer();

			gl::disableAlphaBlending();
		}

		gl::pushMatrices();
		gl::enableAlphaBlending(true);
		gl::translate(0, app::getWindowHeight());
		gl::scale(1, -1);
		
		buffer_.getTexture().enableAndBind();
		gaussianShader_->bind();
		gaussianShader_->uniform("tex0", 0);
		gaussianShader_->uniform("sampleOffset", Vec2f(2.f * blurAmount_ / app::getWindowWidth(), 5.f * blurAmount_ / app::getWindowHeight()));
		gl::drawSolidRect(ci::app::getWindowBounds());

		gaussianShader_->unbind();
		buffer_.unbindTexture();

		gl::disableAlphaBlending();
		gl::popMatrices();
	}

	gl::popMatrices();
}

void DistortionAnimation::drawSmallVideo()
{
	ci::Rectf bound(smallVideoPosition_, smallVideoPosition_ + smallVideoSize_);

	gl::color(Colorf::white());
	gl::draw(videoImage_, bound);
}

void DistortionAnimation::reset()
{
	if (!videoInfos_[videoIndex_].video.isPlaying())
		videoInfos_[videoIndex_].video.play();
}

void DistortionAnimation::stop()
{
	if (videoInfos_[videoIndex_].video.isPlaying())
		videoInfos_[videoIndex_].video.pause();
}

void DistortionAnimation::setCurrentFilter(Filter filter)
{
	currentFilter_ = filter;
}

void DistortionAnimation::playPauseVideo()
{
	if (videoInfos_[videoIndex_].video.isPlaying())
		videoInfos_[videoIndex_].video.pause();
	else
		videoInfos_[videoIndex_].video.play();
}

void DistortionAnimation::restartVideo()
{
	videoInfos_[videoIndex_].video.seekToStart();
	if (!videoInfos_[videoIndex_].video.isPlaying())
		videoInfos_[videoIndex_].video.play();
}

void DistortionAnimation::setVideo(int index)
{
	videoInfos_[videoIndex_].video.stop();
	videoIndex_ = index;
	videoInfos_[videoIndex_].video.seekToStart();
	videoInfos_[videoIndex_].video.play();
}