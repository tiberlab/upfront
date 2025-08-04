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
 * A TerminalView imitates the look for a command console and displays system commands' outputs.
 * 2019-10
 */

#ifndef TERMINALVIEW_H
#define TERMINALVIEW_H

#include <QPoint>
#include <QTextEdit>

class TerminalView : public QWidget {
	Q_OBJECT

	public:
		explicit TerminalView(QWidget *parent = nullptr);
		void log(const QString &text, const bool &is_std_err = false);

	private:
		void onContextMenuRequest(const QPoint &pos);

		QTextEdit *console_ = nullptr;
};

#endif //TERMINALVIEW_H
