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
 * A logging window to display info and warnings on demand.
 * 2019-10
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QKeyEvent>
#include <QListWidget>
#include <QString>
#include <QWidget>

class Logger : public QWidget {
	Q_OBJECT

	public:
		explicit Logger(QWidget *parent = nullptr);
		void log(const QString &message, const QString &color = "normal",
		    const bool &no_timestamp = false);
		void logSystemInfo();

	protected:
		void keyPressEvent(QKeyEvent *event) override;

	private:
		QListWidget *loglist_ = nullptr;

	private slots:
		void saveLog();
		void closeLogger();
		void clearLog();
};

#endif // LOGGER_H
