#include "PixelEffect1.h"
#include "cinder/ip/Resize.h"
#include "cinder/ip/EdgeDetect.h"
#include "cinder/ip/Grayscale.h"
#include "cinder/gl/Vbo.h"

using namespace ci;

void PixelEffect1::readConfig(Bit::JsonTree* tree)
{
	Bit::JsonTree *videoListTree = tree->getChildPtr("videos");
	for (Bit::JsonTree::Iter videoListNode = videoListTree->begin(); videoListNode != videoListTree->end(); videoListNode++)
	{
		VideoInfoPixel info;
		info.title = Bit::JsonTree::toBitJsonTreePtr(*videoListNode)->getChildPtr("title")->getValueT<std::string>();
		info.video.readConfig(Bit::JsonTree::toBitJsonTreePtr(*videoListNode));
		videoInfos_.push_back(info);
	}

	processingSize_.x = tree->getChildPtr("processingSize.width")->getValueT<float>();
	processingSize_.y = tree->getChildPtr("processingSize.height")->getValueT<float>();

	smallVideoPosition_ = tree->getChildPtr("smallVideoPosition")->getValueT<Vec2f>();

	float smallVideoWidth = tree->getChildPtr("smallVideoWidth")->getValueT<float>();
	smallVideoSize_ = Vec2f(smallVideoWidth, smallVideoWidth * 1080.f / 1920.f);
}

void PixelEffect1::readParams(Bit::JsonTree* tree, Bit::ParamsRef params)
{

	for (int i = 0; i < videoInfos_.size(); i++)
	{
		params->addButton("Play " + videoInfos_[i].title + " Video", std::bind(&PixelEffect1::setVideo, this, i), "group='" + tree->getKey() + "'");
	}
	params->addSeparator("", "group='" + tree->getKey() + "'");
	params->addButton("Play/Pause Video For Effect", std::bind(&PixelEffect1::playPauseVideo, this), "group='" + tree->getKey() + "'");
	params->addButton("Play Video From Start For Effect", std::bind(&PixelEffect1::restartVideo, this), "group='" + tree->getKey() + "'");

	params->addSeparator("", "group='" + tree->getKey() + "'");
	params->addParam<bool>(tree->getChildPtr("triangleEffect"), triangleEffect_, "Triangle Effect");
	params->addParam<bool>(tree->getChildPtr("useShader"), useShader_, "Use Shader");
	params->addParam<int>(tree->getChildPtr("blockWidth"), blockWidth_, "Tri Width");
	params->addParam<int>(tree->getChildPtr("blockHeight"), blockHeight_, "Tri Height");
	params->addParam<float>(tree->getChildPtr("motionBlurFactor"), motionBlurFactor_, "Motion Blur Factor");
	params->addParam<int>(tree->getChildPtr("border"), borderWidth_, "Border");
	params->addParam<int>(tree->getChildPtr("cutMeanThreshold"), cutMeanThreshold_, "Cut Threshold");
	params->addParam<int>(tree->getChildPtr("colorShade"), colorShade_, "Color Shade");

}

void PixelEffect1::setup()
{
	for (int i = 0; i < videoInfos_.size(); i++)
	{
		videoInfos_[i].video.setup();
	}

	videoIndex_ = 0;

	gl::Fbo::Format msaaFormat;
	msaaFormat.setSamples(5);
	buffer_ = gl::Fbo(app::getWindowIndex(0)->getWidth(), app::getWindowIndex(0)->getHeight(), msaaFormat);

	mShader_ = gl::GlslProg::create(ci::app::loadAsset(RES_PIXELEFFECT_VERT), ci::app::loadAsset(RES_PIXELEFFECT_FRAG));

	playPauseVideo();
}

void PixelEffect1::drawAt(ci::Vec2f midPoint, float Width, float Height, ci::ColorAf color, int &index, int rotationType)
{
	if (rotationType){
		Vec2f u = midPoint;
		u.y -= Height/2;

		Vec2f br = midPoint;
		br.y += Height/2;
		br.x -= Width;

		Vec2f bl = midPoint;
		bl.y += Height / 2;
		bl.x += Width;

		vertices_.push_back(Vec3f(u));
		vertices_.push_back(Vec3f(br));
		vertices_.push_back(Vec3f(bl));
	}
	else {

		Vec2f b = midPoint;
		b.y += Height/2;

		Vec2f ur = midPoint;
		ur.y -= Height / 2;
		ur.x -= Width;

		Vec2f ul = midPoint;
		ul.y -= Height / 2;
		ul.x += Width;

		vertices_.push_back(Vec3f(b));
		vertices_.push_back(Vec3f(ur));
		vertices_.push_back(Vec3f(ul));

	}

	for (int k = 0; k < 3; k++)
	{
		colors_.push_back(color);
		indices_.push_back(index++);
	}
}

