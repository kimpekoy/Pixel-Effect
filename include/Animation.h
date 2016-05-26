#pragma once

#include "Configurable.h"

class Animation : public Configurable
{
public:
	virtual void readConfig(Bit::JsonTree* tree) {};
	virtual void readParams(Bit::JsonTree* tree, Bit::ParamsRef params) {};
	virtual void setup() {};
	virtual void update() {};
	virtual void draw() {};
	virtual void reset() {};
	virtual void stop() {};
	virtual bool is2D(){ return false; };
	virtual std::string getConfigName(){ return ""; };
	virtual std::string getParamsName(){ return ""; };
};