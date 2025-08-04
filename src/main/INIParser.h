/*****************************************************************************/
/*  Copyright 2019 WSL Institute for Snow and Avalanche Research  SLF-DAVOS  */
/*****************************************************************************/
/* This file is part of INIshell.
   INIshell is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   INIshell is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with INIshell.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * The top level INI file interface.
 * 2019-10
 */

#ifndef INIPARSER_H
#define INIPARSER_H

#include "src/gui/Logger.h"
#include "src/main/common.h"

#include <algorithm>
#include <list>
#include <map>
#include <set>

#include <QCoreApplication> //for translations
#include <QRegularExpression>
#include <QString>
#include <QTextStream>

class KeyValue {
	public:
		KeyValue();
		KeyValue(QString key, QString value = QString());
		QString getKey() const { return key_; }
		void setKey(const QString &str_key) { key_ = str_key; }
		QString getValue() const { return value_; }
		void setValue(const QString &str_value) { value_ = str_value; }
		QString getInlineComment() const noexcept { return inline_comment_; }
		void setInlineComment(const QString &in_comment) noexcept { inline_comment_ = in_comment; }
		QString getBlockComment() const noexcept { return block_comment_; }
		void setBlockComment(const QString &in_comment) noexcept { block_comment_ = in_comment; }
		void setKeyValProperties(const QRegularExpressionMatch &rexmatch);
		void setKeyValWhitespaces(const std::vector<QString> &vec_ws) { whitespaces_ = vec_ws; }
		std::vector<QString> getKeyValWhiteSpaces() const noexcept { return whitespaces_; }
		void setMandatory(const bool &is_mandatory) noexcept { is_mandatory_ = is_mandatory; }
		bool isMandatory() const noexcept { return is_mandatory_; }
		void setIsUnknownToApp() noexcept { is_unknown_ = true; }
		bool isUnknownToApp() const noexcept { return is_unknown_; }
		void print(QTextStream &out_ss);
		void clear() noexcept { key_ = value_ = inline_comment_ = block_comment_ = QString(); }

	private:
		QString key_;
		QString value_;
		QString inline_comment_;
		QString block_comment_;
		std::vector<QString> whitespaces_; //user's whitespaces around keys and values
		bool is_mandatory_ = false; //injected when parsing XML
		bool is_unknown_ = false; //does the current GUI know this key?
};

class Section {
	public:
		Section();
		KeyValue * operator[] (const QString &str_key); //access by key
		KeyValue * operator[] (const size_t &index); //access by index in order of insertion
		bool operator< (const Section &other) const; //to function as a map key
		QString getName() const noexcept { return name_; }
		void setName(const QString &in_name) noexcept { name_ = in_name; }
		QString getInlineComment() const noexcept { return inline_comment_; }
		void setInlineComment(const QString &in_comment) noexcept { inline_comment_ = in_comment; }
		QString getBlockComment() const noexcept { return block_comment_; } //block comment preceeding the section
		void setBlockComment(const QString &in_comment) noexcept { block_comment_ = in_comment; }
		void setSectionProperties(const QRegularExpressionMatch &rexmatch);
		void setKeyValWhitespaces(const std::vector<QString> &vec_ws) { whitespaces_ = vec_ws; }
		std::vector<QString> getKeyValWhiteSpaces() const noexcept { return whitespaces_; }
		bool hasKeyValue(const QString &str_key) const;
		KeyValue * getKeyValue(const QString &str_key);
		KeyValue * addKeyValue(const KeyValue &keyval);
		bool removeKey(const QString &key);
		void print(QTextStream &out_ss);
		void printKeyValues(QTextStream &out_ss, const bool &alphabetical = false);
		void clear() noexcept { name_ = inline_comment_ = block_comment_ = QString(); }
		size_t size() const noexcept { return key_values_.size(); }
		std::map<QString, KeyValue, CaseInsensitiveCompare> getKeyValueList() const noexcept { return key_values_; }
		void defaultNameSet() noexcept { default_name_set_ = true; } //default name was used for this section
		void sectionIsInIni() noexcept { present_in_ini_ = true; }
		bool isSectionInIni() const noexcept { return present_in_ini_; }

