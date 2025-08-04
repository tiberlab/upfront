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

#include "Spacer.h"

#include "src/main/constants.h"
#include "src/main/inishell.h"

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @brief Default constructor for a Spacer.
 * @details A QSpacerItem needs separate handling everywhere (it's not a QWidget), which is why
 * we simply use an empty widget for this purpose.
 * @param[in] options XML node for the spacer with its options.
 * @param[in] parent The Spacer's parent panel/widget.
 */
Spacer::Spacer(const QDomNode &options, QWidget *parent) : QWidget(parent)
{
	int height = Cst::default_spacer_size;
	int width = Cst::default_spacer_size;
	bool success = true;
	const QDomElement op_el( options.toElement() );
	/* look for "height" or "h" */
	if (!op_el.attribute("height").isNull())
		height = op_el.attribute("height").toInt(&success);
	if (!success && !op_el.attribute("h").isNull())
		height = op_el.attribute("h").toInt(&success);
	if (!success)
		topLog(tr(R"(XML error: Could not parse height for spacer element)"), "error");
	/* look for "width" or "w" */
	success = true;
	if (!op_el.attribute("width").isNull())
		width = op_el.attribute("width").toInt(&success);
	if (!success && !op_el.attribute("w").isNull())
		width = op_el.attribute("w").toInt(&success);
	if (!success)
		topLog(tr(R"(XML error: Could not parse width for spacer element)"), "error");
	this->setFixedSize(width, height);
}
