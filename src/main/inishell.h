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
 * This file houses the main GUI building routine, and it provides some common functionality
 * across INIshell. Contrary to common.cc it is mainly for interface routines.
 * 2019-10
 */

#ifndef INISHELL_H
#define INISHELL_H

#include "src/gui/MainWindow.h"
#include "src/main/constants.h"
#include "src/main/colors.h"
#include "src/main/INIParser.h"

#include <QApplication>
#include <QString>
#include <QStringList>
#include <QWidgetList>
#include <QtXml>

class PropertyWatcher : public QObject
{
	Q_OBJECT

	public:
		PropertyWatcher(QObject *parent);

	signals:
		void changedValue();

	protected:
		bool eventFilter(QObject *obj, QEvent *event) override;
};

MainWindow* getMainWindow();
void recursiveBuild(const QDomNode &parent_node, Group *parent_group, const QString &parent_section,
    const bool& no_spacers = false);
bool parseAvailableSections(const QDomElement &current_element, const QString &parent_section, QStringList &section_list);
void topLog(const QString &message, const QString &color = "normal");
void topStatus(const QString &message, const QString &color = "normal", const bool &status_light = false,
    const int &time = -1);

#endif //INISHELL_H
