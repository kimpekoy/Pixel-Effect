#include "SpeakerOverlayAnimation.h"
#include "cinder/ImageIo.h"

void SpeakerOverlayAnimation::readConfig(Bit::JsonTree* tree)
{
	Bit::JsonTree* speakerTree =  tree->getChildPtr("speakerNames");
	for (Bit::JsonTree::Iter speakerNode = speakerTree->begin(); speakerNode != speakerTree->end(); speakerNode++)
	{
		SpeakerInfo speakerInfo;

		std::string speakerName = Bit::JsonTree::toBitJsonTreePtr(*speakerNode)->getChildPtr("name")->getValueT<std::string>();		
		std::string imagePath = "";
		//if not separator
		if (speakerName.compare("Separator"))
		{
			imagePath = Bit::JsonTree::toBitJsonTreePtr(*speakerNode)->getChildPtr("image")->getValueT<std::string>();
		}

		if (Bit::JsonTree::toBitJsonTreePtr(*speakerNode)->hasChild("group") && Bit::JsonTree::toBitJsonTreePtr(*speakerNode)->getChildPtr("group")->getValueT<bool>() == true)
		{
			speakerInfo.isGroup = true;
		}
		
		speakerInfo.speakerName = speakerName;
		speakerInfo.renderImagePath = imagePath;
		speakerInfos_.push_back(speakerInfo);
	}

	fadeDuration_ = tree->getChildPtr("fadeDuration")->getValueT<float>();
	defaultFontScale_ = tree->getChildPtr("defaultFontScale")->getValueT<float>();
}

void SpeakerOverlayAnimation::readParams(Bit::JsonTree* tree, Bit::ParamsRef params)
{
	params->addParam(tree->getChildPtr("textPosition"), namePosition_, "Text Position").group("Speaker Name");
	//params->addParam(tree->getChildPtr("transparentText"), transparentText_, "Transparent Text").group("Speaker Name");
	transparentText_ = true;

	params->addParam(tree->getChildPtr("fontSize"), nameSize_, "Text Size").group("Speaker Name");

	((ci::params::InterfaceGl)(*params)).addParam("Current Speaker ", &currentSpeakerName_, "group='Speaker Name'", true);

	for (int i = 0; i < speakerInfos_.size(); i++)
	{
		if (speakerInfos_[i].speakerName.compare("Separator"))
		{
			params->addButton(speakerInfos_[i].speakerName, std::bind(&SpeakerOverlayAnimation::setSpeakerIndex, this, i), "group='Speaker Name'");
		}
		else
		{
			params->addSeparator("", "group='Speaker Name'");
		}		
	}
}

void SpeakerOverlayAnimation::setup()
{
	speakerIndex_ = 0;
	currentSpeakerName_ = speakerInfos_[speakerIndex_].speakerName;
	currentAlpha_ = 0;

	for (int i = 0; i < speakerInfos_.size(); i++)
	{
		if (speakerInfos_[i].renderImagePath.length() > 0)
		{
			ci::DataSourceRef dataSourceRef = ci::app::loadAsset(speakerInfos_[i].renderImagePath);			
			speakerInfos_[i].renderImage = ci::loadImage(dataSourceRef);
		}
	}
} 

void SpeakerOverlayAnimation::update()
{

	float currentTime = ci::app::getElapsedSeconds();
	float increment = (currentTime - previousFrameTime_) / fadeDuration_;

	currentSpeakerName_ = speakerInfos_[speakerIndex_].speakerName;
	if (isVisible_ && currentAlpha_ < 1.f)
	{
		currentAlpha_ = std::min(1.f, currentAlpha_ + increment);
	}
	else if (!isVisible_ && currentAlpha_ > 0.f)
	{
		currentAlpha_ = std::max(0.f, currentAlpha_ - increment);
	}

	previousFrameTime_ = currentTime;
}

void SpeakerOverlayAnimation::draw()
{	
	gl::color(ColorAf(Colorf::white(), currentAlpha_));
	gl::pushMatrices();

	if (transparentText_)
	{
		gl::enable(GL_BLEND);
		glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
		gl::color(Colorf(currentAlpha_, currentAlpha_, currentAlpha_));
		gl::translate(namePosition_);
		gl::scale(nameSize_ * 0.01 * defaultFontScale_, nameSize_ * 0.01 * defaultFontScale_);
		if (speakerInfos_[speakerIndex_].isGroup)
		{
			//additional scale
			gl::scale(1.2, 1.2);
		}

		gl::draw(speakerInfos_[speakerIndex_].renderImage);
		gl::disable(GL_BLEND);
	}
	else
	{
		gl::enableAlphaBlending();
		gl::translate(namePosition_);
		gl::scale(nameSize_ * 0.01 * defaultFontScale_, nameSize_ * 0.01 * defaultFontScale_);
		if (speakerInfos_[speakerIndex_].isGroup)
		{
			//additional scale
			gl::scale(1.2, 1.2);
		}

		gl::draw(speakerInfos_[speakerIndex_].renderImage);
		gl::disableAlphaBlending();
	}

	gl::popMatrices();
	gl::color(Colorf::white());	
}

void SpeakerOverlayAnimation::setSpeakerIndex(int index)
{
	speakerIndex_ = index;
}

void SpeakerOverlayAnimation::setVisible(bool isVisible)
{
	isVisible_ = isVisible;
}