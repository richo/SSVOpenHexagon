// Copyright (c) 2013 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#include <SSVUtils/SSVUtils.h>
#include <SSVUtilsJson/SSVUtilsJson.h>
#include "SSVOpenHexagon/Data/StyleData.h"
#include "SSVOpenHexagon/Utils/Utils.h"
#include "SSVOpenHexagon/Global/Config.h"

using namespace std;
using namespace sf;
using namespace ssvs::Utils;
using namespace hg::Utils;
using namespace ssvu;
using namespace ssvuj;

namespace hg
{
	Color StyleData::calculateColor(Json::Value mColorRoot)
	{
		Color color{getColorFromJsonArray(mColorRoot["value"])};

		if(mColorRoot["dynamic"].asBool())
		{
			Color dynamicColor{getColorFromHue((currentHue + mColorRoot["hue_shift"].asFloat()) / 360.0f)};

			if(mColorRoot["main"].asBool()) color = dynamicColor;
			else
			{
				if(!mColorRoot["dynamic_offset"].asBool()) color = getColorDarkened(dynamicColor, mColorRoot["dynamic_darkness"].asFloat());
				else
				{
					color.r += dynamicColor.r / mColorRoot["offset"].asFloat();
					color.g += dynamicColor.g / mColorRoot["offset"].asFloat();
					color.b += dynamicColor.b / mColorRoot["offset"].asFloat();
					color.a += dynamicColor.a;
				}
			}
		}

		Color pulse{getColorFromJsonArray(mColorRoot["pulse"])};
		return Color(getClamped(color.r + pulse.r * pulseFactor, 0.f, 255.f),
					 getClamped(color.g + pulse.g * pulseFactor, 0.f, 255.f),
					 getClamped(color.b + pulse.b * pulseFactor, 0.f, 255.f),
					 getClamped(color.a + pulse.a * pulseFactor, 0.f, 255.f));
	}

	StyleData::StyleData(Json::Value mRoot) : root{mRoot}, currentHue{getHueMin()} { }

	void StyleData::update(float mFrameTime)
	{
		currentSwapTime += mFrameTime;
		if(currentSwapTime > getMaxSwapTime()) currentSwapTime = 0;

		currentHue += getHueIncrement() * mFrameTime;

		if(currentHue < getHueMin())
		{
			if(getHuePingPong()) { currentHue = getHueMin(); root["hue_increment"] = getHueIncrement() * -1; }
			else currentHue = getHueMax();
		}
		if(currentHue > getHueMax())
		{
			if(getHuePingPong()) { currentHue = getHueMax(); root["hue_increment"] = getHueIncrement() * -1; }
			else currentHue = getHueMin();
		}

		pulseFactor += root["pulse_increment"].asFloat() * mFrameTime;

		if(pulseFactor < root["pulse_min"].asFloat()) { root["pulse_increment"] = root["pulse_increment"].asFloat() * -1; pulseFactor = root["pulse_min"].asFloat(); }
		if(pulseFactor > root["pulse_max"].asFloat()) { root["pulse_increment"] = root["pulse_increment"].asFloat() * -1; pulseFactor = root["pulse_max"].asFloat(); }
	}

	void StyleData::computeColors()
	{
		currentMainColor = calculateColor(root["main"]);
		current3DOverrideColor = root.isMember("3D_override_color") ? getColorFromJsonArray(root["3D_override_color"]) : getMainColor();
		currentColors.clear();
		for(unsigned int i{0}; i < root["colors"].size(); i++) currentColors.push_back(calculateColor(root["colors"][i]));
		rotate(currentColors.begin(), currentColors.begin() + currentSwapTime / (getMaxSwapTime() / 2), currentColors.end());
	}

	void StyleData::setRootPath(const std::string& mPath) { rootPath = mPath; }
	string StyleData::getRootPath() { return rootPath; }

