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
 * The Checkbox panel displays a simple checkbox.
 * 2019-11
 */

#ifndef CHECKBOX_H
#define CHECKBOX_H

#include "Atomic.h"
#include "src/gui_elements/Group.h"

#include <QCheckBox>
#include <QString>
#include <QWidget>
#include <QtXml>

class Checkbox : public Atomic {
	Q_OBJECT

	public:
		explicit Checkbox(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);

	private:
		void setOptions(const QDomNode &options);

		QCheckBox *checkbox_ = nullptr;
		Group *margins_group_ = nullptr; //dummy parent for actual container (uniformity)
		Group *container_ = nullptr;
		bool numeric_ini_value_ = false; //user or XML prefers 0/1 to true/false
		bool lowercase_ini_value_ = false; //user or XML has set a lowercase "true" or "false"
		bool short_ini_value_ = false; //user is working with "t/f"

	private slots:
		void onPropertySet() override;
		void checkValue(const int &state);
};

#endif //CHECKBOX_H