	private:
		QString name_;
		QString inline_comment_;
		QString block_comment_;
		std::vector<QString> whitespaces_;
		std::map<QString, KeyValue, CaseInsensitiveCompare> key_values_;
		std::vector<QString> ordered_key_values_;
		bool default_name_set_ = false; //true if no name was found in the INI file
		bool present_in_ini_ = false; //true if the section comes from an INI file
};

class SectionList : private std::list<Section> { //inherits from list to propagate STL functionality
	public:
		using iterator = std::list<Section>::iterator; //propagate section list iterators
		using const_iterator = std::list<Section>::const_iterator; //(for range loops)
		Section * operator[] (const QString &str_section) { return getSection(str_section); }
		iterator begin() noexcept { return section_list_.begin(); }
		iterator end() noexcept { return section_list_.end(); }
		size_t size() const noexcept { return section_list_.size(); }
		bool hasSection(const QString &section_name) const;
		Section * getSection(const QString &str_section);
		Section * addSection(const Section &section);
		std::list<Section> getSectionsList() const noexcept { return section_list_; }
		bool removeSection(const QString &str_section);
		void clear() noexcept;
		void sort() noexcept { section_list_.sort(); }

	private:
		std::list<Section> section_list_;
		std::set<QString, CaseInsensitiveCompare> ordered_section_set_; //find by string, easy sorting
};

class INIParser {
	Q_DECLARE_TR_FUNCTIONS(INIParser) //make shortcut tr(...) available

	public:
		INIParser() = default; //careful: caller must set logger afterwards!
		INIParser(Logger *in_logger) : logger_instance_(in_logger) {}
		INIParser(const QString &in_file, Logger *in_logger) :
		    logger_instance_(in_logger) { parseFile(in_file); }
		bool operator==(const INIParser &other);
		bool operator!=(const INIParser &other){ return !(*this == other); }
		void setLogger(Logger *in_logger) { logger_instance_ = in_logger; }
		QString getFilename() const noexcept { return filename_; }
		void setFilename(const QString &file) noexcept { filename_ = file; } //e. g. for "Save INI as..."
		bool parseFile(const QString &filename, const bool &fresh = true);
		bool parseText(QString text, const bool &fresh = true);
		QString get(const QString &str_section, const QString &str_key);
		bool set(QString str_section_in, const QString &str_key, const QString &str_value = QString(),
		    const bool is_mandatory = false);
		bool getSectionComment(const QString &str_section, QString &out_inline_comment, QString &out_block_comment);
		bool setSectionComment(const QString &str_section, const QString &inline_comment = QString(),
		    const QString &block_comment = QString());
		void setBlockCommentAtEnd(const QString &end_comment) { block_comment_at_end_ = end_comment; }
		QString getBlockCommentAtEnd() const noexcept { return block_comment_at_end_; }
		bool removeSection(const QString &str_section) { return sections_.removeSection(str_section); }
		SectionList getSectionsCopy() const noexcept { return sections_; } //to build the GUI
		SectionList * getSections() noexcept { return &sections_; } //to change whole sections from outside
		bool hasKeyValue(const QString &str_key) const;
		size_t getNrOfSections() const noexcept { return sections_.size(); }
		void outputIni(QTextStream &out_ss, const bool &alphabetical = false);
		void writeIni(const QString &outfile_name, const bool &alphabetical = false);
		void clear(const bool &keep_unknown_keys = false);
		QString getEqualityCheckMsg() const noexcept { return equality_check_msg_; }

	private:
		bool parseStream(QTextStream &tstream);
		bool evaluateComment(const QString &line, QString &out_comment);
		bool isSection(const QString &line, QString &out_section_name, QRegularExpressionMatch &out_rexmatch);
		bool isKeyValue(const QString &line, QString &out_key_name,
		    QRegularExpressionMatch &out_rexmatch);
		void log(const QString &message, const QString &color = "normal");
		void outputSectionIfKeys(Section &section, QTextStream &out_ss);
		void display_error(const QString &error_msg, const QString &error_info = QString(),
		    const QString &error_details = QString());

		bool first_error_message_ = true; //to prepend INI file info if an error occurs
		Logger *logger_instance_ = nullptr;
		QString filename_ = QString();
		SectionList sections_;
		QString block_comment_at_end_; //a final comment that is not followed by any key or section anymore
		QString equality_check_msg_; //when INIParsers are compared this transports a hint as to what's different
};

#endif //INIPARSER_H
