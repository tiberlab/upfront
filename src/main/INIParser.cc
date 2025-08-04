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

#include "INIParser.h"
#include "src/main/colors.h"
#include "src/main/Error.h"
#include "src/main/inishell.h"
#include "src/main/settings.h"

#include <QDir>
#include <QFile>

#include <array>
#include <iostream> //for logging to cerr
#include <utility> //for std::move semantics

#ifdef DEBUG
	#include <QDebug>
#endif

////////////////////////////////////////
///          KEYVALUE class          ///
////////////////////////////////////////

/**
 * @class KeyValue
 * @brief Default constructor for a key/value pair.
 * @details A KeyValue holds one entry of an INI file, i. e. the key, value and comment,
 * as well as surrounding whitespaces. The key is case insensitive.
 */
KeyValue::KeyValue() : KeyValue(QString(), QString())
{
	//delegate the constructor
}

/**
 * @brief Constructor for a key/value pair with a given key and value.
 * @param[in] key The key to set.
 * @param[in] value The value to set.
 */
KeyValue::KeyValue(QString key, QString value) : key_(std::move(key)), value_(std::move(value))
{
	static constexpr int nr_of_whitespace_fields_keyval = 4; //(1)key(2)=(3)value(4)#comment
	whitespaces_.reserve(nr_of_whitespace_fields_keyval);
	whitespaces_.emplace_back(""); //default: no whitespace at beginning of line
	for (int ii = 1; ii < nr_of_whitespace_fields_keyval; ++ii)
		whitespaces_.emplace_back(" "); //default inbetween whitespace
}

/**
 * @brief Assign attributes to a KeyValue.
 * @details The key name is set beforehand, here we set the value and secondary properties, i. e.
 * the inline comment and surrounding whitespaces. Called when parsing an INI file.
 * @param[in] rexmatch The already parsed regular expression holding all properties, i. e. the full
 * INI line.
 */
void KeyValue::setKeyValProperties(const QRegularExpressionMatch &rexmatch)
{
	static constexpr size_t idx_value = 5; //indices of the respective capturing groups
	static constexpr size_t idx_comment = 7;
	static constexpr int nr_of_whitespace_fields_keyval = 4; //(1)key(2)=(3)value(4)#comment
	static constexpr std::array<size_t, nr_of_whitespace_fields_keyval> indices_whitespaces( {1, 3, 4, 6} );

	this->setValue(rexmatch.captured(idx_value));
	this->setInlineComment(rexmatch.captured(idx_comment));
	if (getSetting("user::inireader::whitespaces", "value") == "USER") {
		for (size_t ii = 0; ii < nr_of_whitespace_fields_keyval; ++ii)
			whitespaces_.at(ii) = rexmatch.captured(static_cast<int>(indices_whitespaces.at(ii)));
	}
}

/**
 * @brief Print the key/value pair to a text stream.
 * @param[in,out] out_ss The stream to print to.
 */
void KeyValue::print(QTextStream &out_ss)
{
	out_ss << getBlockComment();
	out_ss << whitespaces_.at(0) << getKey() << whitespaces_.at(1) << "=" <<
	    whitespaces_.at(2) << getValue();
	if (!getInlineComment().isEmpty())
		out_ss << whitespaces_.at(3) << getInlineComment();
	out_ss << "\n";
}

////////////////////////////////////////
///           SECTION class          ///
////////////////////////////////////////

/**
 * @class Section
 * @brief Default constructor for a Section.
 * @details A Section holds one section of an INI file. This includes the section's properties, i. e.
 * the name, inline comment and surrounding whitespaces, as well as a list of all KeyValues
 * contained in the section. The section name is case insensitive.
 * KeyValues are stored in a map with their INI key names as container key, so the map works with
 * a copy of KeyValues.getName() as the map key for comfortable access and checking of existence.
 * Furthermore, when new KeyValues are inserted their map key is stored in order of insertion
 * in a vector so that the map can be iterated through unsorted, i. e. how it was read from the INI file.
 */
