//------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Core/XMLUtils/XMLUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Rgba.hpp"

//------------------------------------------------------------------------------------------------------------------------------
std::string ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, const std::string& defaultValue )
{
	if(!xmlElement.Attribute(attributeName))
		return defaultValue;

	std::string s = xmlElement.Attribute(attributeName);
	if(s.size() == 0)
	{
		return defaultValue;
	}
	else
	{
		return s;
	}
}

int ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, int defaultValue )
{
	std::string s = ParseXmlAttribute(xmlElement, attributeName, std::to_string(defaultValue));
	if(s.size() == 0)
	{
		return defaultValue;
	}
	else
		return atoi(s.c_str());
}

uint ParseXmlAttribute(const XMLElement& xmlElement, const char* attributeName, uint defaultValue)
{
	std::string s = ParseXmlAttribute(xmlElement, attributeName, std::to_string(defaultValue));
	if (s.size() == 0)
	{
		return defaultValue;
	}
	else
		return atoi(s.c_str());
}

char ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, char defaultValue )
{
	std::string s = ParseXmlAttribute(xmlElement, attributeName, std::to_string(defaultValue));
	if(s.size() == 1)
	{
		return s[0];
	}
	else
	{
		return defaultValue;
	}
}

bool ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, bool defaultValue )
{
	std::string s = ParseXmlAttribute(xmlElement, attributeName, std::to_string(defaultValue));
	if(s == "true" || s == "True")
		return true;
	else if(s == "false" || s == "False")
	{
		return false;
	}
	else
	{
		return defaultValue;
	}
}

float ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, float defaultValue )
{
	std::string s = ParseXmlAttribute(xmlElement, attributeName, std::to_string(defaultValue));
	if(s.size() == 0)
		return defaultValue;
	else
	{
		return static_cast<float>(atof(s.c_str()));
	}
}

Rgba ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, const Rgba& defaultValue )
{
	std::string defaultString = std::to_string(defaultValue.r) + "," + std::to_string(defaultValue.g) + "," + std::to_string(defaultValue.b) + "," + std::to_string(defaultValue.a);
	std::string s = ParseXmlAttribute(xmlElement, attributeName, defaultString);
	if(s.size() == 0)
	{
		return defaultValue;
	}
	else
	{
		Rgba newColor = Rgba(s.c_str());
		return newColor;
	}
}

Vec2 ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, const Vec2& defaultValue )
{
	std::string defaultString = std::to_string(defaultValue.x) + "," + std::to_string(defaultValue.y);
	std::string s = ParseXmlAttribute(xmlElement, attributeName, defaultString);
	if(s.size() == 0)
	{
		return defaultValue;
	}
	else
	{
		Vec2 newVec2 = Vec2(s.c_str());
		return newVec2;
	}
}

IntRange ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, const IntRange& defaultValue )
{
	std::string defaultString = std::to_string(defaultValue.minInt) + "~" + std::to_string(defaultValue.maxInt);
	std::string s = ParseXmlAttribute(xmlElement, attributeName, defaultString);
	if(s.size() == 0)
	{
		return defaultValue;
	}
	else
	{
		IntRange newIntRange = IntRange(s.c_str());
		return newIntRange;
	}
}

FloatRange ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, const FloatRange& defaultValue )
{
	std::string defaultString = std::to_string(defaultValue.minFloat) + "~" + std::to_string(defaultValue.maxFloat);
	std::string s = ParseXmlAttribute(xmlElement, attributeName, defaultString);
	if(s.size() == 0)
	{
		return defaultValue;
	}
	else
	{
		FloatRange newFloatRange = FloatRange(s.c_str());
		return newFloatRange;
	}
}

IntVec2 ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, const IntVec2& defaultValue )
{
	std::string defaultString = std::to_string(defaultValue.x) + "," + std::to_string(defaultValue.y);
	std::string s = ParseXmlAttribute(xmlElement, attributeName, defaultString);
	if(s.size() == 0)
	{
		return defaultValue;
	}
	else
	{
		IntVec2 newIntVec = IntVec2(s.c_str());
		return newIntVec;
	}
}

std::string ParseXmlAttribute( const XMLElement& xmlElement, const char* attributeName, const char* defaultValue/*=nullptr */ )
{
	if(!xmlElement.Attribute(attributeName))
		return defaultValue;

	std::string s = xmlElement.Attribute(attributeName);
	if(s.size() == 0)
	{
		return defaultValue;
	}
	else
	{
		return s;
	}
}

Vec3 ParseXmlAttribute(const XMLElement& xmlElement, const char* attributeName, const Vec3& defaultValue)
{
	std::string defaultString = std::to_string(defaultValue.x) + "," + std::to_string(defaultValue.y) + "," + std::to_string(defaultValue.z);
	std::string s = ParseXmlAttribute(xmlElement, attributeName, defaultString);
	if (s.size() == 0)
	{
		return defaultValue;
	}
	else
	{
		Vec3 newVec3 = Vec3(s.c_str());
		return newVec3;
	}
}

tinyxml2::XMLNode* ParseXMLDocumentForElement(tinyxml2::XMLElement& root, std::string sSearchString)
{
	//inspired by http://stackoverflow.com/questions/11921463/find-a-specific-node-in-a-xml-document-with-tinyxml
	tinyxml2::XMLNode * xElem = root.FirstChild();
	while (xElem)
	{
		if (xElem->Value() && !std::string(xElem->Value()).compare(sSearchString))
		{
			return xElem;
		}

		/*
		*   We move through the XML tree following these rules (basically in-order tree walk):
		*
		*   (1) if there is one or more child element(s) visit the first one
		*       else
		*   (2)     if there is one or more next sibling element(s) visit the first one
		*               else
		*   (3)             move to the parent until there is one or more next sibling elements
		*   (4)             if we reach the end break the loop
		*/
		if (xElem->FirstChildElement()) //(1)
			xElem = xElem->FirstChildElement();
		else if (xElem->NextSiblingElement())  //(2)
			xElem = xElem->NextSiblingElement();
		else
		{
			while (xElem->Parent() && !xElem->Parent()->NextSiblingElement()) //(3)
				xElem = xElem->Parent();
			if (xElem->Parent() && xElem->Parent()->NextSiblingElement())
				xElem = xElem->Parent()->NextSiblingElement();
			else //(4)
				break;
		}//else
	}//while
}