#include "ThemeData.h"
#include "Renderer.h"
#include "resources/Font.h"
#include "Sound.h"
#include "resources/TextureResource.h"
#include "Log.h"
#include "pugiXML/pugixml.hpp"
#include <boost/assign.hpp>

std::map< std::string, std::map<std::string, ThemeData::ElementPropertyType> > ThemeData::sElementMap = boost::assign::map_list_of
	("image", boost::assign::map_list_of
		("pos", NORMALIZED_PAIR)
		("size", NORMALIZED_PAIR)
		("origin", NORMALIZED_PAIR)
		("path", PATH)
		("tile", BOOLEAN))
	("text", boost::assign::map_list_of
		("pos", NORMALIZED_PAIR)
		("size", NORMALIZED_PAIR)
		("text", STRING)
		("color", COLOR)
		("fontPath", PATH)
		("fontSize", FLOAT)
		("center", BOOLEAN))
	("textlist", boost::assign::map_list_of
		("pos", NORMALIZED_PAIR)
		("size", NORMALIZED_PAIR)
		("selectorColor", COLOR)
		("selectedColor", COLOR)
		("primaryColor", COLOR)
		("secondaryColor", COLOR)
		("fontPath", PATH)
		("fontSize", FLOAT))
	("sound", boost::assign::map_list_of
		("path", PATH));

namespace fs = boost::filesystem;

#define MINIMUM_THEME_VERSION 3
#define CURRENT_THEME_VERSION 3

// still TODO:
// * how to do <include>?

ThemeData::ThemeData()
{
	mVersion = 0;
}

void ThemeData::loadFile(const std::string& path)
{
	ThemeException error;
	error.setFile(path);

	mPath = path;

	if(!fs::exists(path))
		throw error << "Missing file!";

	mVersion = 0;
	mViews.clear();

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(path.c_str());
	if(!res)
		throw error << "XML parsing error: \n    " << res.description();

	pugi::xml_node root = doc.child("theme");
	if(!root)
		throw error << "Missing <theme> tag!";

	// parse version
	mVersion = root.child("version").text().as_float(-404);
	if(mVersion == -404)
		throw error << "<version> tag missing!\n   It's either out of date or you need to add <version>" << CURRENT_THEME_VERSION << "</version> inside your <theme> tag.";


	if(mVersion < MINIMUM_THEME_VERSION)
		throw error << "Theme is version " << mVersion << ". Minimum supported version is " << MINIMUM_THEME_VERSION << ".";

	// parse views
	for(pugi::xml_node node = root.child("view"); node; node = node.next_sibling("view"))
	{
		if(!node.attribute("name"))
			throw error << "View missing \"name\" attribute!";

		ThemeView view = parseView(node);

		if(view.elements.size() > 0)
			mViews[node.attribute("name").as_string()] = view;
	}
}

ThemeData::ThemeView ThemeData::parseView(const pugi::xml_node& root)
{
	ThemeView view;
	ThemeException error;
	error.setFile(mPath.string());

	for(pugi::xml_node node = root.first_child(); node; node = node.next_sibling())
	{
		if(!node.attribute("name"))
			throw error << "Element of type \"" << node.name() << "\" missing \"name\" attribute!";

		auto elemTypeIt = sElementMap.find(node.name());
		if(elemTypeIt == sElementMap.end())
			throw error << "Unknown element of type \"" << node.name() << "\"!";

		ThemeElement element = parseElement(node, elemTypeIt->second);

		view.elements[node.attribute("name").as_string()] = element;
	}

	return view;
}

unsigned int getHexColor(const char* str)
{
	ThemeException error;
	if(!str)
		throw error << "Empty color";

	size_t len = strlen(str);
	if(len != 6 && len != 8)
		throw error << "Invalid color (bad length, \"" << str << "\" - must be 6 or 8)";

	unsigned int val;
	std::stringstream ss;
	ss << str;
	ss >> std::hex >> val;

	if(len == 6)
		val = (val << 8) | 0xFF;

	return val;
}

std::string resolvePath(const char* in, const fs::path& relative)
{
	if(!in || in[0] == '\0')
		return in;

	fs::path relPath = relative.parent_path();
	
	boost::filesystem::path path(in);
	
	// we use boost filesystem here instead of just string checks because 
	// some directories could theoretically start with ~ or .
	if(*path.begin() == "~")
	{
		path = getHomePath() + (in + 1);
	}else if(*path.begin() == ".")
	{
		path = relPath / (in + 1);
	}

	return path.generic_string();
}


ThemeData::ThemeElement ThemeData::parseElement(const pugi::xml_node& root, const std::map<std::string, ElementPropertyType>& typeMap)
{
	ThemeException error;
	error.setFile(mPath.string());

	ThemeElement element;
	element.extra = root.attribute("extra").as_bool(false);

	for(pugi::xml_node node = root.first_child(); node; node = node.next_sibling())
	{
		auto typeIt = typeMap.find(node.name());
		if(typeIt == typeMap.end())
			throw error << "Unknown property type \"" << node.name() << "\" (for element of type " << root.name() << ").";

		switch(typeIt->second)
		{
		case NORMALIZED_PAIR:
		{
			std::string str = std::string(node.text().as_string());

			size_t divider = str.find(' ');
			if(divider == std::string::npos) 
				throw error << "invalid normalized pair (\"" << str.c_str() << "\")";

			std::string first = str.substr(0, divider);
			std::string second = str.substr(divider, std::string::npos);

			Eigen::Vector2f val(atof(first.c_str()), atof(second.c_str()));

			element.properties[node.name()] = val;
			break;
		}
		case STRING:
			element.properties[node.name()] = std::string(node.text().as_string());
			break;
		case PATH:
		{
			std::string path = resolvePath(node.text().as_string(), mPath.string());
			if(!fs::exists(path))
				LOG(LogWarning) << "  Warning: theme \"" << mPath << "\" - could not find file \"" << node.text().get() << "\" (resolved to \"" << path << "\")";
			element.properties[node.name()] = path;
			break;
		}
		case COLOR:
			element.properties[node.name()] = getHexColor(node.text().as_string());
			break;
		case FLOAT:
			element.properties[node.name()] = node.text().as_float();
			break;
		case BOOLEAN:
			element.properties[node.name()] = node.text().as_bool();
			break;
		default:
			throw error << "Unknown ElementPropertyType for " << root.attribute("name").as_string() << " property " << node.name();
		}
	}

	return element;
}
