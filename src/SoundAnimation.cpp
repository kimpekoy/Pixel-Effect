#include "SoundAnimation.h"
#include "cinder/Rand.h"
#include "cinder/app/AppNative.h"

//#define MAX_LOUDNESS 10.0
//#define SPEED_FACTOR 0.002f
//#define RANDOMNESS 0.2f

void SoundAnimation::readConfig(Bit::JsonTree* tree)
{
	VertexConnectionAnimation::readConfig(tree);
	inputFile_ = tree->getChildPtr("inputFile")->getValueT<std::string>();
	randomness_ = tree->getChildPtr("randomness")->getValueT<float>();
}

void SoundAnimation::readParams(Bit::JsonTree* tree, Bit::ParamsRef params)
{
	params->addParam<float>(tree->getChildPtr("maxLoudness"), maxLoudness_, "Maximum loudness");
	params->addParam<float>(tree->getChildPtr("speedFactor"), speedFactor_, "Scatter velocity");
}

void SoundAnimation::setup()
{
	loadFile("line_animation/logo1.png");
	animationWaitTime_ = -1;
	initVariables();

	audioContext_ = audio::Context::master();
	monitorNode_ = audioContext_->makeNode(new audio::MonitorNode);

	input_ = audioContext_->createInputDeviceNode();

	auto sourceFile = ci::audio::load(app::loadAsset(inputFile_), audioContext_->getSampleRate());
	auto bufferPlayer = audioContext_->makeNode(new audio::BufferPlayerNode());
	bufferPlayer->loadBuffer(sourceFile);
	bufferPlayer->setLoopEnabled();
	bufferPlayer->start();
	input_ = bufferPlayer;

	//input_->enable();
	input_ >> monitorNode_;	
	input_ >> audioContext_->getOutput();

	audioContext_->enable();
}

void SoundAnimation::update()
{
	VertexConnectionAnimation::update();
	loudness_ = sqrt(monitorNode_->getVolume());

	if (loudness_ > maxLoudness_)
		loudness_ = maxLoudness_;
	
	//apply force	
	for (int i = 0; i < verticesSpeed_.size(); i++)
	{
		if (ci::Rand().randFloat() < randomness_)
		{
			//add force vector from 0,0,0
			verticesSpeed_[i] += ci::Rand().randVec2f() * (speedFactor_ * loudness_ / (1.f + pow(verticesLocation_[i].length(), 2)));
			//verticesSpeed_[i] += verticesLocation_[i].normalized() * (0.001f * loudness / (1.f + pow(verticesLocation_[i].length(), 2)));
		}
	}
}

void SoundAnimation::draw()
{
	VertexConnectionAnimation::draw();
	gl::drawSphere(Vec3f(0, 0, -1), loudness_ / maxLoudness_, 30);
}

void SoundAnimation::reset()
{
	VertexConnectionAnimation::reset();
}