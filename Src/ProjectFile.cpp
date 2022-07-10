// SPDX-License-Identifier: GPL-2.0-or-later
/** 
 * @file  ProjectFile.cpp
 *
 * @brief Implementation file for ProjectFile class.
 */

#include "pch.h"
#include "ProjectFile.h"
#include <stack>
#include <string>
#include <Poco/FileStream.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/SAX/SAXParser.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/Exception.h>
#include "UnicodeString.h"
#include "unicoder.h"

using Poco::FileStream;
using Poco::XML::SAXParser;
using Poco::XML::ContentHandler;
using Poco::XML::Locator;
using Poco::XML::XMLWriter;
using Poco::XML::XMLChar;
using Poco::XML::XMLString;
using Poco::XML::Attributes;
using Poco::Exception;
using ucr::toTString;
using ucr::toUTF8;

// Constants for xml element names
const char Root_element_name[] = "project";
const char Paths_element_name[] = "paths";
const char Left_element_name[] = "left";
const char Middle_element_name[] = "middle";
const char Right_element_name[] = "right";
const char Filter_element_name[] = "filter";
const char Subfolders_element_name[] = "subfolders";
const char Left_ro_element_name[] = "left-readonly";
const char Middle_ro_element_name[] = "middle-readonly";
const char Right_ro_element_name[] = "right-readonly";
const char Unpacker_element_name[] = "unpacker";
const char Prediffer_element_name[] = "prediffer";
const char White_spaces_element_name[] = "white-spaces";
const char Ignore_blank_lines_element_name[] = "ignore-blank-lines";
const char Ignore_case_element_name[] = "ignore-case";
const char Ignore_cr_diff_element_name[] = "ignore-carriage-return-diff";
const char Ignore_numbers_element_name[] = "ignore-numbers";
const char Ignore_codepage_diff_element_name[] = "ignore-codepage-diff";
const char Ignore_comment_diff_element_name[] = "ignore-comment-diff";
const char Compare_method_element_name[] = "compare-method";
const char Hidden_list_element_name[] = "hidden-list";
const char Hidden_items_element_name[] = "hidden-item";

namespace
{

String xmlch2tstr(const XMLChar *ch, int length)
{
	return toTString(std::string(ch, length));
}

void writeElement(XMLWriter& writer, const std::string& tagname, const std::string& characters)
{
	writer.startElement("", "", tagname);
	writer.characters(characters);
	writer.endElement("", "", tagname);
}

void saveHiddenItems(XMLWriter& writer, const std::vector<String>& hiddenItems) 
{
	writer.startElement("", "", Hidden_list_element_name);
	for (const auto& hiddenItem : hiddenItems) {
		writeElement(writer, Hidden_items_element_name, toUTF8(hiddenItem));
	}
	writer.endElement("", "", Hidden_list_element_name);
}

}

class ProjectFileHandler: public ContentHandler
{
public:
	explicit ProjectFileHandler(std::list<ProjectFileItem> *pProject) : m_pProject(pProject) {}

