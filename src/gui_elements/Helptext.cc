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

#include "Helptext.h"
#include "Atomic.h"
#include "src/main/colors.h"
#include "src/main/constants.h"

/**
 * @class Helptext
 * @brief Default constructor for a Helptext.
 * @details A Helptext is a styled label with properties set to display it on the far right of a layout.
 * The help text can either be displayed with a fixed width for a uniform look.
 * Or it can be displayed in a single line, in which case it is shown right next to the panel it describes
 * and extens as far as the text goes, viewable with the main horizontal scroll bar.
 * @param[in] text The help text.
 * @param[in] single_line If set, do not try to align the text in multiple columns. Rather, display it
 * in a single line and use the main scroll bars for it.
 * @param[in] parent The parent widget.
 */
Helptext::Helptext(const QString &text, const bool &tight, const bool &single_line, QWidget *parent)
    : QLabel(parent)
{
	initHelpLabel();
	if (!single_line) //no space to preceding elements, users scroll to the end
		this->setWordWrap(true); //multi line
	//Note: right-aligning small texts looks weird, so to get rid of a (potential) gap on the
	//right side for small texts we would probably need to check all Helptext widths and
	//then set to the required one. For now, we reserve a fixed space for all texts.
	if (tight)
		this->setFixedWidth(getMinTextSize(text, Cst::width_help));
	else
		this->setFixedWidth(Cst::width_help);
	this->setText(text);
	this->setFocusPolicy(Qt::NoFocus); //remove help texts from tab ordering
}

/**
 * @brief Constructor to use Helptext as a panel.
 * @details A Helptext is usually constructed from within panels, but it can also be specified
 * in the XML as usual and stand on its own. The style is kept consistent throughout Helptexts,
 * for custom styling use a Label.
 * @param[in] options XML node holding the Helptext and its text.
 * @param[in] parent The parent widget.
 */
Helptext::Helptext(const QDomNode &options, QWidget *parent) : QLabel(parent)
{
	const QString help_text( options.toElement().firstChildElement("help").text() );
	this->setToolTip(options.toElement().attribute("help"));
	initHelpLabel();
	this->setText(help_text);
	if (options.toElement().attribute("wrap").toLower() == "true")
		this->setWordWrap(true); //it's hard to decide by code and mostly necessary for the doc
}

/**
 * @brief Set a new help text.
 * @param[in] text The new help text to display.
 */
void Helptext::updateText(const QString &text)
{
	this->setText(text);
}

/**
 * @brief Helper function to set some properties of the internal help text label.
 */
void Helptext::initHelpLabel()
{
	this->setTextFormat(Qt::RichText);
	this->setTextInteractionFlags(Qt::TextBrowserInteraction);
	this->setOpenExternalLinks(true); //clickable links
	QPalette label_palette( this->palette() ); //custom color
	label_palette.setColor(QPalette::WindowText, colors::getQColor("helptext"));
	this->setPalette(label_palette);
}

/**
 * @brief Compare the standard Helptext width with the width of the text.
 * @details If the width of the text is smaller than the standard help text width then this
 * function returns the smaller text width. This way the element floats freely without
 * unnecessary margins.
 * @param[in] text Text that will be displayed.
 * @param[in] min_width
 * @return The fixed width to use for this Helptext.
 */
int Helptext::getMinTextSize(const QString &text, const int &standard_width)
{
	const QFontMetrics font_metrics( this->font() );
	const int text_width = font_metrics.boundingRect(text).width(); //horizontal pixels
	if (text_width < standard_width)
		return text_width + Cst::label_padding; //tiny bit of room to not wrap unexpectedly
	else
		return standard_width;
}