Section::Section()
{
	static constexpr int nr_of_whitespace_fields_section = 2; //(1)[SECTION](2)#comment
	whitespaces_.reserve(nr_of_whitespace_fields_section);
	whitespaces_.emplace_back(""); //default whitespaces at beginning of line
	whitespaces_.emplace_back(" "); //default whitespaces before comment
}

/**
 * @brief Convenience call to retrieve a KeyValue from a section.
 * @details This function accesses KeyValues by their key names.
 * @param[in] str_key The key name to find.
 * @return A reference to the found KeyValue, or nullptr if non-existent.
 */
KeyValue * Section::operator[] (const QString &str_key)
{
	return getKeyValue(str_key);
}

/**
 * @brief Retrieve a KeyValue by order of insertion.
 * @details This function allows access of the KeyValues in order of insertion,
 * for example to reproduce the ordering of an input INI file.
 * @param[in] index The KeyValue's number of insertion.
 * @return A reference to the found KeyValue, or nullptr if non-existent.
 */
KeyValue * Section::operator[] (const size_t &index)
{
	const QString key( ordered_key_values_.at(index) );
	return &key_values_[key];
}

/**
 * @brief Lesser-operator to have Sections function as map keys.
 * @param[in] other The Section to compare to.
 * @return True if this section's name is lexicographically smaller than the other's.
 */
bool Section::operator<(const Section &other) const
{
	return QString::compare(this->getName(), other.getName(), Qt::CaseInsensitive) < 0;
}

/**
 * @brief Assign attributes to a Section.
 * @details Set the Section's properties, i. e. name, comment, and whitespaces.
 * Called when parsing an INI file.
 * @param[in] rexmatch The already parsed regular expression holding all properties, i. e. the full
 * INI line.
 */
void Section::setSectionProperties(const QRegularExpressionMatch &rexmatch)
{
	static constexpr size_t idx_name = 2; //indices of respective capturing groups
	static constexpr size_t idx_comment = 4;
	static constexpr int nr_of_whitespace_fields_section = 2;
	static constexpr std::array<size_t, nr_of_whitespace_fields_section> indices_whitespaces( {1, 3} );

	this->setName(rexmatch.captured(idx_name));
	this->setInlineComment(rexmatch.captured(idx_comment));
	//if getMainWindow is NULL then we are in command line mode - no user settings available
	if (getMainWindow() == nullptr || getSetting("user::inireader::whitespaces", "value") == "USER") {
		for (size_t ii = 0; ii < nr_of_whitespace_fields_section; ++ii)
			whitespaces_.at(ii) = rexmatch.captured(static_cast<int>(indices_whitespaces.at(ii)));
	}
}

/**
 * @brief Check if the Section contains a certain KeyValue.
 * @param[in] str_key The INI key to check for.
 * @return True if the INI key was found in this Section.
 */
bool Section::hasKeyValue(const QString &str_key) const
{
	return key_values_.count(str_key) > 0;
}

/**
 * @brief Retrieve a KeyValue from a Section.
 * @param[in] str_key The INI key to look for.
 * @return A reference to the found KeyValue, or nullptr if non-existent.
 */
KeyValue * Section::getKeyValue(const QString &str_key)
{
	if (key_values_.find(str_key) == key_values_.end())
		return nullptr;
	return &key_values_[str_key];
}

/**
 * @brief Add a key/value pair to the Section.
 * @details This function checks if an INI key already exists in the Section, and if so, returns it.
 * If not, the (new) KeyValue is added to the Section for further use. Properties are set from outside
 * after this check; attributes such as whitespaces are not propagated here.
 * @param[in] keyval An already constructed KeyValue.
 * @return A reference to either the found or the newly inserted KeyValue.
 */
KeyValue * Section::addKeyValue(const KeyValue &keyval)
{
	std::pair<std::map<QString, KeyValue>::iterator, bool> result;
	result = key_values_.insert(std::make_pair(keyval.getKey(), keyval));
	if (result.second) //a new item was inserted
		ordered_key_values_.push_back(keyval.getKey()); //store key in order of insertion
	return &result.first->second;
}

