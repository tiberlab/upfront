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

#include "Checkbox.h"
#include "Label.h"

#include "src/main/colors.h"
#include "src/main/inishell.h"

#include <QHBoxLayout>

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class Checkbox
 * @brief Default constructor for a Checkbox panel.
 * @details A Checkbox is a single on/off switch.
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this Checklist.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Checkbox::Checkbox(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	/* label and checkbox */
	QString caption( options.toElement().attribute("caption") );
	//if a caption is given, and the label is empty, hide the label (nicer XMLs for standard use):
	QString label_string;
	if (!options.toElement().attribute("label").isEmpty())
		label_string = options.toElement().attribute("label");
	if (label_string.isEmpty() && caption.isEmpty())
		label_string = key_;

	auto *key_label( new Label(QString(), QString(), options, no_spacers, label_string, this) );
	if (caption.isEmpty())
		caption = key;
	checkbox_ = new QCheckBox(caption);
	connect(checkbox_, &QCheckBox::stateChanged, this, &Checkbox::checkValue);
	setPrimaryWidget(checkbox_);
	setFontOptions(primary_widget_, options);

	auto *checkbox_layout( new QHBoxLayout );
	if (!key_label->isEmpty()) //this is why Label has a parent
		checkbox_layout->addWidget(key_label, 0, Qt::AlignLeft);
	checkbox_layout->addWidget(checkbox_, 0, Qt::AlignLeft);

	/* spacer and help text */
	if (!no_spacers)
		checkbox_layout->addSpacerItem(buildSpacer()); //keep widgets to the left
	addHelp(checkbox_layout, options, no_spacers);

	//The other panels that can show children can show different groups for different options. Here,
	//we only have one group for checked/unchecked, but we embed them in a dummy container group
	//to get exactly the same margins as for the others.
	margins_group_ = new Group(section, "_checkbox_margins_group_", true);
	container_ = new Group(section, "_checkbox_" + key_);
	margins_group_->setVisible(false);
	margins_group_->addWidget(container_);
	auto *layout( new QVBoxLayout );
	setLayoutMargins(layout);
	layout->addLayout(checkbox_layout);
	layout->addWidget(margins_group_);
	this->setLayout(layout);

	setOptions(options);
}

/**
 * @brief Parse options for a Checkbox from XML.
 * @param[in] options XML node holding the Checkbox.
 */
void Checkbox::setOptions(const QDomNode &options)
{
	QDomNode op( options.firstChildElement("option"));
	if (op.isNull())
		op = options.firstChildElement("o");
	if (!op.isNull())
		recursiveBuild(op, container_, section_);

	if (!op.nextSiblingElement("option").isNull() || !op.nextSiblingElement("o").isNull())
		topLog(tr(R"(XML error: Ignored additional option in Checkbox "%1::%2", there can only be a single one.)").arg(
		    section_, key_), "error");
}

/**
 * @brief Event listener for changed INI values.
 * @details The "ini_value" property is set when parsing default values and potentially again
 * when setting INI keys while parsing a file.
 */
void Checkbox::onPropertySet()
{
	const QString value( this->property("ini_value").toString() );
	const QString value_lc( value.toLower() );
	if (ini_value_ == value)
		return;
	if (value_lc == "true" || value_lc == "t" || value == "1") {
		checkbox_->setCheckState(Qt::Checked);
	} else if (value_lc == "false" || value_lc == "f" || value == "0") {
		checkbox_->setCheckState(Qt::Unchecked);
	} else if (value.isEmpty()) { //the panel is being cleared
		checkbox_->setCheckState(Qt::Unchecked);
		ini_value_ = QString();
		return;
	} else {
		topLog(tr(R"(Ignored non-boolean value "%1" for checkbox "%2::%3")").arg(
		    value, section_, key_), "warning");
		return;
	}

	/* keep desired output format (1/0 or true/false or TRUE/FALSE) */
	if (value == "1" || value == "0")
		numeric_ini_value_ = true;
	else //so that the INI can overpower the XML format
		numeric_ini_value_ = false;
	lowercase_ini_value_ = (value_lc == value);
	short_ini_value_ = (value.length() == 1);

	checkValue(checkbox_->checkState());
}

/**
 * @brief Handle changes to the INI value through this panel.
 * @details This function is called as an event listener by the primary widget when the user
 * interacts with it and also when the property watcher reports changes, i. e. at all times
 * the INI value is requested to change. It is the final layer before handling the value to
 * the INIParser and can therefore perform checks and formatting.
 * @param[in] state The checkbox state (checked, unchecked, or tristate)
 */
void Checkbox::checkValue(const int &state)
{
	QString user_state; //user formatted INI value from checkbox state
	if (state == Qt::Checked) {
		margins_group_->setVisible(!container_->isEmpty());
		user_state = numeric_ini_value_? "1" : (short_ini_value_? "T" : "TRUE");
	} else {
		margins_group_->setVisible(false);
		user_state = numeric_ini_value_? "0" : (short_ini_value_? "F" : "FALSE");
	}
	if (lowercase_ini_value_)
		user_state = user_state.toLower();

	setDefaultPanelStyles(user_state);
	setIniValue(user_state);
}
