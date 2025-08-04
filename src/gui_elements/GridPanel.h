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
 * A GridPanel has no interactive elements on its own and only provides a raster to build child
 * panels into, for example to lay out complicated settings for an Alternative or Choice panel child.
 * 2019-10
 */

#ifndef GRIDPANEL_H
#define GRIDPANEL_H

#include "Atomic.h"

#include <QGridLayout>
#include <QString>
#include <QtXml>

class GridPanel : public Atomic {
	Q_OBJECT

	public:
		explicit GridPanel(const QString &section, const QString &key, const QDomNode &options,
		    QWidget *parent = nullptr);

	private:
		void setOptions(const QDomNode &options);

		QGridLayout *grid_layout_ = nullptr;
};

#endif //GRIDPANEL_H
