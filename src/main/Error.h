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
 * An error class handling interactive and fatal errors, as well as warning and info texts.
 * 2019-10
 */

#ifndef ERROR_H
#define ERROR_H

#include <QString>

namespace error {
	enum urgency {
		info,
		warning,
		error,
		critical,
		fatal
	};
}

class Error {
	public:
		Error(const QString &message);
		Error(const QString &message, const QString &infotext);
		Error(const QString &message, const QString &infotext, const QString &details);
		Error(const QString &message, const QString &infotext, const QString &details,
		    const error::urgency &level, const bool &no_log = false);
};

class Info {
	public:
		Info(const QString &message);
};

int messageBox(const QString &message, const QString &infotext = "", const QString &details = "",
    const error::urgency &level = error::error, const bool &no_log = false);

#endif //ERROR_H
