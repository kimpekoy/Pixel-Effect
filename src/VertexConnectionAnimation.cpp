#include "VertexConnectionAnimation.h"
#include "cinder/app/AppNative.h"
#include "cinder/app/AppBasic.h"
#include "cinder/Rand.h"
#include "cinder/gl/Vbo.h"

using namespace std;
using namespace ci;

template <typename T, unsigned S>
inline unsigned arraysize(const T(&v)[S]) { return S; }

Colorf readColor(Bit::JsonTree* configNode)
{
	Colorf color;
	color.r = configNode->getChildPtr("r")->getValue<float>() / 255.0;
	color.g = configNode->getChildPtr("g")->getValue<float>() / 255.0;
	color.b = configNode->getChildPtr("b")->getValue<float>() / 255.0;

	return color;
}

void VertexConnectionAnimation::readConfig(Bit::JsonTree* tree)
{
	depthRotationTime_ = tree->getChildPtr("depthRotationTime")->getValueT<float>();
	autoplayBurstVelocityFactor_ = tree->getChildPtr("autoplayBurstVelocityFactor")->getValueT<float>();
	idleBurstVelocityFactor_ = tree->getChildPtr("idleBurstVelocityFactor")->getValueT<float>();	
	accelerationFactor_ = tree->getChildPtr("accelerationFactor")->getValueT<float>();
	accelerationTimeFactor_ = tree->getChildPtr("accelerationTimeFactor")->getValueT<float>();
	maxLength_ = tree->getChildPtr("maxLength")->getValueT<float>();
	floatingMaxLength_ = tree->getChildPtr("floatingMaxLength")->getValueT<float>();
	maxAlpha_ = tree->getChildPtr("maxAlpha")->getValueT<float>();
	depthMagnitude_ = tree->getChildPtr("depthMagnitude")->getValueT<float>();
	velocityDampingFactor_ = tree->getChildPtr("velocityDampingFactor")->getValueT<float>();
	contourDistance_ = tree->getChildPtr("contourDistance")->getValueT<float>();
	animationWaitTime_ = tree->getChildPtr("animationWaitTime")->getValueT<float>();
	pointNum_ = tree->getChildPtr("pointNum")->getValueT<float>();

	colorSwapTime_ = tree->getChildPtr("colorSwapTime")->getValueT<float>();

	windowWidth_ = tree->getChildPtr("windowWidth")->getValueT<float>();
	windowHeight_ = tree->getChildPtr("windowHeight")->getValueT<float>();

	Bit::JsonTree *fileListTree = tree->getChildPtr("fileList");
	for (Bit::JsonTree::Iter fileListNode = fileListTree->begin(); fileListNode != fileListTree->end(); fileListNode++)
	{
		FileInfo info;
		info.title = Bit::JsonTree::toBitJsonTreePtr(*fileListNode)->getChildPtr("title")->getValueT<string>();
		info.path = Bit::JsonTree::toBitJsonTreePtr(*fileListNode)->getChildPtr("path")->getValueT<string>();
		fileList_.push_back(info);
	}

	Bit::JsonTree *colorPresetTree = tree->getChildPtr("colorPresets");
	for (Bit::JsonTree::Iter colorPresetNode = colorPresetTree->begin(); colorPresetNode != colorPresetTree->end(); colorPresetNode++)
	{
		ColorPreset preset;
		preset.title = Bit::JsonTree::toBitJsonTreePtr(*colorPresetNode)->getChildPtr("title")->getValueT<string>();
		preset.brighterLines = Bit::JsonTree::toBitJsonTreePtr(*colorPresetNode)->getChildPtr("brighterLines")->getValueT<bool>();
		preset.gradientAngle = Bit::JsonTree::toBitJsonTreePtr(*colorPresetNode)->getChildPtr("gradientAngle")->getValueT<float>();

		if (Bit::JsonTree::toBitJsonTreePtr(*colorPresetNode)->getChildPtr("pointsColor")->hasChild("auto"))
		{
			preset.autoColor = true;
			preset.pointsColor = Colorf(1, 1, 1);
		}
		else
		{
			preset.autoColor = false;
			preset.pointsColor = readColor(Bit::JsonTree::toBitJsonTreePtr(*colorPresetNode)->getChildPtr("pointsColor"));
		}
		
		preset.bg1Color = readColor(Bit::JsonTree::toBitJsonTreePtr(*colorPresetNode)->getChildPtr("bg1Color"));
		preset.bg2Color = readColor(Bit::JsonTree::toBitJsonTreePtr(*colorPresetNode)->getChildPtr("bg2Color"));
		
		colorPresets_.push_back(preset);
	}
}

