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
 * A HorizontalPanel has no interactive elements on its own and only provides a raster to build child
 * panels into, for example to lay out settings for an Alternative or Choice panel child.
 * 2019-10
 */

#ifndef HORIZONTALPANEL_H
#define HORIZONTALPANEL_H

#include "Atomic.h"

#include <QHBoxLayout>
#include <QString>
#include <QWidget>
#include <QtXml>

class HorizontalPanel : public Atomic {
	Q_OBJECT

	public:
		explicit HorizontalPanel(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);

	private:
		void setOptions(const QDomNode &options, const bool &no_spacers);

		QHBoxLayout *horizontal_layout_ = nullptr;
};

#endif //HORIZONTALPANEL_H
