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
 * If a help node is present, it is added to all panels and child panels. It can also be standalone.
 * 2019-10
 */

#ifndef HELPTEXT_H
#define HELPTEXT_H

#include <QLabel>
#include <QString>
#include <QtXml>

class Helptext : public QLabel {
	Q_OBJECT

	public:
		explicit Helptext(const QString &text, const bool &tight, const bool &single_line,
		    QWidget *parent = nullptr);
		Helptext(const QDomNode &options, QWidget *parent);
		void updateText(const QString &text);

	private:
		int getMinTextSize(const QString &text, const int &standard_width);
		void initHelpLabel();

		QLabel *help_ = nullptr;
};
#endif //HELPTEXT_H