void VertexConnectionAnimation::readParams(Bit::JsonTree* tree, Bit::ParamsRef params)
{
	params->addParam<bool>(tree->getChildPtr("autoPlay"), autoPlay_, "Autoplay Animations");
	
	params->addButton("Idle Mode", std::bind(&VertexConnectionAnimation::setImageIndex, this, -1), "group='" + tree->getKey() + "'");

	for (int i = 0; i < fileList_.size(); i++)
	{
		FileInfo info = fileList_[i];
		params->addButton(info.title, std::bind(&VertexConnectionAnimation::setImageIndex, this, i), "group='" + tree->getKey() + "'");
	}
	params->addSeparator("", "group='" + tree->getKey() + "'");

	for (int i = 0; i < colorPresets_.size(); i++)
	{
		params->addButton(colorPresets_[i].title, std::bind(&VertexConnectionAnimation::applyPreset, this, i), "group='" + tree->getKey() + "'");
	}

	params->addSeparator("", "group='" + tree->getKey() + "'");
	params->addParam<Vec2f>(tree->getChildPtr("displayOffset"), displayOffset_, "Display Offset").group(tree->getKey());
	params->addParam<bool>(tree->getChildPtr("colorSolid"), colorSolid_, "Display Solid Color");
	params->addParam<float>(tree->getChildPtr("gradientAngle"), gradientAngle_, "Gradient Angle");
	params->addParam<bool>(tree->getChildPtr("brighterLines"), brighterLines_, "Brighter Lines");	

	((ci::params::InterfaceGl)(*params)).addParam("Solid Color", &pointsColorParam_).group(tree->getKey());
	((ci::params::InterfaceGl)(*params)).addParam("Background Color 1", &bgColor1Param_).group(tree->getKey());
	((ci::params::InterfaceGl)(*params)).addParam("Background Color 2", &bgColor2Param_).group(tree->getKey());

	params->addButton("Apply Colors", std::bind(&VertexConnectionAnimation::applyColors, this), "group='" + tree->getKey() + "'");
}

void VertexConnectionAnimation::applyPreset(int index)
{
	ColorPreset preset = colorPresets_[index];

	pointsColorParam_ = preset.pointsColor;
	bgColor1Param_ = preset.bg1Color;
	bgColor2Param_ = preset.bg2Color;

	colorSolid_ = !preset.autoColor;
	gradientAngle_ = preset.gradientAngle;
	brighterLines_ = preset.brighterLines;

	applyColors();
}

void VertexConnectionAnimation::applyColors()
{
	previousPointsColor_ = pointsColor_;
	previousBgColor1_ = bgColor1_;
	previousBgColor2_ = bgColor2_;

	pointsColor_ = pointsColorParam_;	
	bgColor1_ = bgColor1Param_;
	bgColor2_ = bgColor2Param_;
	colorSwapStartTime_ = app::getElapsedSeconds();
}

void VertexConnectionAnimation::setImageIndex(int index)
{
	previousIndex_ = imageIndex_;
	imageIndex_ = index;
	stateStartTime_ = ci::app::getElapsedSeconds();

	if (index == -1)
	{
		randomBurst(idleBurstVelocityFactor_);
		autoPlay_ = false;
	}
	else
	{
		randomBurst(autoplayBurstVelocityFactor_);
	}
}

void VertexConnectionAnimation::initVariables()
{
	ci::Rand rand(0);
	for (int i = 0; i < RAND_SIZE; i++)
	{
		rands_[i] = rand.nextFloat();
	}

	stateStartTime_ = ci::app::getElapsedSeconds();

	imageIndex_ = -1;
	previousIndex_ = -1;
	
	//color params
	bgColor1Param_ = bgColor1_ = Colorf::black();
	bgColor2Param_ = bgColor2_ = Colorf::black();
	pointsColorParam_ = pointsColor_ = ci::Colorf::white();	

	rePopulatePoints();

	depths_.resize(pointNum_);
	verticesLocation_.resize(pointNum_);
	verticesSpeed_.resize(pointNum_);

	resetVertices();
}

