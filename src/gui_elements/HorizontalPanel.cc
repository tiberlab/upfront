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

#include "HorizontalPanel.h"
#include "src/main/inishell.h"

/**
 * @class HorizontalPanel
 * @brief Default constructor for a HorizontalPanel.
 * @details A HorizontalPanel is a simple layout that organizes child widgets horizontally. The children
 * are given enclosed in <option> tags in the XML file.
 * @param[in] section The INI section is set for style targeting.
 * @param[in] key INI key is used for an optional label and ignored otherwise.
 * @param[in] options XML node responsible for this panel with all desired children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
HorizontalPanel::HorizontalPanel(const QString &section, const QString &key, const QDomNode &options,
    const bool &no_spacers, QWidget *parent) : Atomic(section, key, parent)
{
	horizontal_layout_ = new QHBoxLayout; //HorizontalPanel only holds a horizontal layout for child panels
	setLayoutMargins(horizontal_layout_);
	this->setLayout(horizontal_layout_);

	setOptions(options, no_spacers); //construct children
	addHelp(horizontal_layout_, options, no_spacers); //children and the panel can both have help texts
}

/**
 * @brief Parse options for a HorizontalPanel from XML.
 * @param[in] options XML node holding the HorizontalPanel.
 */
void HorizontalPanel::setOptions(const QDomNode &options, const bool &no_spacers)
{
	if (!key_.isEmpty() && options.toElement().attribute("label") != "") { //display caption
		auto *key_label( new Label(QString(), QString(), options, true, key_) );
		horizontal_layout_->addWidget(key_label, 0, Qt::AlignLeft | Qt::AlignCenter);
	}
	/* build all children */
	bool found_option = false;
	for (QDomElement op = options.firstChildElement(); !op.isNull(); op = op.nextSiblingElement()) {
		if (op.tagName() != "option" && op.tagName() != "o")
			continue; //short <o></o> notation
		if (!hasSectionSpecified(section_, op)) //option is excluded for this section
			continue;
		found_option = true;

		substituteKeys(op, "@", this->key_); //convenience substitution to shorten child names/labels
		auto *item_group( new Group(section_, "_horizontal_itemgroup_" + key_, false, false, false, true) ); //tight layout
		recursiveBuild(op, item_group, section_, true); //recursive build with horizontal space savings
		horizontal_layout_->addWidget(item_group, 0, Qt::AlignVCenter | Qt::AlignLeft);
	}
	if (!no_spacers)
		horizontal_layout_->addSpacerItem(buildSpacer()); //keep widgets to the left
	if (!found_option)
		topLog(tr(R"(XML error: No child panels specified for Grid "%1::%2")").arg(section_, key_), "error");
}
