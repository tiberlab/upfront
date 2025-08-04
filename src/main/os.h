/*****************************************************************************/
/*  Copyright 2020 WSL Institute for Snow and Avalanche Research  SLF-DAVOS  */
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
 * Operating system specific functionalities.
 * 2020-03
 */

#ifndef OS_H
#define OS_H

#include <QString>
#include <QStringList>

namespace os {

void getSystemLocations(QStringList &locations);
bool isKde();
bool isDarkTheme();
QString getExtraPath(const QString& appname);
void setSystemPath(const QString& appname);
QString getHelpSequence();
QString getLogName();

} //endif namespace

#endif // OS_H
