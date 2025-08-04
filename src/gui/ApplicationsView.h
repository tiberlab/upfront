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
 * This panel displays a list of applications or simulations that are read from XML.
 * 2019-11
 */

#ifndef APPLICATIONSVIEW_H
#define APPLICATIONSVIEW_H

#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QMenu>
#include <QPoint>
#include <QString>
#include <QWidget>

class ApplicationsView : public QWidget {
	Q_OBJECT

	public:
		explicit ApplicationsView(QString tag_name, QWidget *parent = nullptr);
		void addApplication(const QFile &file, const QRegularExpressionMatch &match_inishell);
		void addInfoSeparator(const QString &text, const int &index);
		void clear() { application_list_->clear(); }
		int count() const { return application_list_->count(); }

	private:
		void createContextMenu();

		QListWidget *application_list_ = nullptr;
		QMenu list_context_menu_;
		QString tag_name_;

	private slots:
		void onListDoubleClick(QListWidgetItem *item);
		void showListContextMenu(const QPoint &coords);
};

#endif //APPLICATIONSVIEW_H
