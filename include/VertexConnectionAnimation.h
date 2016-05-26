#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "CinderOpenCV.h"
#include "Animation.h"

#define RAND_SIZE 10000

struct FileInfo
{
	std::string title;
	std::string path;
};

struct ColorPreset
{
	std::string title;
	bool brighterLines;
	float gradientAngle;
	bool autoColor;

	ci::Color pointsColor;
	ci::Color bg1Color;
	ci::Color bg2Color;

};

class VertexConnectionAnimation : public Animation
{
public:
	void readConfig(Bit::JsonTree* tree);
	void readParams(Bit::JsonTree* tree, Bit::ParamsRef params);

	void setup();
	void update();
	void draw();
	void reset();
	std::string getConfigName(){ return "Dots & Lines"; }
	std::string getParamsName(){ return "Dots & Lines"; }

protected:
	ci::Vec3f getLocation(int vertexIndex);
	void resetVertices();
	void rePopulatePoints();
	void calculateVertexMovementToDestination();
	void updateVertexLocation();
	void updateVertexConnection();
	void initVariables();
	void setImageIndex(int index);
	void randomBurst(float speed);
	void applyPreset(int index);
	void applyColors();

	float stateStartTime_;
	int previousIndex_;
	int imageIndex_;

	ci::Colorf getVertexColor(int index);

	void loadFile(std::string filename);
	std::vector<cv::Point2d> findContour(cv::Mat mat);

	float rands_[RAND_SIZE];

	std::vector<float> depths_;
	std::vector<float> randDestination_;

	std::vector< ci::Vec3f > vertices_;
	std::vector< uint32_t > indices_;
	std::vector< ci::ColorA > colors_;

	int pointNum_;
	
	std::vector<std::vector<cv::Point2d>> imagePoints_;
	std::vector<ci::Vec2f> verticesLocation_;
	std::vector<ci::Vec2f> verticesSpeed_;

	bool autoPlay_;
	bool colorSolid_;
	bool brighterLines_;
	float colorSwapStartTime_;
	float colorSwapTime_;

	float depthRotationTime_;
	float animationWaitTime_;
	float autoplayBurstVelocityFactor_;
	float idleBurstVelocityFactor_;
	float accelerationFactor_;
	float accelerationTimeFactor_;
	float maxLength_;
	float floatingMaxLength_;
	float maxAlpha_;
	float depthMagnitude_;

	float velocityDampingFactor_;
	float contourDistance_;

	float windowWidth_;
	float windowHeight_;

	float gradientAngle_;

	ci::Color pointsColor_;
	ci::Color bgColor1_;
	ci::Color bgColor2_;

	ci::Color pointsColorParam_;
	ci::Color bgColor1Param_;
	ci::Color bgColor2Param_;

	ci::Color previousPointsColor_;
	ci::Color previousBgColor1_;
	ci::Color previousBgColor2_;

	std::vector<FileInfo> fileList_;
	std::vector<ColorPreset> colorPresets_;
	
	Vec2f displayOffset_;
};