/**
 * @brief Remove a key from this INI section.
 * @param[in] key The key to remove.
 * @return False if the key was not found.
 */
bool Section::removeKey(const QString &key)
{
	auto it(key_values_.find(key));
	if (it == key_values_.end()) {
		return false;
	} else {
		key_values_.erase(it);
		ordered_key_values_.erase(
		    std::find(ordered_key_values_.begin(), ordered_key_values_.end(), key));
		return true;
	}
}

/**
 * @brief Print the section header to a text stream.
 * @param[in,out] out_ss The stream to print to.
 */
void Section::print(QTextStream &out_ss)
{
	if (default_name_set_) //user did not provide the (default) section --> do not output it
		return;
	if (!this->getBlockComment().isEmpty())
		out_ss << this->getBlockComment();
	out_ss << whitespaces_.at(0) << Cst::section_open << this->getName() << Cst::section_close <<
	    (this->getInlineComment().isEmpty()? "" : whitespaces_.at(1) + this->getInlineComment()) << "\n";
}

/**
 * @brief Print all of this Section's KeyValues to a text stream.
 * @details This can either be done in container sorting order (alphabetical), or in order of
 * insertion (tracked in a separate map).
 * @param[in,out] out_ss The stream to print to.
 * @param[in] alphabetical If true, INI keys are writtin in alphabetical order.
 */
void Section::printKeyValues(QTextStream &out_ss, const bool &alphabetical)
{
	if (alphabetical) { //range based loop with implicit container sorting
		for (auto &keyval : key_values_) {
			if (!keyval.second.getValue().isEmpty())
				keyval.second.print(out_ss);
		}
	} else { //for loop through a vector that stores map keys in order of insertion
		for (size_t ii = 0; ii < ordered_key_values_.size(); ++ii) {
			if (!key_values_[ordered_key_values_[ii]].getValue().isEmpty())
				key_values_[ordered_key_values_[ii]].print(out_ss);
		}
	} //end if alphabetical
}

////////////////////////////////////////
///         SECTIONLIST class        ///
////////////////////////////////////////

/**
 * @class SectionList
 * @brief A SectionList collects sections and therefore composes a complete INI file, save for
 * a trailing block comment.
 * @details The SectionList inherits from std::list to enable range loops on its main container,
 * a std::list of Sections. In a secondary container, an std::set, the section names are collected
 * in order of insertion to be able to reproduce user INI files and for easy lookup.
 */

/**
 * @brief Check if a section name already exists in the collection of Sections.
 * @param[in] section_name The INI file's name for the section.
 * @return True if the section exists.
 */
bool SectionList::hasSection(const QString &section_name) const
{
	return (ordered_section_set_.find(section_name) != ordered_section_set_.end());
}

/**
 * @brief Retrieve a Section by its name.
 * @param[in] str_section The INI file's name for the section.
 * @return A reference to the found Section, or nullptr if it does not exist.
 */
Section * SectionList::getSection(const QString &str_section)
{ //Look for the section by name, not by equality (different comments are still the same section)
	for (auto &sec : section_list_) { //only called once per section in parse()
		if (QString::compare(sec.getName(), str_section, Qt::CaseInsensitive) == 0)
			return &sec;
	}
	return nullptr;
}

/**
 * @brief Add a Section to the list of sections.
 * @details This function checks if a Section already exists and if so returns it.
 * If not, the (new) Section is added for further use. Properties are set from outside
 * after this check; attributes such as whitespaces are not propagated here.
 * @param[in] section An already constructed Section.
 * @return A reference to either the found or the newly inserted Section.
 */
Section * SectionList::addSection(const Section &section)
{
	Section *out_section( getSection(section.getName()) );
	if (out_section == nullptr) {
		section_list_.push_back(section);
		out_section = &section_list_.back();
		ordered_section_set_.insert(section.getName());
	}
	return out_section;
}

/**
 * @brief Remove a Section from the collection.
 * @param[in] str_section The INI file's name for the section.
 * @return True if successful, false if the Section did not exist.
 */
