#pragma once

#include "VertexConnectionAnimation.h"

class BrownianPointAnimation : public VertexConnectionAnimation
{
public:
	void readConfig(Bit::JsonTree* tree);
	void readParams(Bit::JsonTree* tree, Bit::ParamsRef params);

	void setup();
	void update();
	void draw();
	void reset();

	std::string getConfigName(){ return "Floating_Animation"; }
	std::string getParamsName(){ return "Floating_Animation"; }

private:
	void toggleMultiColor() { useMultiColor_ = !useMultiColor_; };

	float windowWidth_;
	float windowHeight_;
	float maxVelocity_;
	float minVelocity_;
	int numPoints_;
	float pointRadiusMin_;
	float pointRadiusMax_;

	bool useMultiColor_;
	ci::Color pointsColor_;
};