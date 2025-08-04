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

#include "Textfield.h"
#include "Label.h"
#include "src/main/expressions.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QToolButton>

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class Textfield
 * @brief Default constructor for a Textfield.
 * @details A Textfield is used to enter plain text.
 * @param[in] key INI key corresponding to the value that is being controlled by this Textfield.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Textfield::Textfield(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	/* label and text box */
	auto *key_label( new Label(QString(), QString(), options, no_spacers, key_, this) );
	textfield_ = new QLineEdit; //single line text box
	setPrimaryWidget(textfield_);
	connect(textfield_, &QLineEdit::textEdited, this, &Textfield::checkValue);

	/* action button */
	check_button_ = new QToolButton; //a button that can pop up if the text has a certain format
	check_button_->setVisible(false);
	check_button_->setAutoRaise(true);
	check_button_->setIcon(getIcon("internet-web-browser")); //only coordinates for now
	connect(check_button_, &QToolButton::clicked, this, &Textfield::checkButtonClicked);

	/* layout of textbox plus button and the main layout */
	auto *field_button_layout( new QHBoxLayout );
	field_button_layout->addWidget(textfield_);
	field_button_layout->addWidget(check_button_);
	auto *textfield_layout( new QHBoxLayout );
	setLayoutMargins(textfield_layout);
	textfield_layout->addWidget(key_label, 0, Qt::AlignLeft);
	textfield_layout->addLayout(field_button_layout);

	/* choose size of text box */
	const QString size( options.toElement().attribute("size") );
	if (size.toLower() == "small")
		textfield_->setMinimumWidth(Cst::tiny);
	else
		textfield_->setMinimumWidth(Cst::width_textbox_medium);
	if (!no_spacers && size.toLower() != "large") //"large": text box extends to window width
		textfield_layout->addSpacerItem(buildSpacer());

	addHelp(textfield_layout, options, no_spacers);
	this->setLayout(textfield_layout);
	setOptions(options);
}

/**
 * @brief Parse options for a Textfield from XML.
 * @param[in] options XML node holding the Textfield.
 */
void Textfield::setOptions(const QDomNode &options)
{
	validation_regex_ = options.toElement().attribute("validate");
	if (options.toElement().attribute("lenient").toLower() == "true")
		needs_prefix_for_evaluation_ = false; //always evaluate arithmetic expressions
	if (!options.toElement().attribute("placeholder").isEmpty())
		textfield_->setPlaceholderText(options.toElement().attribute("placeholder"));

	//user-set substitutions in expressions to style custom keys correctly:
	substitutions_ = expr::parseSubstitutions(options);
}

/**
 * @brief Event listener for changed INI values.
 * @details The "ini_value" property is set when parsing default values and potentially again
 * when setting INI keys while parsing a file.
 */
void Textfield::onPropertySet()
{
	const QString text_to_set( this->property("ini_value").toString() );
	if (ini_value_ == text_to_set)
		return;
	textfield_->setText(text_to_set);
	checkValue(text_to_set);
}

/**
 * @brief Perform checks on the entered text.
 * @details This function checks if the entered text stands for a recognized format such
 * as expressions (like the Number panel does), and if yes, styles the text box.
 * @param[in] text The current text to check.
 */
void Textfield::checkValue(const QString &text)
{
	/* check for coordinates */
	static const QString regex_coord(R"(\Alatlon\s*\(([-\d\.]+)(?:,)\s*([-\d\.]+)((?:,)\s*([-\d\.]+))?\))");
	static const QRegularExpression rex_coord(regex_coord);
	const QRegularExpressionMatch coord_match(rex_coord.match(text));

	static const int idx_full = 0;
	setDefaultPanelStyles(text);
	if (coord_match.captured(idx_full) == text && !text.isEmpty()) {
		check_button_->setVisible(true);
	} else {
		check_button_->setVisible(false);
		if (!validation_regex_.isNull()) { //check against regex specified in XML
			const QRegularExpression rex_xml(validation_regex_);
			const QRegularExpressionMatch xml_match(rex_xml.match(text));
			setValidPanelStyle(xml_match.captured(idx_full) == text && !text.isEmpty());
		} else { //check for (arithmetic) expressions
			bool evaluation_success;
			if (expr::checkExpression(text, evaluation_success, substitutions_, needs_prefix_for_evaluation_))
				setValidPanelStyle(evaluation_success);
		}
	}
	setIniValue(text); //checks are just a hint - set anyway
}

/**
 * @brief Event listener for the action button.
 * @details Currently, the button is only shown for coordinates and will open the Browser
 * with a map link.
 */
void Textfield::checkButtonClicked() //for now this button only pops up for coordinates
{
	static const QString regex_coord(R"(\Alatlon\s*\(([-\d\.]+)(?:,)\s*([-\d\.]+)((?:,)\s*([-\d\.]+))?\))");
	static const QRegularExpression rex_coord(regex_coord);
	const QRegularExpressionMatch coord_match(rex_coord.match(textfield_->text()));
	static const int idx_full = 0;
	static const int idx_lat = 1;
	static const int idx_lon = 2;

	if (coord_match.captured(idx_full) == textfield_->text()) {
		const QChar latchar = coord_match.captured(idx_lat).toDouble() < 0? 'S' : 'N';
		const QChar lonchar = coord_match.captured(idx_lon).toDouble() < 0? 'W' : 'E';
		QString url("https://tools.wmflabs.org/geohack/geohack.php?params=");
		url += coord_match.captured(idx_lat) + "_" + latchar + "_";
		url += coord_match.captured(idx_lon) + "_" + lonchar;
		QDesktopServices::openUrl(url);
	}
}