bool SectionList::removeSection(const QString &str_section)
{
	//find Section in the list:
	const auto lit = std::find_if(section_list_.begin(), section_list_.end(),
	    [str_section](Section const& section) {return (QString::compare(section.getName(), str_section, Qt::CaseInsensitive) == 0);});
	if (lit != section_list_.end()) {
		//find section in the ordered list:
		for (auto &sec : ordered_section_set_) {
			if (QString::compare(sec, str_section, Qt::CaseInsensitive) == 0) {
				ordered_section_set_.erase(sec);
				break;
			}
		}
		section_list_.erase(lit);
		return true;
	}
	return false; //section did not exist
}

/**
 * @brief Clear both internal containers for the list of sections.
 */
void SectionList::clear() noexcept
{
	section_list_.clear();
	ordered_section_set_.clear();
}

////////////////////////////////////////
///            INIPARSER             ///
////////////////////////////////////////

/**
 * @class INIParser
 * @brief The top level interface to read and store INI sections and key/value pairs.
 * @details The INIParser handles everything to do with reading key/value pairs and sections
 * from an INI file on the file system, manipulating them, and writing the result back out.
 */

/**
 * @brief The equality operator checks sections and keys.
 * @details Each section name and key/value-pair is compared, comments and whitespaces do not
 * matter.
 * @param[in] other The INIParser to compare against.
 * @return True if both INIparsers are the same.
 */
bool INIParser::operator==(const INIParser &other)
{
	equality_check_msg_.clear(); //store why the assertion is false
	auto other_sections( other.getSectionsCopy() );
	if (other_sections.size() != sections_.size()) {
		if (filename_.isEmpty()) {
			equality_check_msg_ = "An application has been opened, but it's values have not been saved yet.\n";
			return false;
		}
		equality_check_msg_ = tr("Different number of sections (%1 vs. %2).\nThis usually implies a different number of keys.\n\n").arg(
		    sections_.size()).arg(other_sections.size());
		QString new_in_this, new_in_other;
		for (auto &sec : sections_) {
			if (!other_sections.hasSection(sec.getName()))
				new_in_other += sec.getName() + ", ";
		}
		for (auto &sec : other_sections) {
			if (!sections_.hasSection(sec.getName()))
				new_in_this += sec.getName() + ", ";
		}
		new_in_this.chop(2);
		new_in_other.chop(2);
		if (!new_in_this.isEmpty())
			equality_check_msg_ += "Sections not in " + filename_ + ": " + new_in_this + "\n" +
			    "(The loaded application may have inserted missing mandatory keys.)\n";
		if (!new_in_other.isEmpty())
			equality_check_msg_ += "Sections present in original but not in the new file: " + new_in_other + "\n";
		return false;
	}

	for (auto &sec : sections_) { //TODO: more info in these messages
		const auto other_sec( other_sections[sec.getName()] );
		if (other_sec == nullptr) {
			equality_check_msg_ = tr(R"(Section "%1" not found)").arg(sec.getName());
			return false;
		}
		const auto key_values( sec.getKeyValueList() );
		auto other_key_values( other_sec->getKeyValueList() );
		if (key_values.size() != other_key_values.size()) {
			equality_check_msg_ = tr(R"(Different number of key/value pairs (%1 vs. %2))").arg(
			    key_values.size()).arg(other_key_values.size());
			return false;
		}
		for (auto &keyval : key_values) {
			if (other_key_values.count(keyval.first) == 0) {
				equality_check_msg_ = tr(R"(Key "%1" not found)").arg(keyval.first);
				return false;
			}
			const auto other_keyval( other_key_values[keyval.first] );
			const bool both_empty = (keyval.second.getValue().isEmpty() && other_keyval.getValue().isEmpty());
			if (both_empty) //e. g. one is not present (Null) and the other one reported empty
				continue;
			if (QString::compare(keyval.second.getValue(), other_keyval.getValue(), Qt::CaseInsensitive) != 0) {
				equality_check_msg_ = tr(R"((One of) the different key(s) is: "%1")").arg(keyval.first);
				return false;
			}
			//note that there is no numeric check against different precisions here (i. e. 1.0 != 1),
			//this must be handled by the Number panel
		}
	}
	return true;
}