void VertexConnectionAnimation::resetVertices()
{
	for (int i = 0; i < pointNum_; i++)
	{
		verticesLocation_[i] = (Vec2f(ci::Rand().randFloat(), ci::Rand().randFloat()) * 2 - Vec2f(1.f, 1.f)) * 0.5f;
		verticesSpeed_[i] = (Vec2f(ci::Rand().randFloat(), ci::Rand().randFloat()) * 2 - Vec2f(1.f, 1.f)) * 0.001f;
	}
}

void VertexConnectionAnimation::setup()
{
	for (int i = 0; i < fileList_.size(); i++)
	{
		FileInfo info = fileList_[i];
		loadFile(info.path);
	}

	initVariables();
}

void VertexConnectionAnimation::rePopulatePoints()
{
	for (int i = 0; i < imagePoints_.size(); i++)
	{
		int pointsSize = imagePoints_[i].size();
		vector<cv::Point2d> points;

		for (int j = 0; j < pointNum_; j++)
		{
			int index = j * pointsSize / pointNum_;
			points.push_back(imagePoints_[i][index]);
		}						

		std::shuffle(points.begin(), points.end(), std::default_random_engine(0));
		imagePoints_[i] = points;

		//shuffle
		//std::shuffle(imagePoints_[i].begin(), imagePoints_[i].end(), std::default_random_engine(0));
	}
}

Colorf VertexConnectionAnimation::getVertexColor(int index)
{
	if (colorSolid_)
	{
		float ratio = (app::getElapsedSeconds() - colorSwapStartTime_) / colorSwapTime_;
		
		if (ratio < 1)
		{
			return (ratio) * pointsColor_ + (1 - ratio) * previousPointsColor_;
		}
		else
		{
			return pointsColor_;
		}
	}

	return Colorf(0.5 + 0.5 * (rands_[index]), 0.5 + 0.5 * ((3 * rands_[index]) - (int)(3 * rands_[index])), 0.5 + 0.5 * ((10 * rands_[index]) - (int)(10 * rands_[index])));
	//return Colorf::white();
}

void VertexConnectionAnimation::update()
{
	if (autoPlay_ && imageIndex_ == -1)
		imageIndex_ = 0;

	float framePos = fmod(ci::app::getElapsedSeconds(), depthRotationTime_) / depthRotationTime_;

	//0 ~ 1 => 0 ~ 0.5
	//1 ~ 0 => 0.5 ~ 1.0

	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		float framePosAdjusted = rands_[i] + framePos;

		if (framePosAdjusted > 1.f)
			framePosAdjusted -= 1.f;

		depths_[i] = sin(framePosAdjusted * 2 * M_PI);
	}

	float currentTime = ci::app::getElapsedSeconds();
	
	calculateVertexMovementToDestination();
	updateVertexLocation();
	updateVertexConnection();

	if (autoPlay_ && animationWaitTime_ > 0 && currentTime - stateStartTime_ > animationWaitTime_ && imagePoints_.size() > 0)
	{
		imageIndex_ = (imageIndex_ + 1) % imagePoints_.size();
		setImageIndex(imageIndex_);
		
		//stateStartTime_ = currentTime;
		////random velocity (do not random when there is only one image)
		//if (imagePoints_.size() > 1)
		//{
		//	randomBurst(autoplayBurstVelocityFactor_);
		//}
	}
}

void VertexConnectionAnimation::randomBurst(float speed)
{
	for (int i = 0; i < verticesSpeed_.size(); i++)
		verticesSpeed_[i] = (Vec2f(ci::Rand().randFloat(), ci::Rand().randFloat()) * 2 - Vec2f(1.f, 1.f)) * speed;
}

void VertexConnectionAnimation::calculateVertexMovementToDestination()
{
	if (imagePoints_.size() == 0)
		return;

	if (imageIndex_ < 0)
		return;

	//update speed
	for (int i = 0; i < imagePoints_[imageIndex_].size(); i++)
	{
		Vec2f destination = Vec2f(imagePoints_[imageIndex_][i].x, imagePoints_[imageIndex_][i].y) + 0.001 * displayOffset_ * Vec2f(1, -1);
		Vec2f distanceVector = destination - verticesLocation_[i];

		float distance = distanceVector.length();

		//acceleration ~ distance
		//float acceleration = distance * ACCELERATION_FACTOR - DECELERATION_FACTOR;

		float currentTime = ci::app::getElapsedSeconds();

		float acceleration;
		
		//if (autoPlay_)
		//	acceleration = accelerationFactor_ * (accelerationTimeFactor_ * ((currentTime - stateStartTime_) / animationWaitTime_) + (1 - accelerationTimeFactor_));
		//else
		
		acceleration = accelerationFactor_;

		if (acceleration < 0.f)
			acceleration = 0.f;

		verticesSpeed_[i] = verticesSpeed_[i] / (1.f + velocityDampingFactor_) + (distanceVector.normalized()) * acceleration;
	}
}