	string StyleData::getId() 					{ return root["id"].asString(); }
	float StyleData::getHueMin() 				{ return root["hue_min"].asFloat(); }
	float StyleData::getHueMax()				{ return root["hue_max"].asFloat(); }
	bool StyleData::getHuePingPong()			{ return root["hue_ping_pong"].asBool(); }
	float StyleData::getHueIncrement()			{ return root["hue_increment"].asFloat(); }
	float StyleData::getMaxSwapTime()			{ return as<float>(root, "max_swap_time", 100.f); }

	float StyleData::getCurrentHue() 			{ return currentHue; }
	float StyleData::getCurrentSwapTime() 		{ return currentSwapTime; }
	Color StyleData::getMainColor()				{ return currentMainColor; }
	vector<Color> StyleData::getColors() 		{ return currentColors; }

	void StyleData::setValueFloat(const string& mValueName, float mValue)			{ root[mValueName] = mValue; }
	float StyleData::getValueFloat(const string& mValueName)						{ return root[mValueName].asFloat(); }
	void StyleData::setValueInt(const string& mValueName, int mValue)				{ root[mValueName] = mValue; }
	float StyleData::getValueInt(const string& mValueName)							{ return root[mValueName].asInt(); }
	void StyleData::setValueString(const string& mValueName, const string& mValue)	{ root[mValueName] = mValue; }
	string StyleData::getValueString(const string& mValueName)						{ return root[mValueName].asString(); }
	void StyleData::setValueBool(const string& mValueName, bool mValue)				{ root[mValueName] = mValue; }
	bool StyleData::getValueBool(const string& mValueName)							{ return root[mValueName].asBool(); }

	void StyleData::drawBackground(RenderTarget& mRenderTarget, Vector2f mCenterPos, int mSides)
	{
		float div{360.f / mSides * 1.0001f}, distance{4500};

		VertexArray vertices{PrimitiveType::Triangles, 3};
		vector<Color> colors{getColors()};

		for(int i{0}; i < mSides; ++i)
		{
			float angle{div * i};
			Color currentColor{colors[i % colors.size()]};
			if(getBlackAndWhite()) currentColor = Color::Black;

			if(i % 2 == 0) if(i == mSides - 1) currentColor = getColorDarkened(currentColor, 1.4f);

			vertices.append(Vertex{mCenterPos, currentColor});
			vertices.append(Vertex{getOrbitFromDegrees(mCenterPos, angle + div * 0.5f, distance), currentColor});
			vertices.append(Vertex{getOrbitFromDegrees(mCenterPos, angle - div * 0.5f, distance), currentColor});
		}

		mRenderTarget.draw(vertices);
	}

	unsigned int StyleData::get3DDepth() 			{ return as<float>(root, "3D_depth", 15); }
	float StyleData::get3DSkew() 					{ return as<float>(root, "3D_skew", 0.18f); }
	float StyleData::get3DSpacing() 				{ return as<float>(root, "3D_spacing", 1.0f); }
	float StyleData::get3DDarkenMultiplier() 		{ return as<float>(root, "3D_darken_multiplier", 1.5f); }
	float StyleData::get3DAlphaMultiplier() 		{ return as<float>(root, "3D_alpha_multiplier", 0.5f); }
	float StyleData::get3DAlphaFalloff() 			{ return as<float>(root, "3D_alpha_falloff", 3.0f); }
	Color StyleData::get3DOverrideColor() 			{ return current3DOverrideColor; }
	float StyleData::get3DPulseMax()				{ return as<float>(root, "3D_pulse_max", 3.2f); }
	float StyleData::get3DPulseMin()				{ return as<float>(root, "3D_pulse_min", -0.0f); }
	float StyleData::get3DPulseSpeed()				{ return as<float>(root, "3D_pulse_speed", 0.01f); }
	float StyleData::get3DPerspectiveMultiplier()	{ return as<float>(root, "3D_perspective_multiplier", 1.0f); }
}

