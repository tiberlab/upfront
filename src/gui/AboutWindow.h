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
 * This is the About window accessible through the help menu.
 * 2019-11
 */

#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <QKeyEvent>
#include <QTextBrowser>
#include <QWidget>

class AboutWindow : public QWidget {
	Q_OBJECT

	public:
		explicit AboutWindow(QWidget *parent = nullptr);

	protected:
		void keyPressEvent(QKeyEvent *event) override;

	private:
		void setAboutText(QTextBrowser *textbox);
};

#endif //ABOUTWINDOW_H
