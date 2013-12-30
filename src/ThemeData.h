#pragma once

#include <iostream>
#include <sstream>
#include <memory>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/variant.hpp>
#include <Eigen/Dense>
#include "pugiXML/pugixml.hpp"
#include "GuiComponent.h"

template<typename T>
class TextListComponent;

class Sound;
class ImageComponent;
class NinePatchComponent;
class TextComponent;
class Window;

namespace ThemeFlags
{
	enum PropertyFlags : unsigned int
	{
		PATH = 1,
		POSITION = 2,
		SIZE = 4,
		ORIGIN = 8,
		COLOR = 16,
		FONT_PATH = 32,
		FONT_SIZE = 64,
		TILING = 128,
		SOUND = 256,
		CENTER = 512,
		TEXT = 1024
	};
}

class ThemeException : public std::exception
{
protected:
	std::string msg;

public:
	virtual const char* what() const throw() { return msg.c_str(); }

	template<typename T>
	friend ThemeException& operator<<(ThemeException& e, T msg);
	
	inline void setFile(const std::string& filename) { *this << "Error loading theme from \"" << filename << "\":\n   "; }
};

template<typename T>
ThemeException& operator<<(ThemeException& e, T appendMsg)
{
	std::stringstream ss;
	ss << e.msg << appendMsg;
	e.msg = ss.str();
	return e;
}

class ThemeData
{
private:

	class ThemeElement
	{
	public:
		bool extra;
		std::string type;

		std::map< std::string, boost::variant<Eigen::Vector2f, std::string, unsigned int, float, bool> > properties;

		template<typename T>
		T get(const std::string& prop) { return boost::get<T>(properties.at(prop)); }

		inline bool has(const std::string& prop) { return (properties.find(prop) != properties.end()); }
	};

	class ThemeView
	{
	public:
		ThemeView() : mExtrasDirty(true) {}
		virtual ~ThemeView() { for(auto it = mExtras.begin(); it != mExtras.end(); it++) delete *it; }

		std::map<std::string, ThemeElement> elements;
		
		const std::vector<GuiComponent*>& getExtras(Window* window);

	private:
		bool mExtrasDirty;
		std::vector<GuiComponent*> mExtras;
	};

public:

	ThemeData();

	// throws ThemeException
	void loadFile(const std::string& path);

	enum ElementPropertyType
	{
		NORMALIZED_PAIR,
		PATH,
		STRING,
		COLOR,
		FLOAT,
		BOOLEAN
	};

	void renderExtras(const std::string& view, Window* window, const Eigen::Affine3f& transform);

	void applyToImage(const std::string& view, const std::string& element, ImageComponent* image, unsigned int properties);
	void applyToNinePatch(const std::string& view, const std::string& element, NinePatchComponent* patch, unsigned int properties);
	void applyToText(const std::string& view, const std::string& element, TextComponent* text, unsigned int properties);

	template <typename T>
	void applyToTextList(const std::string& view, const std::string& element, TextListComponent<T>* list, unsigned int properties);

	void playSound(const std::string& name);

private:
	static std::map< std::string, std::map<std::string, ElementPropertyType> > sElementMap;

	boost::filesystem::path mPath;
	float mVersion;

	ThemeView parseView(const pugi::xml_node& view);
	ThemeElement parseElement(const pugi::xml_node& element, const std::map<std::string, ElementPropertyType>& typeMap);

	ThemeElement* getElement(const std::string& viewName, const std::string& elementName);

	std::map<std::string, ThemeView> mViews;

	std::map< std::string, std::shared_ptr<Sound> > mSoundCache;
};


template <typename T>
void ThemeData::applyToTextList(const std::string& view, const std::string& element, TextListComponent<T>* list, unsigned int properties)
{

}
