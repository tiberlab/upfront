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
 * The Checklist panel displays a list with checkable items that can activate child panels.
 * The syntax and behavior is the same as for a Checklist, but it is more compact.
 * 2019-10
 */

#ifndef CHECKLIST_H
#define CHECKLIST_H

#include "Atomic.h"
#include "Group.h"
#include "Helptext.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QString>
#include <QWidget>
#include <QtXml>

#include <vector>

class Checklist : public Atomic {
	Q_OBJECT

	public:
		explicit Checklist(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);

	private:
		void setOptions(const QDomNode &options);
		QString getOrderedIniList() const;
		void setChildVisibility(QListWidgetItem *item);

		std::vector<QListWidgetItem *> ordered_item_list_; //order items were checked in
		QListWidget *list_ = nullptr;
		Group *child_container_ = nullptr;
		QVBoxLayout *checklist_layout_ = nullptr;
		Helptext *main_help_ = nullptr;
		bool has_child_helptexts_ = false;

	private slots:
		void listClick(QListWidgetItem *);
		void onPropertySet() override;
};

#endif //CHECKLIST_H
