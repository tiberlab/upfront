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

#include "settings.h"
#include "src/main/constants.h"
#include "src/main/Error.h"
#include "src/main/inishell.h"
#include "src/main/XMLReader.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>

QDomDocument global_xml_settings = QDomDocument( ); //the settings are always in scope

/*
 * To add a new user setting you only need to incorporate it into the settings_dialog.xml.
 * If you want to give a default value, create the settings node in inishell_settings_minimal.xml
 * as well. New settings that are not displayed to the user you can just start using right away.
 */

/**
 * @brief Check if a valid settings file is available, and if not, create it.
 * @param xml_settings
 */
void checkSettings()
{
	/* if not available, get a settings file from the resources and set the settings XML to it */
	if (global_xml_settings.firstChildElement("inishell_settings").isNull()) {
		QFile resettings(":inishell_settings_minimal.xml");
		resettings.open(QIODevice::ReadOnly | QIODevice::Text);
		QByteArray settings_minimal(resettings.readAll());
		global_xml_settings.setContent(settings_minimal); //will be saved on program end
		resettings.close();
	}
}

/**
 * @brief Save the current settings to the file system.
 * @param[in] xml_settings The temporary XML document that is being manipulated by the program.
 */
void saveSettings()
{
	const QString settings_file(getMainWindow()->getXmlSettingsFilename());
	QDir settings_dir;
	settings_dir.mkpath(QFileInfo( settings_file ).path()); //create settings location if non-existent

	QFile outfile(settings_file);
	if(!outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		Error(QApplication::tr("Could not open settings file for writing"), QString(),
		    QDir::toNativeSeparators(settings_file) + ":\n" + outfile.errorString());
		return;
	}

	QTextStream out_ss(&outfile);
	out_ss << global_xml_settings.toString();
	outfile.close();
}

/**
 * @brief Get full path of settings file.
 * @return System-dependent path to store a settings file at.
 * @details The main program remembers the path once it has been chosen (it could be overridden
 * by a command line option). As soon as it's available use MainWindow::getXmlSettingsFilename().
 */
QString getSettingsFileName()
{
	return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
	    "/" + Cst::settings_file_name;
}

/**
 * @brief Read a single setting from the settings XML.
 * @param[in] setting_name The setting to read in the format "section::subsection::...::key"
 * @param[in] attribute XML attribute to read from the settings key. If not set, the settings
 * key's text node value is returned.
 * @return The setting's value or a Null-string if not available.
 */
QString getSetting(const QString &setting_name, const QString &attribute)
{
	QStringList setting = setting_name.split("::");
	QDomNode s_node(global_xml_settings.firstChildElement());
	for (auto &part : setting)
		s_node = s_node.firstChildElement(part);
	if (attribute.isNull()) //<setting>value</setting>
		return s_node.toElement().text();
	else //<setting attribute="value"/>
		return s_node.toElement().attribute(attribute);
}

/**
 * @brief Get a list of values stored in child nodes of a parent setting's node.
 * @details For example, this could read all folders from a node that looks like this:
 * <xmlpaths>
 *     <path>my_folder_1</path>
 *     <path>my_folder_2</path>
 *     ...
 * </xmlpaths>
 * @param parent_setting The outer settings name (here: "xmlpaths").
 * @param node_name The inner settings name (here: "path").
 * @return List of the stored values (here: {"my_folder_1", "my_folder_2", ...}"
 */
QStringList getListSetting(const QString &parent_setting, const QString &node_name)
{
	QStringList setting = parent_setting.split("::");
	QDomNode parent_node(global_xml_settings.firstChildElement());
	for (int ii = 0; ii < setting.size(); ++ii)
		parent_node = parent_node.firstChildElement(setting[ii]);

	QStringList value_list;
	for (auto node = parent_node.firstChildElement(node_name); !node.isNull();
	    node = node.nextSiblingElement(node_name)) {
		value_list.push_back(node.text());
	} //endfor node
	return value_list;
}

/**
 * @brief Set a list of values as child nodes in a parent setting's node.
 * @details This is the opposite of getListSetting(), removing all items of the parent
 * setting node, and then adding the given list.
 * @param parent_setting The parent setting's name.
 * @param node_name The child setting's name.
 * @param item_list The list of values to set.
 */
void setListSetting(const QString &parent_setting, const QString &node_name, const QStringList &item_list)
{
	QStringList setting = parent_setting.split("::");
	QDomNode parent_node(global_xml_settings.firstChildElement());
	for (int ii = 0; ii < setting.size(); ++ii)
		parent_node = parent_node.firstChildElement(setting[ii]);
	while (parent_node.hasChildNodes()) //clear the node
		parent_node.removeChild(parent_node.firstChild());

	for (auto &item : item_list) {
		QDomNode new_node = parent_node.appendChild(global_xml_settings.createElement(node_name));
		new_node.appendChild(global_xml_settings.createTextNode(item));
	}
}

/**
 * @brief Set a single setting in the XML settings.
 * @param[in] setting_name The setting to read in the format "section::subsection::...::key"
 * @param[in] attribute XML attribute to set for the settings key. If not specified, the settings
 * key's text node value is altered.
 * @param[in] value Value to set.
 */
void setSetting(const QString &setting_name, const QString &attribute, const QString &value)
{
	QStringList setting = setting_name.split("::");
	QDomNode s_node(global_xml_settings.firstChildElement());
	for (auto &part : setting) { //look for the setting's node, creating the parents if necessary
		auto check_node = s_node.firstChildElement(part);
		if (check_node.isNull())
			s_node = s_node.appendChild(global_xml_settings.createElement(part));
		else
			s_node = check_node;
	}
	if (attribute.isNull()) //<setting>value</setting>
		s_node.setNodeValue(value);
	else //<setting attribute="value"/>
		s_node.toElement().setAttribute(attribute, value);
}

/**
 * @brief Helper function to make a list of settings INIshell supports.
 * @details Dynamic settings like those that are set automatically (e. g. window sizes) do not
 * have to be present in the settings file template; same with settings that will be defaulted
 * elsewhere. For some however, having a value assigned in the minimal settings resource file
 * is already the mechanism to set a default value. Hence, those must be in the template to gain
 * a meaningful state. Their names are parsed here from the file. This is used to access all
 * available settings panels from outside when the settings are being displayed (to display
 * and save them).
 * @return List of simple settings INIshell supports.
 */
QStringList getSimpleSettingsNames()
{
	QString settings_error;
	XMLReader settings_parser(":inishell_settings_minimal.xml", settings_error, true);
#ifdef DEBUG
	if (!settings_error.isEmpty())
		qDebug() << "There are errors in the internal settings file:" << settings_error;
#endif //def DEBUG

	QStringList settings_list;
	//only interested in user setable settings:
	QDomElement user_el( settings_parser.getXml().firstChildElement().firstChildElement("user") );
	for (QDomElement sec( user_el.firstChildElement() ); !sec.isNull();
	    sec = sec.nextSiblingElement()) { //run through sections (inireader, appearance, ...)
		for (QDomElement setting( sec.firstChildElement() ); !setting.isNull(); setting = setting.nextSiblingElement())
			settings_list << (user_el.tagName() + "::" + sec.tagName() + "::" + setting.tagName());
	}
	return settings_list;
}