	void setDocumentLocator(const Locator* loc) {}
	void startDocument() {}
	void endDocument() {}
	void startElement(const XMLString& uri, const XMLString& localName, const XMLString& qname, const Attributes& attributes)
	{
		if (localName == Paths_element_name)
			m_pProject->push_back(ProjectFileItem{});
		m_stack.push(localName);
	}
	void endElement(const XMLString& uri, const XMLString& localName, const XMLString& qname)
	{
		m_stack.pop();
	}
	void characters(const XMLChar ch[], int start, int length)
	{
		//process .winmerge entries of third level deep only
		if (m_stack.size() != 3 && m_pProject->size() == 0)
			return;

		ProjectFileItem& currentItem = m_pProject->back();

		const std::string& nodename = m_stack.top();
		const std::string& token = std::string(ch + start, length);

		if (nodename == Left_element_name)
		{
			currentItem.m_paths.SetLeft(currentItem.m_paths.GetLeft() + xmlch2tstr(ch + start, length), false);
			currentItem.m_bHasLeft = true;
		}
		else if (nodename == Middle_element_name)
		{
			currentItem.m_paths.SetMiddle(currentItem.m_paths.GetMiddle() + xmlch2tstr(ch + start, length), false);
			currentItem.m_bHasMiddle = true;
		}
		else if (nodename == Right_element_name)
		{
			currentItem.m_paths.SetRight(currentItem.m_paths.GetRight() + xmlch2tstr(ch + start, length), false);
			currentItem.m_bHasRight = true;
		}
		else if (nodename == Filter_element_name)
		{
			currentItem.m_filter += xmlch2tstr(ch + start, length);
			currentItem.m_bHasFilter = true;
		}
		else if (nodename == Subfolders_element_name)
		{
			currentItem.m_subfolders = atoi(token.c_str());
			currentItem.m_bHasSubfolders = true;
		}
		else if (nodename == Left_ro_element_name)
		{
			currentItem.m_bLeftReadOnly = atoi(token.c_str()) != 0;
		}
		else if (nodename == Middle_ro_element_name)
		{
			currentItem.m_bMiddleReadOnly = atoi(token.c_str()) != 0;
		}
		else if (nodename == Right_ro_element_name)
		{
			currentItem.m_bRightReadOnly = atoi(token.c_str()) != 0;
		}
		else if (nodename == Unpacker_element_name)
		{
			currentItem.m_unpacker += xmlch2tstr(ch + start, length);
			currentItem.m_bHasUnpacker = true;
		}
		else if (nodename == Prediffer_element_name)
		{
			currentItem.m_prediffer += xmlch2tstr(ch + start, length);
			currentItem.m_bHasPrediffer = true;
		}
		else if (nodename == White_spaces_element_name)
		{
			currentItem.m_nIgnoreWhite = atoi(token.c_str());
			currentItem.m_bHasIgnoreWhite = true;
		}
		else if (nodename == Ignore_blank_lines_element_name)
		{
			currentItem.m_bIgnoreBlankLines = atoi(token.c_str()) != 0;
			currentItem.m_bHasIgnoreBlankLines = true;
		}
		else if (nodename == Ignore_case_element_name)
		{
			currentItem.m_bIgnoreCase = atoi(token.c_str()) != 0;
			currentItem.m_bHasIgnoreCase = true;
		}
		else if (nodename == Ignore_cr_diff_element_name)
		{
			currentItem.m_bIgnoreEol = atoi(token.c_str()) != 0;
			currentItem.m_bHasIgnoreEol = true;
		}
		else if (nodename == Ignore_numbers_element_name)
		{
			currentItem.m_bIgnoreNumbers = atoi(token.c_str()) != 0;
			currentItem.m_bHasIgnoreNumbers = true;
		}
		else if (nodename == Ignore_codepage_diff_element_name)
		{
			currentItem.m_bIgnoreCodepage = atoi(token.c_str()) != 0;
			currentItem.m_bHasIgnoreCodepage = true;
		}
		else if (nodename == Ignore_comment_diff_element_name)
		{
			currentItem.m_bFilterCommentsLines = atoi(token.c_str()) != 0;
			currentItem.m_bHasFilterCommentsLines = true;
		}
		else if (nodename == Compare_method_element_name)
		{
			currentItem.m_nCompareMethod = atoi(token.c_str());
			currentItem.m_bHasCompareMethod = true;
		}
		//This nodes are under Hidden_list_element_name
		else if (nodename ==  Hidden_items_element_name)
		{
			currentItem.m_vSavedHiddenItems.push_back(toTString(token));
			currentItem.m_bHasHiddenItems = true;
		}
	}
	void ignorableWhitespace(const XMLChar ch[], int start, int length)	{}
	void processingInstruction(const XMLString& target, const XMLString& data) {}
	void startPrefixMapping(const XMLString& prefix, const XMLString& uri) {}
	void endPrefixMapping(const XMLString& prefix) {}
	void skippedEntity(const XMLString& name) {}

private:
	std::list<ProjectFileItem> *m_pProject = nullptr;
	std::stack<std::string> m_stack;
};

/** @brief File extension for path files */
const String ProjectFile::PROJECTFILE_EXT = toTString("WinMerge");

/** 
 * @brief Standard constructor.
 */
 ProjectFileItem::ProjectFileItem()
