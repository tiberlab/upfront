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
 * A Choice panel displays a collection of checkboxes below each other and can show child panels
 * at the click of one or more of them. The syntax and behavior is the same as for a Checklist.
 * 2019-10
 */

#ifndef CHOICE_H
#define CHOICE_H

#include "Atomic.h"
#include "Group.h"

#include <QCheckBox>
#include <QString>
#include <QtXml>

#include <vector>

class Choice : public Atomic {
	Q_OBJECT

	public:
		explicit Choice(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);
		void setDefaultPanelStyles(const QString &in_value) override;

	private:
		void setOptions(const QDomNode &options);
		QString getOrderedIniList() const;
		void setChildVisibility(const int &index, const Qt::CheckState &checked);

		std::vector<int> ordered_item_list_; //order items were checked in
		Group * checkbox_container_ = nullptr;
		Group * child_container_ = nullptr;

	private slots:
		void changedState(int index);
		void onPropertySet() override;
};

#endif //CHOICE_H