/**
 * @brief Parse an INI file and store everything in container classes.
 * @details This opens and parses an INI file from the file system.
 * @param[in] File name to parse.
 * @param[in] fresh Delete existing sections and start afresh.
 * @return True if the parsing was successful.
 */
bool INIParser::parseFile(const QString &filename, const bool &fresh)
{
	if (fresh)
		this->clear();
	filename_ = filename; //remember last file parsed
	first_error_message_ = true;
	/* open the file */
	QFile infile(filename_);
	if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		display_error(tr("Could not open INI file for reading"), QString(),
		    QDir::toNativeSeparators(filename_) + ":\n" + infile.errorString());
		return false;
	}
	QTextStream tstream(&infile);
	const bool success = parseStream(tstream);
	infile.close();
	return success;
}

/**
 * @brief Parse INI contents from a string.
 * @details This is used for example in the preview window.
 * @param[in] text Text to parse.
 * @param[in] fresh Delete existing sections and start afresh.
 * @return True if successful.
 */
bool INIParser::parseText(QString text, const bool &fresh)
{
	if (fresh)
		this->clear();
	filename_ = "./preview_ini.ini";
	QTextStream tstream(&text);
	return parseStream(tstream);
}

/**
 * @brief Retrieve a single INI key's value.
 * @param[in] str_section Section to search for the key/value.
 * @param[in] str_key INI key to find.
 * @return The INI value, or a Null-String if not found.
 */
QString INIParser::get(const QString &str_section, const QString &str_key)
{
	Section *sec( sections_[str_section] );
	if (sec == nullptr)
		return QString();
	const KeyValue *keyval( sec->getKeyValue(str_key) );
	if (keyval == nullptr)
		return QString();
	return keyval->getValue();
}

/**
 * @brief Set a key's value, creating the KeyValue if necessary.
 * @param[in] str_section Section name the key/value belongs to.
 * @param[in] str_key The key to insert or modify.
 * @param[in] str_value The key's value to set, or empty if only comments are changed.
 * @return True if the value was set, false if nothing was changed.
 */
bool INIParser::set(QString str_section_in, const QString &str_key, const QString &str_value,
    const bool is_mandatory)
{
	/* find or create the Section and KeyValue */
	if (str_section_in.isNull())
		str_section_in = Cst::default_section;
	bool changed = false;
	Section *sec( sections_[str_section_in] );
	if (sec == nullptr) {
		Section new_section;
		new_section.setName(str_section_in);
		sec = sections_.addSection(new_section);
		changed = true;
	}
	KeyValue *keyval( sec->getKeyValue(str_key) );
	if (keyval == nullptr) {
		KeyValue new_keyval(str_key);
		keyval = sec->addKeyValue(new_keyval);
		changed = true;
	}
	keyval->setValue(str_value);
	keyval->setMandatory(is_mandatory); //injected when parsing XML
	return changed;
}

/**
 * @brief Check if a certain INI key is present.
 * @param[in] str_key The key to look for.
 * @return True if found.
 */
bool INIParser::hasKeyValue(const QString &str_key) const
{
	for (auto &sec : sections_.getSectionsList()) {
		if (sec.hasKeyValue(str_key))
			return true;
	}
	return false;
}

/**
 * @brief Retrieve a section's inline and block comments.
 * @param[in] str_section The section name.
 * @param[out] out_inline_comment The returned inline comment.
 * @param[out] out_block_comment The returned block comment.
 * @return True if the Section exists, false if not.
 */
bool INIParser::getSectionComment(const QString &str_section, QString &out_inline_comment,
    QString &out_block_comment)
{
	const Section *sec( sections_.getSection(str_section) );
	if (sec != nullptr) {
		out_inline_comment = sec->getInlineComment();
		out_block_comment = sec->getBlockComment();
		return true;
	}
	return false; //section does not exist
}

