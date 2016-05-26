#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "CinderOpenCV.h"
#include "cinder/audio/Source.h"
#include "cinder/audio/Voice.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/MonitorNode.h"

#include "Animation.h"
#include "VertexConnectionAnimation.h"

class SoundAnimation : public VertexConnectionAnimation
{
public:
	void readConfig(Bit::JsonTree* tree);
	void readParams(Bit::JsonTree* tree, Bit::ParamsRef params);

	void setup();
	void update();
	void draw();
	void reset();

	std::string getConfigName(){ return "Active_Voice_Animation"; }
	std::string getParamsName(){ return "Active_Voice_Animation"; }

private:
	ci::audio::Context* audioContext_;
	std::shared_ptr<ci::audio::MonitorNode> monitorNode_;
	ci::audio::NodeRef input_;

	std::string inputFile_;

	float loudness_;

	float maxLoudness_;
	float speedFactor_;
	float randomness_;
};