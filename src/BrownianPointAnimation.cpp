#include "BrownianPointAnimation.h"
#include "cinder/Rand.h"

using namespace ci;

void BrownianPointAnimation::readConfig(Bit::JsonTree* tree)
{
	VertexConnectionAnimation::readConfig(tree);

	windowWidth_ = tree->getChildPtr("windowWidth")->getValueT<float>();
	windowHeight_ = tree->getChildPtr("windowHeight")->getValueT<float>();
	minVelocity_ = tree->getChildPtr("minVelocity")->getValueT<float>();
	maxVelocity_ = tree->getChildPtr("maxVelocity")->getValueT<float>();
	
	pointRadiusMin_ = tree->getChildPtr("pointRadiusMin")->getValueT<float>();
	pointRadiusMax_ = tree->getChildPtr("pointRadiusMax")->getValueT<float>();
}

void BrownianPointAnimation::readParams(Bit::JsonTree* tree, Bit::ParamsRef params)
{
	params->addParam<int>(tree->getChildPtr("numPoints"), numPoints_, "Number of points");
	((ci::params::InterfaceGl)(*params)).addParam("Point Color", &pointsColor_).group(tree->getKey());
	params->addButton("Toggle color display mode", std::bind(&BrownianPointAnimation::toggleMultiColor, this), "group='" + tree->getKey() + "'");
	
}

void BrownianPointAnimation::setup()
{
	imagePoints_.clear();
	animationWaitTime_ = -1;
	initVariables();
	useMultiColor_ = true;

	verticesLocation_.resize(numPoints_);
	verticesSpeed_.resize(numPoints_);

	depths_.resize(numPoints_);

	//random
	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		verticesLocation_[i] = Vec2f(ci::Rand().randFloat(), ci::Rand().randFloat()) * 2 - Vec2f(1.f, 1.f);
		verticesSpeed_[i] = ci::Rand().randVec2f() * ci::Rand().randFloat(minVelocity_, maxVelocity_);
	}
}

void BrownianPointAnimation::update()
{
	//In case numPoints_ changed

	if (verticesLocation_.size() != numPoints_)
	{
		if (numPoints_ < 20)
			numPoints_ = 20;
		setup();
	}

	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		if (verticesLocation_[i].x < -windowWidth_ / 2.f)
			verticesLocation_[i].x += windowWidth_;
		if (verticesLocation_[i].x > windowWidth_ / 2.f)
			verticesLocation_[i].x -= windowWidth_;

		if (verticesLocation_[i].y < -windowHeight_ / 2.f)
			verticesLocation_[i].y += windowHeight_;
		if (verticesLocation_[i].y > windowHeight_ / 2.f)
			verticesLocation_[i].y -= windowHeight_;
	}

	VertexConnectionAnimation::update();
}

void BrownianPointAnimation::draw()
{
	VertexConnectionAnimation::draw();

	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		float pointRadius = ci::Rand(i).nextFloat(pointRadiusMin_, pointRadiusMax_);

		gl::color(getVertexColor(i));
		gl::drawSphere(getLocation(i), pointRadius);
	}

	gl::color(Colorf::white());
}

void BrownianPointAnimation::reset()
{
	//Re-random
	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		verticesLocation_[i] = Vec2f(ci::Rand().randFloat(), ci::Rand().randFloat()) * 2 - Vec2f(1.f, 1.f);
		verticesSpeed_[i] = ci::Rand().randVec2f() * ci::Rand().randFloat(minVelocity_, maxVelocity_);
	}
}