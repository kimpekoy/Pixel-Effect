#pragma once

#include "Animation.h"
#include "Configurable.h"
#include "BitMultimedia.h"
#include "cinder/gl/Texture.h"

struct SpeakerInfo
{
	std::string speakerName;
	std::string renderImagePath;
	ci::gl::Texture renderImage;
	bool isGroup;
};

class SpeakerOverlayAnimation : public Animation
{
public:
	void readConfig(Bit::JsonTree* tree);
	void readParams(Bit::JsonTree* tree, Bit::ParamsRef params);
	void setup();
	void update();
	void draw();
	bool is2D(){ return true; };
	void setVisible(bool isVisible);

	std::string getConfigName(){ return "Speaker_Overlay"; }
	std::string getParamsName(){ return "Speaker_Overlay"; }

private:
	void setSpeakerIndex(int index);

	int speakerIndex_;
	std::vector<SpeakerInfo> speakerInfos_;
	std::string currentSpeakerName_;

	float previousFrameTime_;

	float fadeDuration_;
	float currentAlpha_;
	//gl::TextureFontRef	textureFont_;
	
	ci::Vec2f	namePosition_;

	float nameSize_;
	float defaultFontScale_;

	bool isVisible_;
	bool transparentText_;
	float textVerticalOffset_;
};