/**
 * @brief Set a section's inline and block comments.
 * @param[in] str_section The section name.
 * @param[in] inline_comment The inline comment to set.
 * @param[in] block_comment The block comment to set.
 * @return
 */
bool INIParser::setSectionComment(const QString &str_section, const QString &inline_comment,
    const QString &block_comment)
{
	Section *sec( sections_.getSection(str_section) );
	if (sec == nullptr)
		return false; //section does not exist
	if (!inline_comment.isNull())
		sec->setInlineComment(inline_comment);
	if (!block_comment.isNull())
		sec->setBlockComment(block_comment);
	return true; //also true if no comment was given, means "section exists"
}

/**
 * @brief Print the INIParser's contents to a text stream.
 * @param[in,out] out_ss The stream to write to.
 * @param[in] alphabetical Sort sections and keys in order of insertion or alphabetically?
 */
void INIParser::outputIni(QTextStream &out_ss, const bool &alphabetical)
{
	if (alphabetical) {
		SectionList sections_copy(sections_);
		sections_copy.sort(); //leave the original and sort a copy
		for (auto &sec : sections_copy)
			outputSectionIfKeys(sec, out_ss);
	} else { //order as inserted
		for (auto &sec : sections_)
			outputSectionIfKeys(sec, out_ss);
	}
	out_ss << block_comment_at_end_;
}

/**
 * @brief Write the INIParser's contents to the file system.
 * @param[in] outfile_name File name to output to.
 * @param[in] alphabetical Sort sections and keys in order of insertion or alphabetically?
 */
void INIParser::writeIni(const QString &outfile_name, const bool &alphabetical)
{
	QFile outfile(outfile_name);
	if (!outfile.open(QIODevice::WriteOnly)) {
		const QString msg( tr("Could not open INI file for writing") );
		display_error(msg, QString(), QDir::toNativeSeparators(outfile_name) + ":\n" + outfile.errorString());
		return;
	}
	QTextStream ss(&outfile);
	outputIni(ss, alphabetical);
	outfile.close();
	
	/*
	 * When creating an INI file from scratch, it is not connected to the file system yet.
	 * So we do this when writing out an INI file here so that the Workflow panel can
	 * access INI keys and so that we are in the same state as if the user had edited this
	 * file with an external program and then loaded it into INIshell.
	 * I. e., we mimick the usual "save as" behaviour.
	 */
	if (getMainWindow()->getIni() != this) getMainWindow()->setIni( *this );
}

/**
 * @brief Clear contents of the INIParser.
 * @param[in] keep_unknown_keys Only clear the keys that are being controlled by the GUI.
 */
void INIParser::clear(const bool &keep_unknown_keys)
{
	if (keep_unknown_keys) { //keep meta info and keys from original INI that are unknown to the GUI
		for (auto &sec : sections_) {
			for (auto &keyval : sec.getKeyValueList()) {
				if (!keyval.second.isUnknownToApp())
					sec.removeKey(keyval.first);
			}
		} //TODO: clear sections that are empty now
	} else { //only the logger is left in place
		sections_.clear();
		filename_ = QString();
		block_comment_at_end_ = QString();
	}
}

/**
 * @brief Internal function to parse INI contents from a stream.
 * @details This function can be called from a file or text and is the core
 * function to parse INI contents into data containers.
 * @param[in] tstream The input text stream to read.
 * @return True if all went well.
 */
