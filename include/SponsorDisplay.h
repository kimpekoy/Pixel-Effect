#pragma once

#include "Animation.h"
#include "Configurable.h"
#include "BitMultimedia.h"

class SponsorDisplay : public Animation
{
public:
	void readConfig(Bit::JsonTree* tree);
	void readParams(Bit::JsonTree* tree, Bit::ParamsRef params);
	void setup();
	void update();
	void draw();
	bool is2D(){ return true; };
	void reset();

	std::string getConfigName(){ return "Sponsors"; }
	std::string getParamsName(){ return "Sponsors"; }

private:
	void setPage(int pageIndex);

	int previousIndex_;
	int currentIndex_;

	bool autoPlay_;
	float stateStartTime_;
	float fadeTime_;

	float autoPlayWaitTime_;

	std::vector<std::string> sponsorsImagesPath_;
	std::vector<ci::gl::Texture> sponsorsImages_;
	std::string	currentPageStr_;

};