void VertexConnectionAnimation::updateVertexLocation()
{
	//update location
	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		verticesLocation_[i] += verticesSpeed_[i];
	}

	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		if (verticesLocation_[i].x < -windowWidth_ / 2.f)
			verticesLocation_[i].x += windowWidth_;
		else if (verticesLocation_[i].x > windowWidth_ / 2.f)
			verticesLocation_[i].x -= windowWidth_;

		if (verticesLocation_[i].y < -windowHeight_ / 2.f)
			verticesLocation_[i].y += windowHeight_;
		else if (verticesLocation_[i].y > windowHeight_ / 2.f)
			verticesLocation_[i].y -= windowHeight_;

		//if (verticesLocation_[i].x < -windowWidth_ / 2.f)
		//	verticesLocation_[i].x = windowWidth_ / 2.f;
		//else if (verticesLocation_[i].x > windowWidth_ / 2.f)
		//	verticesLocation_[i].x = -windowWidth_ / 2.f;

		//if (verticesLocation_[i].y < -windowHeight_ / 2.f)
		//	verticesLocation_[i].y = windowHeight_ / 2.f;
		//else if (verticesLocation_[i].y > windowHeight_ / 2.f)
		//	verticesLocation_[i].y = -windowHeight_ / 2.f;

	}
}

void VertexConnectionAnimation::updateVertexConnection()
{
	vertices_.clear();
	indices_.clear();
	colors_.clear();

	int next = 0;

	float currentTime = ci::app::getElapsedSeconds();

	float lineLength;

	if (imageIndex_ < 0)
	{
		float ratio = (currentTime - stateStartTime_) / colorSwapTime_;
		
		if (ratio < 1 && previousIndex_ >= 0)
			lineLength = floatingMaxLength_ * ratio + maxLength_ * (1 - ratio);
		else
			lineLength = floatingMaxLength_;
	}
	else
	{
		float ratio = (currentTime - stateStartTime_) / colorSwapTime_;

		if (ratio < 1 && previousIndex_ < 0)
			lineLength = maxLength_ * ratio + floatingMaxLength_ * (1 - ratio);
		else
			lineLength = maxLength_;
	}

	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		Vec3f veci = getLocation(i);
		Colorf colori = getVertexColor(i);

		for (int j = i + 1; j < verticesLocation_.size(); j++)
		{
			Vec3f vecj = getLocation(j);
			Colorf colorj = getVertexColor(j);

			float distance = (vecj - veci).length();
			float alpha = 0;

			if (distance > lineLength)
				continue;
			else
			{
				if (brighterLines_)
				{
					//alpha = 0.5 + 0.5 * maxAlpha_ * (lineLength - distance) / lineLength;
					alpha = maxAlpha_ * (lineLength - distance) / lineLength;
					alpha = (1 - (1 - alpha) * (1 - alpha));
				}
				else
				{
					alpha = maxAlpha_ * (lineLength - distance) / lineLength;
				}
			}

			indices_.push_back(next++);
			vertices_.push_back(veci);
			colors_.push_back(ColorAf(colori, alpha));

			indices_.push_back(next++);
			vertices_.push_back(vecj);
			colors_.push_back(ColorAf(colorj, alpha));
		}
	}
}