bool INIParser::parseStream(QTextStream &tstream)
{
	first_error_message_ = true; //to print error headers only once

	QString current_block_comment;
	Section *current_section = nullptr;
	bool all_ok = true; //all keys were well-formatted

	/* iterate through lines in file */
	size_t linecount = 0;
	while (!tstream.atEnd()) {
		linecount++; //for logging purposes
		const QString line( tstream.readLine() );

		/* check for comment */
		QString local_block_comment; //collect block comment to append to next section
		if (evaluateComment(line, local_block_comment)) {
			current_block_comment += local_block_comment + "\n"; //this includes empty lines
			continue;
		}

		/* check for section */
		QString section_name;
		QRegularExpressionMatch rex_match;
		const bool section_success = isSection(line, section_name, rex_match);
		if (section_success) {
			const bool has_section = sections_.hasSection(section_name);
			if (has_section) {
				current_section = sections_[section_name];
				//TODO: Think about if it's worth the hassle to allow multiple occurrences of the same section
				//TODO: If not, merge the inline comments
				current_block_comment = current_block_comment.prepend(current_section->getBlockComment());
			} else {
				Section new_section;
				new_section.setSectionProperties(rex_match);
				current_section = sections_.addSection(new_section);
			}
			current_section->setBlockComment(current_block_comment);
			current_block_comment.clear(); //clear the block comment once it's been used for a section
			current_section->sectionIsInIni(); //remember to always output, even if empty
			continue;
		} //endif section_success

		/* check for key/value pair */
		QString key_name;
		const bool keyval_success = isKeyValue(line, key_name, rex_match);
		if (keyval_success) {
			if (current_section == nullptr) {
				Section default_section;
				default_section.setName(Cst::default_section);
				default_section.defaultNameSet(); //remember that the section was set from default name
				current_section = sections_.addSection(default_section);
			}
			const bool has_keyval = current_section->hasKeyValue(key_name);
			KeyValue *current_keyval = nullptr;
			if (has_keyval) {
				current_keyval = current_section->getKeyValue(key_name);
			} else {
				KeyValue new_keyval(key_name);
				current_keyval = current_section->addKeyValue(new_keyval);
			}
			current_keyval->setKeyValProperties(rex_match);
			current_keyval->setBlockComment(current_block_comment);
			current_block_comment.clear();
		} else if (!line.trimmed().isEmpty()) { //we allow misplaced whitespace characters
			const QString msg( QString(tr("Undefined format on line %1 of file \"%2\"")).arg(linecount).arg(filename_)
			    + ": " + line );
			log(msg, "warning");
			topStatus(tr("Invalid line in file \"") + filename_ + "\"", "warning");
			all_ok = false;
		}
	} //end while tstream

	//is a comment left at the very end that can not be assigned to a following section or key?
	if (!current_block_comment.isEmpty())
		block_comment_at_end_ = current_block_comment;

	return all_ok;
	//TODO: How to handle keys that are valid multiple times (e. g. IMPORT_BEFORE)?
}

/**
 * @brief Check if an INI line is a comment, and if so, return it.
 * @param[in] line One line of the INI file.
 * @param[out] out_comment The comment including the tag (# or ;)
 * @return True if the line was parsed successfully as a comment.
 */
bool INIParser::evaluateComment(const QString &line, QString &out_comment)
{
	if (line.isEmpty())
		return true; //reproduce empty lines too
	static const QString regex_comment( R"(^\s*[#;].*)" );
	/*                                          |
	 *                                         (1)
	 * (1) Any line that starts with ; or #, optionally prefaced by whitespaces
	 */
	static constexpr size_t idx_comment = 0;

	static const QRegularExpression rex(regex_comment);
	const QRegularExpressionMatch matches = rex.match(line);
	out_comment = matches.captured(idx_comment);
	return matches.hasMatch();
}

/**
 * @brief Check if an INI line is a section header, and if so, return its properties.
 * @param[in] line One line of the INI file.
 * @param[out] out_section_name The parsed name of the section.
 * @param[out] out_rexmatch Further properties (comment, whitespaces) as parsed regex matches.
 * @return True if the line was parsed successfully as a section.
 */
bool INIParser::isSection(const QString &line, QString &out_section_name,
    QRegularExpressionMatch &out_rexmatch)
{
	static const QString regex_section( R"((\s*)\[([\w+]*)\](\s*)([#;].*)*)" );
	/*                                                |              |
	 *                                               (1)            (2)
	 * (1) Alphanumeric string (can't be empty) enclosed by brackets
	 * (2) Comment started with ; or #
	 */
	static constexpr size_t idx_total = 0;
	static constexpr size_t idx_name = 2;
	static const QRegularExpression rex(regex_section);

	out_rexmatch = rex.match(line);
	out_section_name = out_rexmatch.captured(idx_name);
	return (line == out_rexmatch.captured(idx_total)); //full match?
}

