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
 * The central grouping element used to build all groups of panels in.
 * It is the same for the main tabs and child containers of panels to be suitable for recursion.
 * 2019-10
 */

#ifndef GROUP_H
#define GROUP_H

#include "Atomic.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>

class Group : public Atomic {
	Q_OBJECT

	public:
		explicit Group(const QString &section, const QString &key, const bool &has_border = false,
		    const bool &grid_layout = false, const bool &is_frame = false, const bool &tight = false,
		    const QString &caption = QString(), const QString &in_frame_color = QString(),
		    const QString &background_color = QString(),
		    QWidget *parent = nullptr);
		void addWidget(QWidget *widget); //for vertical layout
		void addWidget(QWidget *widget, int row, int column, int rowSpan = 1, int columnSpan = 1,
		    Qt::Alignment alignment = Qt::Alignment()); //for grid layout
		QLayout * getLayout() const { return layout_; }
		QGridLayout * getGridLayout() const { return qobject_cast<QGridLayout *>(layout_); }
		void erase();
		int count() const;
		bool isEmpty() const;

	private:
		QGroupBox *box_ = nullptr;
		QLayout *layout_ = nullptr;
};

#endif //GROUP_H