: m_bHasLeft(false)
, m_bHasMiddle(false)
, m_bHasRight(false)
, m_bHasFilter(false)
, m_bHasSubfolders(false)
, m_bHasUnpacker(false)
, m_bHasPrediffer(false)
, m_subfolders(-1)
, m_bLeftReadOnly(false)
, m_bMiddleReadOnly(false)
, m_bRightReadOnly(false)
, m_bHasIgnoreWhite(false)
, m_nIgnoreWhite(0)
, m_bHasIgnoreBlankLines(false)
, m_bIgnoreBlankLines(false)
, m_bHasIgnoreCase(false)
, m_bIgnoreCase(false)
, m_bHasIgnoreEol(false)
, m_bIgnoreEol(false)
, m_bHasIgnoreNumbers(false)
, m_bIgnoreNumbers(false)
, m_bHasIgnoreCodepage(false)
, m_bIgnoreCodepage(false)
, m_bHasFilterCommentsLines(false)
, m_bFilterCommentsLines(false)
, m_bHasCompareMethod(false)
, m_bHasHiddenItems(false)
, m_nCompareMethod(0)
, m_bSaveFilter(true)
, m_bSaveSubfolders(true)
, m_bSaveUnpacker(true)
, m_bSaveIgnoreWhite(true)
, m_bSaveIgnoreBlankLines(true)
, m_bSaveIgnoreCase(true)
, m_bSaveIgnoreEol(true)
, m_bSaveIgnoreNumbers(true)
, m_bSaveIgnoreCodepage(true)
, m_bSaveFilterCommentsLines(true)
, m_bSaveCompareMethod(true)
, m_bSaveHiddenItems(true)
{
}

/** 
 * @brief Returns left path.
 * @param [out] pReadOnly true if readonly was specified for path.
 * @return Left path.
 */
String ProjectFileItem::GetLeft(bool * pReadOnly /*= nullptr*/) const
{
	if (pReadOnly != nullptr)
		*pReadOnly = m_bLeftReadOnly;
	return m_paths.GetLeft();
}

/** 
 * @brief Set left path, returns old left path.
 * @param [in] sLeft Left path.
 * @param [in] bReadOnly Will path be recorded read-only?
 */
void ProjectFileItem::SetLeft(const String& sLeft, const bool * pReadOnly /*= nullptr*/)
{
	m_paths.SetLeft(sLeft, false);
	if (pReadOnly != nullptr)
		m_bLeftReadOnly = *pReadOnly;
}

/** 
 * @brief Returns middle path.
 * @param [out] pReadOnly true if readonly was specified for path.
 */
String ProjectFileItem::GetMiddle(bool * pReadOnly /*= nullptr*/) const
{
	if (pReadOnly != nullptr)
		*pReadOnly = m_bMiddleReadOnly;
	return m_paths.GetMiddle();
}

/** 
 * @brief Set middle path, returns old middle path.
 * @param [in] sMiddle Middle path.
 * @param [in] bReadOnly Will path be recorded read-only?
 */
void ProjectFileItem::SetMiddle(const String& sMiddle, const bool * pReadOnly /*= nullptr*/)
{
	m_paths.SetMiddle(sMiddle, false);
	if (pReadOnly != nullptr)
		m_bMiddleReadOnly = *pReadOnly;

	return;
}

/** 
 * @brief Returns right path.
 * @param [out] pReadOnly true if readonly was specified for path.
 * @return Right path.
 */
String ProjectFileItem::GetRight(bool * pReadOnly /*= nullptr*/) const
{
	if (pReadOnly != nullptr)
		*pReadOnly = m_bRightReadOnly;
	return m_paths.GetRight();
}

/** 
 * @brief Set right path, returns old right path.
 * @param [in] sRight Right path.
 * @param [in] bReadOnly Will path be recorded read-only?
 */
void ProjectFileItem::SetRight(const String& sRight, const bool * pReadOnly /*= nullptr*/)
{
	m_paths.SetRight(sRight, false);
	if (pReadOnly != nullptr)
		m_bRightReadOnly = *pReadOnly;
}

/** 
 * @brief Returns left and right paths and recursive from project file
 * 
 * @param [out] files Files in project
 * @param [out] bSubFolders If true subfolders included (recursive compare)
 */
