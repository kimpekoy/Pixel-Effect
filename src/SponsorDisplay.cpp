#include "SponsorDisplay.h"
#include "cinder/app/AppBasic.h"
#include "cinder/ImageIo.h"

using namespace std;

void SponsorDisplay::readConfig(Bit::JsonTree* tree)
{
	fadeTime_ = tree->getChildPtr("fadeTime")->getValueT<float>();
	autoPlayWaitTime_ = tree->getChildPtr("autoPlayWaitTime")->getValueT<float>();

	Bit::JsonTree* imagePathTree = tree->getChildPtr("sponsorsImagesPath");
	for (Bit::JsonTree::Iter imagePathNode = imagePathTree->begin(); imagePathNode != imagePathTree->end(); imagePathNode++)
	{
		string path = Bit::JsonTree::toBitJsonTreePtr(*imagePathNode)->getChildPtr("path")->getValueT<string>();
		sponsorsImagesPath_.push_back(path);
	}
}

void SponsorDisplay::readParams(Bit::JsonTree* tree, Bit::ParamsRef params)
{
	((params::InterfaceGl)(*params)).addParam("Current Page", &currentPageStr_, true).group("Sponsors");

	params->addParam<bool>(tree->getChildPtr("autoPlay"), autoPlay_, "Autoplay Sponsors").group("Sponsors");

	for (int i = 0; i < sponsorsImagesPath_.size(); i++)
	{
		params->addButton("Goto Page " + to_string(i+1), std::bind(&SponsorDisplay::setPage, this, i), "group='Sponsors'");
	}
}

void SponsorDisplay::setup()
{
	for (int i = 0; i < sponsorsImagesPath_.size(); i++)
	{
		ci::DataSourceRef dataSourceRef = ci::app::loadAsset(sponsorsImagesPath_[i]);
		sponsorsImages_.push_back(ci::loadImage(dataSourceRef));
	}
	currentPageStr_ = "Page 1";
	stateStartTime_ = app::getElapsedSeconds();
	previousIndex_ = currentIndex_ = 0;
}

void SponsorDisplay::setPage(int pageIndex)
{
	previousIndex_ = currentIndex_;
	currentIndex_ = pageIndex;
	currentPageStr_ = "Page " + to_string(pageIndex+1);
	stateStartTime_ = ci::app::getElapsedSeconds();
}

void SponsorDisplay::update()
{
	float currentTime = app::getElapsedSeconds();
	if ((autoPlay_) && (currentTime - stateStartTime_ > autoPlayWaitTime_))
	{
		setPage((currentIndex_ + 1) % sponsorsImages_.size());
	}
}

void SponsorDisplay::draw()
{
	float currentTime = app::getElapsedSeconds();
	if (currentTime - stateStartTime_ < fadeTime_)
	{
		float ratio = 1 - (currentTime - stateStartTime_) / fadeTime_;

		gl::enableAlphaBlending();

		gl::color(Colorf::white());
		ci::gl::draw(sponsorsImages_[currentIndex_], ci::app::getWindowBounds());
		gl::color(ColorAf(1, 1, 1, ratio));
		ci::gl::draw(sponsorsImages_[previousIndex_], ci::app::getWindowBounds());
	}
	else
	{
		gl::color(Colorf::white());
		ci::gl::draw(sponsorsImages_[currentIndex_], ci::app::getWindowBounds());
	}

	gl::color(Colorf::white());	
}

void SponsorDisplay::reset()
{
	stateStartTime_ = ci::app::getElapsedSeconds();
}