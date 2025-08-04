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

#include "GridPanel.h"
#include "src/main/inishell.h"

/**
 * @class GridPanel
 * @brief Default constructor for a GridPanel.
 * @details A GridPanel is a simple grid layout that organizes child widgets on a raster. The children
 * are given enclosed in <option> tags in the XML file.
 * @param[in] section The INI section is set for style targeting.
 * @param[in] key INI key is used for an optional label and ignored otherwise.
 * @param[in] options XML node responsible for this panel with all desired children.
 * @param[in] parent The parent widget.
 */
GridPanel::GridPanel(const QString &section, const QString &key, const QDomNode &options, QWidget *parent)
    : Atomic(section, key, parent)
{
	grid_layout_ = new QGridLayout; //GridPanel only holds a grid layout for child panels
	setLayoutMargins(grid_layout_);
	this->setLayout(grid_layout_);
	setOptions(options); //construct children
}

/**
 * @brief Parse options for a GridPanel from XML.
 * @param[in] options XML node holding the GridPanel.
 */
void GridPanel::setOptions(const QDomNode &options)
{
	/* construct all child elements */
	bool found_option = false;
	for (QDomElement op = options.firstChildElement(); !op.isNull(); op = op.nextSiblingElement()) {
		if (op.tagName() != "option" && op.tagName() != "o")
			continue; //short <o></o> notation possible
		if (!hasSectionSpecified(section_, op)) //option is excluded for this section
			continue;
		found_option = true;

		substituteKeys(op, "@", this->key_); //convenience substitution to shorten child names/labels
		auto *item_group( new Group(section_, "_grid_itemgroup_" + key_, false, false, false, true) ); //tight layout

		const bool raster_unspecified = (op.attribute("row").isNull() && op.attribute("r").isNull())
		    || (op.attribute("column").isNull() && op.attribute("c").isNull());
		//recursive build with horizontal space savings unless the grid is used as a vertical panel collection:
		recursiveBuild(op, item_group, section_, !raster_unspecified);

		if (raster_unspecified) {
			grid_layout_->addWidget(item_group); //let Qt choose
			continue;
		}

		/* parse row and column span arguments */
		unsigned int rowspan = 1; //rowspan or colspan can be given on their own
		unsigned int colspan = 1;
		bool rowspan_success = true; //if available, they must be a positive integer
		bool colspan_success = true;
		if (!op.attribute("rowspan").isNull())
			rowspan = op.attribute("rowspan").toUInt(&rowspan_success);
		if (!op.attribute("colspan").isNull())
			colspan = op.attribute("colspan").toUInt(&colspan_success);
		if (!rowspan_success || !colspan_success) {
			topLog(tr(R"(XML error: Unsuitable rowspan or colspan argument in grid for key "%1::%2")").arg(
			    section_, key_), "error");
			continue;
		}

		/* parse row and column */
		bool row_success = false, col_success = false;
		unsigned int row = op.attribute("row").toUInt(&row_success) - 1; //indices start at 1 in XML file
		if (!row_success) //try the shortcut version
			row = op.attribute("r").toUInt(&row_success) -1;
		unsigned int column = op.attribute("column").toUInt(&col_success) - 1;
		if (!col_success)
			column = op.attribute("c").toUInt(&col_success) -1;
		if (!(row_success && col_success)) //row and column must be both specified (or both missing)
			topLog(tr(R"(XML error: Unsuitable or missing grid row or column index (both must be an integer equal or greater than 1) for key "%1::%2")").arg(
			    section_, key_), "error");
		else
			grid_layout_->addWidget(item_group, static_cast<int>(row), static_cast<int>(column),
			    static_cast<int>(rowspan), static_cast<int>(colspan), Qt::AlignVCenter | Qt::AlignLeft);
	}
	if (!found_option)
		topLog(tr(R"(XML error: No child panels specified for Grid "%1::%2")").arg(section_, key_), "error");
}
