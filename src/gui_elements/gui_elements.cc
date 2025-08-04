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

#include "gui_elements.h"
#include "src/main/colors.h"
#include "src/main/inishell.h"
#include "src/gui/Logger.h"

#include <QApplication> //for translations

/**
 * @brief Object factory for the panels.
 * @details Construct an object from a string name (often read from an XML).
 * @param[in] identifier Name of the object.
 * @param[in] section INI section the controlled values belong to.
 * @param[in] key INI key of the controlled value.
 * @param[in] options The current XML node with all options and children.
 * @param[in] no_spacers If available, set a tight layout for the object.
 * @param[in] parent The parent widget.
 * @return An object of the panel family to manipulate values.
 */
QWidget * elementFactory(const QString &in_identifier, const QString &section, const QString &key,
    const QDomNode &options, const bool& no_spacers, QWidget *parent)
{
	const QString identifier( in_identifier.toLower() );
	bool is_helptext_element = false;

	QWidget *element;
	if (options.toElement().attribute("replicate").toLower() == "true") {
		element = new Replicator(section, key, options, no_spacers, parent);
	} else if (identifier == "alternative") {
		element = new Dropdown(section, key, options, no_spacers, parent);
	} else if (identifier == "datetime") {
		element = new Datepicker(section, key, options, no_spacers, parent);
	} else if (identifier == "checklist") {
		element = new Checklist(section, key, options, no_spacers, parent);
	} else if (identifier == "checkbox") {
		element = new Checkbox(section, key, options, no_spacers, parent);
	} else if (identifier == "choice") {
		element = new Choice(section, key, options, no_spacers, parent);
	} else if (identifier == "file" || identifier == "filename" || identifier == "path") {
		element = new FilePath(section, key, options, no_spacers, parent);
	} else if (identifier == "grid") {
		element = new GridPanel(section, key, options, parent);
	} else if (identifier == "helptext") {
		element = new Helptext(options, parent);
		is_helptext_element = true;
	} else if (identifier == "horizontal") {
		element = new HorizontalPanel(section, key, options, no_spacers, parent);
	} else if (identifier == "label") {
		element = new Label(section, key, options, no_spacers, QString(), parent); //no forced caption
	} else if (identifier == "number") {
		element = new Number(section, key, options, no_spacers, parent);
	} else if (identifier == "selector") {
		element = new Selector(section, key, options, no_spacers, parent);
	} else if (identifier == "text") {
		element = new Textfield(section, key, options, no_spacers, parent);
	} else if (identifier == "space" || identifier == "spacer") {
		element = new Spacer(options, parent);
	} else {
		if (identifier.isEmpty())
			topLog(QApplication::tr("XML error: A parameter in the XML file is missing its type."),
			    "error");
		else
			topLog(QApplication::tr(
			    R"(XML error: Unknown parameter object in XML file: "%1" for "%2::%3")").arg(identifier, section, key), "error");
		element = nullptr; //will throw a Qt warning
	}

	if (element != nullptr && !is_helptext_element) {
		const bool is_mandatory = (options.toElement().attribute("optional") == "false");
		if (is_mandatory)
			element->setProperty("is_mandatory", "true");
		QString default_value( options.toElement().attribute("default") );
		const QString default_from_constructor( element->property("default_value").toString() );

		if (!default_from_constructor.isEmpty()) {
			if (!default_value.isNull()) //default values were already found in the constructor
				topLog(QString(QApplication::tr(
				    R"(XML error: Additional default value "%1" ignored because defaults were already set in options for key "%2".)")).arg(
				    default_value, key), "error");
			default_value = default_from_constructor;
		}
		if (!default_value.isNull()) {
			element->setProperty("default_value", default_value); //should call setDefaultPanelStyles()
			element->setProperty("ini_value", default_value);
		} else if (is_mandatory) {
			auto *atomic_element = static_cast<Atomic *>(element);
			atomic_element->setDefaultPanelStyles(QString());
		}
		//TODO: an unavailable default Dropdown value in the XML may still be styled as default
		//(but will produce a warning). Reason is that from here we don't know the element doesn't exist.
	}
	return element;
}
