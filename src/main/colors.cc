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

#include "colors.h"
#include "src/main/os.h"
#include "src/main/settings.h"

#include <QDebug>
#include <QPalette>

namespace colors {

/**
 * @brief Decide whether to use dark mode dependent on user preference and system settings.
 * @return True if dark mode should be enabled.
 */
bool useDarkTheme()
{
	bool use_dark = false; //OFF if not set
	const QString darkmode_setting( getSetting("user::appearance::darkmode", "value") );
	if (darkmode_setting == "AUTO")
		use_dark = os::isDarkTheme();
	else if (darkmode_setting == "ON")
		use_dark = true;
	return use_dark;
}

/**
 * @brief Get color for a specific event or item.
 * @param[in] INIshell's name for the color.
 * @return Qt usable color.
 */
QColor getQColor(const QString &colorname)
{
	QString name(colorname.toLower());
	const bool use_darkmode = useDarkTheme(); //decide whether to use dark mode

	/* substitutions */
	if (name == "app_bg")
		return (use_darkmode)? QColor(0x31363b) : QColor("white");
	else if (name == "normal")
		return (use_darkmode)? QColor("white") : QColor();
	else if (name == "info")
		name = "sl_base01";
	else if (name == "error")
		name = "sl_red";
	else if (name == "warning")
		name = "sl_orange";
	else if (name == "special")
		name = "sl_blue";
	else if (name == "important")
		name = "sl_red";
	else if (name == "helptext")
		name = "sl_base1";
	else if (name == "mandatory")
		name = "sl_orange";
	else if (name == "default_values")
		name = "sl_base00";
	else if (name == "faulty_values")
		name = "sl_orange";
	else if (name == "valid_values")
		return (use_darkmode)? QColor("white") : QColor();
	else if (name == "number")
		name = "sl_cyan";
	else if (name == "groupborder")
		name = "sl_base1";
	else if (name == "frameborder")
		name = "sl_base1";

	/* syntax highlighter */
	if (name == "syntax_known_key")
		name = "sl_blue";
	else if (name == "syntax_unknown_key")
		name = "sl_yellow";
	else if (name == "syntax_known_section")
		name = (use_darkmode)? "sl_base2" : "sl_base02";
	else if (name == "syntax_unknown_section")
		name = "sl_orange";
	else if (name == "syntax_value")
		name = "sl_green";
	else if (name == "coordinate")
		name = "sl_cyan";
	else if (name == "syntax_background") {
		if (use_darkmode) return QColor(0x41464b);
		name = "sl_base3";
	} else if (name == "syntax_invalid")
		name = "sl_red";
	else if (name == "syntax_comment")
		name = "sl_base1";

	/* solarized named colors */
	if (name == "sl_base03")
		return QColor(0x002b36);
	else if (name == "sl_base02")
		return QColor(0x073642);
	else if (name == "sl_base01")
		return QColor(0x586e75);
	else if (name == "sl_base00")
		return QColor(0x657b83);
	else if (name == "sl_base0")
		return QColor(0x839496);
	else if (name == "sl_base1")
		return QColor(0x93a1a1);
	else if (name == "sl_base2")
		return QColor(0xeee8d5);
	else if (name == "sl_base3")
		return QColor(0xfdf6e3);
	else if (name == "sl_yellow")
		return QColor(0xb58900);
	else if (name == "sl_orange")
		return QColor(0xcb4b16);
	else if (name == "sl_red")
		return QColor(0xdc322f);
	else if (name == "sl_magenta")
		return QColor(0xd33682);
	else if (name == "sl_violet")
		return QColor(0x6c71c4);
	else if (name == "sl_blue")
		return QColor(0x268bd2);
	else if (name == "sl_cyan")
		return QColor(0x2aa198);
	else if (name == "sl_green")
		return QColor(0x859900);

#ifdef DEBUG //Qt will also correctly choose for strings like "red" but usually we want to be specific
	if (!name.isEmpty() && name.at(0) != "#")
		qDebug() << "Custom color not found:" << name;
#endif

	return {name}; //let Qt pick (e. g. hex codes), black if not valid
}

} //namespace colors