void PixelEffect1::update()
{
	if (blockWidth_ < 8)
		blockWidth_ = 8;

	if (blockHeight_ < 8)
		blockHeight_ = 8;

	float lineWidthMin = 0;
	float lineWidthMax = blockWidth_;

	if (!triangleEffect_) return;

	videoImage_ = videoInfos_[videoIndex_].video.getSurface();

	vertices_.clear();
	indices_.clear();
	colors_.clear();

	int index = 0;

	if (!videoImage_ )
		return;

	Vec2i blockSize;

	blockSize = Vec2i(blockWidth_, blockHeight_);
	
	cv::Mat image = toOcv(videoImage_);
	if (prevImage_.size == image.size){
		image = ( image * motionBlurFactor_) + (prevImage_ *(1 - motionBlurFactor_));
	}
	prevImage_ = image;

	cv::Size windowSize = toOcv(app::getWindowIndex(0)->getBounds().getSize()) + 2 * toOcv(blockSize);

	cv::Mat downSampled;
	cv::resize(image, downSampled, cv::Size(windowSize.width / blockSize.x, windowSize.height / blockSize.y), CV_INTER_LANCZOS4);

	cv::Scalar bgColor = cv::mean(downSampled);

	for (int i = 0; i < windowSize.width / blockSize.x; i++)
	{
		for (int j = 0; j < windowSize.height / blockSize.y; j++)
		{
			cv::Scalar meanColor = downSampled.at<cv::Vec4b>(j, i);
			cv::Point2f midPoint = cv::Point2f((i - 1) * blockSize.x + blockSize.x / 2.f, (j - 1) * blockSize.y + blockSize.y / 2.f);

			//Compute max of values
			float maxVal = cv::max(meanColor.val[0],cv::max(meanColor.val[1], meanColor.val[2]));

			if (maxVal < 0)
				maxVal = 0;
			if (maxVal > 255)
				maxVal = 255;

			cv::Scalar darkColor = meanColor;
			if (maxVal < cutMeanThreshold_)
				darkColor = cv::Scalar(0, 0,0);

			ci::ColorAf finalColor( ((((int)darkColor.val[2] + 1) / colorShade_)*colorShade_) / 255.0,
									((((int)darkColor.val[1] + 1) / colorShade_)*colorShade_) / 255.0,
									((((int)darkColor.val[0] + 1) / colorShade_)*colorShade_) / 255.0 );

			drawAt(fromOcv(midPoint), blockSize.x - 2*borderWidth_, blockSize.y - borderWidth_,finalColor, index, (i + j) % 2);
		}
	}

}

void PixelEffect1::draw()
{
	if (!videoImage_)
		return;

	if (!triangleEffect_)
		gl::draw(videoImage_, ci::app::getWindowBounds());

	else {
		gl::pushMatrices();
		
		if (useShader_) {
			mShader_->bind();
			for (int i = 0; i < vertices_.size(); i += 3){
				//mShader_->uniform("inColor", Vec3f(1.0,1.0,1.0));
				mShader_->uniform("inColor", Vec3f(colors_[i].r, colors_[i].g, colors_[i].b));
				gl::drawSolidTriangle(Vec2f(vertices_[i].x, vertices_[i].y),
					Vec2f(vertices_[i + 1].x, vertices_[i + 1].y),
					Vec2f(vertices_[i + 2].x, vertices_[i + 2].y));
			}
			mShader_->unbind();
		}
		else {
			gl::VboMesh::Layout layout;
			layout.setStaticPositions();
			layout.setStaticIndices();
			layout.setStaticColorsRGBA();

			gl::VboMesh mesh(vertices_.size(), indices_.size(), layout, GL_TRIANGLES);
			mesh.bufferPositions(vertices_);
			mesh.bufferIndices(indices_);
			mesh.bufferColorsRGBA(colors_);

			gl::enableAlphaBlending();
			gl::draw(mesh);
			gl::disableAlphaBlending();
		}

		gl::popMatrices();
	}
}

void PixelEffect1::drawSmallVideo()
{
	ci::Rectf bound(smallVideoPosition_, smallVideoPosition_ + smallVideoSize_);

	gl::color(Colorf::white());
	gl::draw(videoImage_, bound);
}

void PixelEffect1::reset()
{
	if (!videoInfos_[videoIndex_].video.isPlaying())
		videoInfos_[videoIndex_].video.play();
}

void PixelEffect1::stop()
{
	if (videoInfos_[videoIndex_].video.isPlaying())
		videoInfos_[videoIndex_].video.pause();
}

void PixelEffect1::playPauseVideo()
{
	if (videoInfos_[videoIndex_].video.isPlaying())
		videoInfos_[videoIndex_].video.pause();
	else
		videoInfos_[videoIndex_].video.play();
}

void PixelEffect1::restartVideo()
{
	videoInfos_[videoIndex_].video.seekToStart();
	if (!videoInfos_[videoIndex_].video.isPlaying())
		videoInfos_[videoIndex_].video.play();
}

void PixelEffect1::setVideo(int index)
{
	videoInfos_[videoIndex_].video.stop();
	videoIndex_ = index;
	videoInfos_[videoIndex_].video.seekToStart();
	videoInfos_[videoIndex_].video.play();
}