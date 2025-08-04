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

/*
 * This file handles the program windows' sizes.
 * 2019-10
 */

#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#include <QScreen>
#include <QWidget>

namespace dim {

struct prop {
	int def_width = 0;
	int def_height = 0;
	int min_width = 0;
	int min_height = 0;
};

enum window_type {
	MAIN_WINDOW,
	LOGGER,
	PREVIEW
};

void setDimensions(QWidget *window, const window_type &type);

} //namespace dim

#endif //DIMENSIONS_H
