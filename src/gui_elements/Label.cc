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

#include "Label.h"
#include "src/main/colors.h"
#include "src/main/inishell.h"

#include <QVBoxLayout>

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class Label
 * @brief Default constructor for a Label.
 * @details A label displays styled text, for example for INI keys.
 * @param[in] section INI section the described value belongs to. Could be used for style targeting
 * but is ignored otherwise.
 * @param[in] key INI key corresponding to the value that is being described. If no label is set then
 * this text will be displayed.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] in_label Label to display instead of the key.
 * @param[in] parent The parent widget.
 */
Label::Label(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    const QString &in_label, QWidget *parent) : Atomic(section, key, parent)
{
	//labels can go as standalones in sections, but they don't hold INI values:
	this->setProperty("no_ini", true);
	/* label styling */
	QString label( options.toElement().attribute("label") );
	if (label.isNull()) //probably not read from XML
		label = in_label; //allow a label different to the key
	label_ = new QLabel(label);
	label_->setTextInteractionFlags(Qt::TextSelectableByMouse); //key names can be copied
	label_->setContextMenuPolicy(Qt::NoContextMenu); //show our panel context menu instead of text context menu
	QPalette label_palette( this->palette() );
	QString label_color;
	if (!options.toElement().attribute("color").isNull())
		label_color = options.toElement().attribute("color");
	else
		label_color = "normal";
	label_palette.setColor(QPalette::WindowText, colors::getQColor(label_color));
	label_->setPalette(label_palette);
	label_->setFont(setFontOptions(label_->font(), options.toElement()));

	/* main layout */
	auto *layout( new QVBoxLayout );
	layout->addWidget(label_);
	setLayoutMargins(layout);
	this->setLayout(layout);
	const bool is_long_label = options.toElement().attribute("longlabel").toLower() == "true";
	if (!no_spacers)
		this->setMinimumWidth(getColumnWidth(label,
		    is_long_label? Cst::width_long_label : Cst::width_label)); //set to minimal required room
	//set a fixed with so that window resizing does not introduce unneccessary margins:
	this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

/**
 * @brief Check if a label actually has text.
 * @return True if the label displays some text.
 */
bool Label::isEmpty()
{
	return label_->text().isEmpty();
}

/**
 * @brief Retrieve the minimal width required for the label.
 * @details A fixed minimal width is set for labels so that they act as the first "column" of the interface
 * making it more visually appealing. If a label needs more space than that it is extended.
 * @param[in] text Text that will be displayed.
 * @param[in] min_width The minimum "column" width for the (INI key) labels.
 * @return The size to set the label to.
 */
int Label::getColumnWidth(const QString &text, const int& min_width)
{
	const QFontMetrics font_metrics( this->font() );
	const int text_width = font_metrics.boundingRect(text).width();
	if (text_width > min_width)
		return text_width + Cst::label_padding;
	else
		return min_width + Cst::label_padding;
}
