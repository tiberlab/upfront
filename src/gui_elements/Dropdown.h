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
 * The Dropdown panel is a combo box that displays child panels dependent on which item is selected.
 * It can also be used to auto-complete text input (potentially showing info when texts are matched).
 * 2019-11
 */

#ifndef DROPDOWN_H
#define DROPDOWN_H

#include "Atomic.h"
#include "Group.h"

#include <QComboBox>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtXml>

#include <vector>

class Dropdown : public Atomic {
	Q_OBJECT
friend class Selector;
	public:
		explicit Dropdown(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);
		Group * getContainer() const { return container_; }
		QString currentText() const;
		int getComboBoxHeight() const; //to size panels using the Dropdown
		void clear(const bool &set_default = true) override;

	private:
		void setOptions(const QDomNode &options);
		QString getCurrentText() const;
		void styleTimer();

		std::vector<QDomElement> child_nodes_; //cache for child panels
		QStringList item_help_texts_;
		QComboBox *dropdown_ = nullptr;
		Group *container_ = nullptr;
		Helptext *main_help_ = nullptr;
		bool has_child_helptexts_ = false;
		bool booleans_only_ = true; //XML options are all booleans
		bool numeric_ini_value_ = false; //user prefers 1/0 to true/false in INI

	private slots:
		void itemChanged(int index);
		void editTextChanged(const QString &text); //free text mode
		void onPropertySet() override;
};

#endif //DROPDOWN_H