/**
 * @brief Check if an INI line is a key/value pair, and if so, return its properties.
 * @param[in] line One line of the INI file.
 * @param[out] out_key_name The parsed name of the key.
 * @param[out] out_rexmatch Further properties (value, comment, whitespaces) as parsed regex matches.
 * @return True if the line was parsed successfully as a key/value pair.
 */
bool INIParser::isKeyValue(const QString &line, QString &out_key_name,
    QRegularExpressionMatch &out_rexmatch)
{
#ifdef DEBUG
	if (line.isEmpty()) {
		qDebug() << "Empty line should not have reached INIParser::isKeyValue(), since it should be added to a comment.";
		return false;
	}
#endif //def DEBUG

	static const QString regex_keyval(
	    R"((\s*)([\w\*\-:_.]*)(\s*)=(\s*)(;$|#$|.+?)(\s*)(#.*|;.*|$))" );
	/*                  |       \    /        \             |
	 *                 (1)       \  /         (3)          (4)
	 *                            (2)
	 * (1) Alphanumeric string for the key (can't be empty)
	 * (3) Any number of whitespacescaround =
	 * (4) Either a single ; or # (e. g. csv delimiter) or any non-empty string for the value
	 * (5) The value is either ended by a comment or the end of the line
	 */
	static constexpr size_t idx_total = 0;
	static constexpr size_t idx_key = 2;

	static const QRegularExpression rex(regex_keyval);
	out_rexmatch = rex.match(line);
	out_key_name = out_rexmatch.captured(idx_key);
	return (line == out_rexmatch.captured(idx_total)); //full match?
}

/**
 * @brief Convenience wrapper for the Logger.
 * @details In command line mode the logger is skipped and the message is written to stderr.
 * @param[in] message The message to log.
 * @param[in] color Color of the log text (ignored in command line mode).
 */
void INIParser::log(const QString &message, const QString &color)
{
	//If all is well, don't log anything. If at least one error occurred, prepend
	//a log message about which file is being read:
	if (logger_instance_ != nullptr) { //GUI mode
		if (first_error_message_) {
			logger_instance_->log(tr(R"(Reading INI file "%1"...)").arg(filename_));
			first_error_message_ = false;
		}
		logger_instance_->log(message, color);
	} else {
		std::cerr << "[W] " << message.toStdString() << std::endl;
	}
}

/**
 * @brief Helper function to output section and skip section header if empty.
 * @param[in] section The section to print.
 * @param[in] out_ss Text stream to print to.
 */
void INIParser::outputSectionIfKeys(Section &section, QTextStream &out_ss)
{
	QString out_ss_keys;
	QTextStream keys_stream(&out_ss_keys);
	section.printKeyValues(keys_stream);
	//TODO: small thing: if an input INI section contains only invalid keys, then it will still be printed.
	//This is because we don't keep track of invalid lines and therefore don't know this.
	if (!out_ss_keys.isEmpty() || section.isSectionInIni()) {
		section.print(out_ss);
		out_ss << out_ss_keys;
	}
}

/**
 * @brief Convenience wrapper for errors.
 * @details In command line mode the GUI error is skipped and the error is written to stderr.
 * @param[in] error_msg The error to display.
 * @param[in] error_info Additional info about the error.
 * @param[in] error_details A detailed description of the error.
 */
void INIParser::display_error(const QString &error_msg, const QString &error_info, const QString &error_details)
{
	if (logger_instance_ != nullptr) //GUI mode
		Error(error_msg, error_info, error_details);
	else
		std::cerr << "[E] " << error_msg.toStdString() << (error_info.isEmpty()? "" : ", ") <<
		    error_info.toStdString() << (error_details.isEmpty()? "" : "; ") <<
		    error_details.toStdString() << std::endl;
}
