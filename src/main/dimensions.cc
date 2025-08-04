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

#include "dimensions.h"
#include "constants.h"
#include "src/main/settings.h"

#include <QGuiApplication>

namespace dim {

/**
 * @brief Set the startup dimension of various windows.
 * @param[in] window The window to resize.
 * @param[in] type The window type (Main, Logger, ...)
 */
void setDimensions(QWidget *window, const window_type &type)
{
	QScreen *screen_object = QGuiApplication::primaryScreen();
	QSize screen(screen_object->geometry().width(), screen_object->geometry().height());

	prop size; //default and minimum widths and heights
	double factor = 1.; //if the window doesn't fit, scale to screen size multiplied by this

	switch (type) {
	case MAIN_WINDOW:
		size.def_width = Cst::width_inishell_default;
		size.min_width = Cst::width_inishell_min;
		size.def_height = Cst::height_inishell_default;
		size.min_height = Cst::height_inishell_min;
		factor = 2/3.;
		break;
	case LOGGER:
		size.def_width = Cst::width_logger_default_;
		size.min_width = Cst::width_logger_min_;
		size.def_height = Cst::height_logger_default;
		size.min_height = Cst::height_logger_min;
		factor = 1/3.;
		break;
	case PREVIEW:
		size.def_width = Cst::width_preview_default;
		size.min_width = Cst::tiny;
		size.def_height = Cst::height_preview_default;
		size.min_height = Cst::tiny;
		factor = 1/2.;
	}

	if (getSetting("user::appearance::remembersizes", "value") == "TRUE") {
		//check if we can restore the window size from the last run:
		bool width_success, height_success;
		const int width_from_settings = getSetting("auto::sizes::window_" + QString::number(type), "width").toInt(&width_success);
		const int height_from_settings = getSetting("auto::sizes::window_" + QString::number(type), "height").toInt(&height_success);
		if (width_success && height_success) {
			size.def_width = width_from_settings;
			size.def_height = height_from_settings;
		}
	}
	if (screen.width() < size.def_width)
		size.def_width = static_cast<int>(screen.width() * factor);
	if (screen.height() < size.def_height)
		size.def_height = static_cast<int>(screen.height() * factor);

	window->resize(size.def_width, size.def_height);
	window->setMinimumSize(size.min_width, size.min_height);
}

} //end namespace dim