void ProjectFileItem::GetPaths(PathContext& files, bool & bSubfolders) const
{
	files = m_paths;
	if (HasSubfolders())
		bSubfolders = (GetSubfolders() == 1);
}

/** 
 * @brief Open given path-file and read data from it to member variables.
 * @param [in] path Path to project file.
 * @param [out] sError Error string if error happened.
 * @return true if reading succeeded, false if error happened.
 */
bool ProjectFile::Read(const String& path)
{
	ProjectFileHandler handler(&m_items);
	SAXParser parser;
	parser.setContentHandler(&handler);
	parser.parse(toUTF8(path));
	return true;
}

/** 
 * @brief Save data from member variables to path-file.
 * @param [in] path Path to project file.
 * @param [out] sError Error string if error happened.
 * @return true if saving succeeded, false if error happened.
 */
bool ProjectFile::Save(const String& path) const
{
	FileStream out(toUTF8(path), FileStream::trunc);
	XMLWriter writer(out, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT);
	writer.startDocument();
	writer.startElement("", "", Root_element_name);
	{
		for (auto& item : m_items)
		{
			writer.startElement("", "", Paths_element_name);
			{
				if (!item.m_paths.GetLeft().empty())
					writeElement(writer, Left_element_name, toUTF8(item.m_paths.GetLeft()));
				if (!item.m_paths.GetMiddle().empty())
					writeElement(writer, Middle_element_name, toUTF8(item.m_paths.GetMiddle()));
				if (!item.m_paths.GetRight().empty())
					writeElement(writer, Right_element_name, toUTF8(item.m_paths.GetRight()));
				if (item.m_bSaveFilter && !item.m_filter.empty())
					writeElement(writer, Filter_element_name, toUTF8(item.m_filter));
				if (item.m_bSaveSubfolders)
					writeElement(writer, Subfolders_element_name, item.m_subfolders != 0 ? "1" : "0");
				writeElement(writer, Left_ro_element_name, item.m_bLeftReadOnly ? "1" : "0");
				if (!item.m_paths.GetMiddle().empty())
					writeElement(writer, Middle_ro_element_name, item.m_bMiddleReadOnly ? "1" : "0");
				writeElement(writer, Right_ro_element_name, item.m_bRightReadOnly ? "1" : "0");
				if (item.m_bSaveUnpacker && !item.m_unpacker.empty())
					writeElement(writer, Unpacker_element_name, toUTF8(item.m_unpacker));
				if (!item.m_prediffer.empty())
					writeElement(writer, Prediffer_element_name, toUTF8(item.m_prediffer));
				if (item.m_bSaveIgnoreWhite)
					writeElement(writer, White_spaces_element_name, std::to_string(item.m_nIgnoreWhite));
				if (item.m_bSaveIgnoreBlankLines)
					writeElement(writer, Ignore_blank_lines_element_name, item.m_bIgnoreBlankLines ? "1" : "0");
				if (item.m_bSaveIgnoreCase)
					writeElement(writer, Ignore_case_element_name, item.m_bIgnoreCase ? "1" : "0");
				if (item.m_bSaveIgnoreEol)
					writeElement(writer, Ignore_cr_diff_element_name, item.m_bIgnoreEol ? "1" : "0");
				if (item.m_bSaveIgnoreNumbers)
					writeElement(writer, Ignore_numbers_element_name, item.m_bIgnoreNumbers ? "1" : "0");
				if (item.m_bSaveIgnoreCodepage)
					writeElement(writer, Ignore_codepage_diff_element_name, item.m_bIgnoreCodepage ? "1" : "0");
				if (item.m_bSaveFilterCommentsLines)
					writeElement(writer, Ignore_comment_diff_element_name, item.m_bFilterCommentsLines ? "1" : "0");
				if (item.m_bSaveCompareMethod)
					writeElement(writer, Compare_method_element_name, std::to_string(item.m_nCompareMethod));
				if (item.m_bSaveHiddenItems && item.m_vSavedHiddenItems.size() > 0) 
					saveHiddenItems(writer, item.m_vSavedHiddenItems);
			}
			writer.endElement("", "", Paths_element_name);
		}
	}
	writer.endElement("", "", Root_element_name);
	writer.endDocument();
	return true;
}