void VertexConnectionAnimation::draw()
{
	ci::ColorAf bg1;
	ci::ColorAf bg2;

	float ratio = (app::getElapsedSeconds() - colorSwapStartTime_) / colorSwapTime_;

	if (ratio < 1)
	{
		bg1 = (ratio) * bgColor1_ + (1 - ratio) * previousBgColor1_;
		bg2 = (ratio) * bgColor2_ + (1 - ratio) * previousBgColor2_;
	}
	else
	{
		bg1 = bgColor1_;
		bg2 = bgColor2_;
	}

	//gl::clear(bgColor_);
	gl::pushMatrices();
	gl::rotate(gradientAngle_);
		glBegin(GL_QUADS);
		gl::color(bg1);
		glVertex2f(windowWidth_, windowHeight_);
		glVertex2f(-windowWidth_, windowHeight_);
		gl::color(bg2);
		glVertex2f(-windowWidth_, -windowHeight_);
		glVertex2f(windowWidth_, -windowHeight_);
		glEnd();
	gl::popMatrices();

	gl::pushMatrices();

	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticIndices();
	layout.setStaticColorsRGBA();

	gl::VboMesh mesh(vertices_.size(), indices_.size(), layout, GL_LINES);
	if (brighterLines_)
		glLineWidth(1.5f);
	else
		glLineWidth(1.0f);

	mesh.bufferPositions(vertices_);
	mesh.bufferIndices(indices_);
	mesh.bufferColorsRGBA(colors_);
	gl::draw(mesh);

	glPointSize(0.002f);
	gl::begin(GL_POINTS);
	for (int i = 0; i < verticesLocation_.size(); i++)
	{
		
		//gl::drawSphere(getLocation(i), pointRadius);
		gl::color(ci::ColorAf(getVertexColor(i), 0.5f));

		Vec3f location = getLocation(i);
		glVertex3f(location.x, location.y, location.z);//upper-right corner
	}
	gl::end();


	gl::color(Color::white());
	gl::popMatrices();
}

bool myfunction(vector<cv::Point2d> i, vector<cv::Point2d> j) { return (i[0].x < j[0].x); }

Vec3f VertexConnectionAnimation::getLocation(int vertexIndex)
{
	return Vec3f(verticesLocation_[vertexIndex].x, verticesLocation_[vertexIndex].y, depths_[vertexIndex] * depthMagnitude_);
}

void VertexConnectionAnimation::loadFile(string filename)
{
	ci::DataSourceRef dataSourceRef = ci::app::loadAsset(filename);
	ci::Surface image = ci::loadImage(dataSourceRef);
	cv::Mat mat = toOcv(image);
	imagePoints_.push_back(findContour(mat));
}

std::vector<cv::Point2d> VertexConnectionAnimation::findContour(cv::Mat mat)
{
	std::vector<cv::Point2d> vertices;

	cv::flip(mat, mat, 0);
	
	cv::cvtColor(mat, mat, CV_BGR2GRAY);
	cv::Canny(mat, mat, 100, 80);

	vector<vector<cv::Point>> contoursInt;
	cv::findContours(mat, contoursInt, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
	vector<vector<cv::Point2d>> contours;

	for (int i = 0; i < contoursInt.size(); i++)
	{
		vector<cv::Point2d> contour;
		contour.insert(contour.end(), contoursInt[i].begin(), contoursInt[i].end());
		contours.push_back(contour);
	}

	std::sort(contours.begin(), contours.end(), myfunction);

	cv::Point2d center = cv::Point2d(mat.cols / 2, mat.rows / 2);

	//scale points to about -1, 1
	float scale = 1.0f / sqrt(mat.cols * mat.cols + mat.rows * mat.rows); //max(mat.cols, mat.rows);

	for (int i = 0; i < contours.size(); i++)
	{
		for (int j = 0; j < contours[i].size(); j++)
		{
			contours[i][j] = (contours[i][j] - center) * scale;
		}
	}

	int idx = 0;
	for (vector<cv::Point2d> contour : contours)
	{
		for (int i = 0; i < contour.size(); i++)
		{
			cv::Point2d point = contour[i];

			//check if there are nearby vertices
			float hasNearbyPoints = false;

			for (cv::Point2d pointInArray : vertices)
			{
				float dist = sqrt((pointInArray - (cv::Point2d)point).ddot(pointInArray - (cv::Point2d)point));
				if (dist < contourDistance_)
				{
					hasNearbyPoints = true;
					break;
				}
			}

			if (hasNearbyPoints)
				continue;

			vertices.push_back(point);

			int j;
			if (i != contour.size() - 1)
				j = i + 1;
			else
				j = 0;

			cv::Point2d point2 = contour[j];
			float dist = sqrt((point2 - point).ddot(point2 - point));
		}
	}

	return vertices;
}

void VertexConnectionAnimation::reset()
{
	stateStartTime_ = ci::app::getElapsedSeconds();
	resetVertices();
	setImageIndex(-1);
}