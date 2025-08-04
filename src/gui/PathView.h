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
 * A PathView panel can be configured via XML and displays files for a single directory.
 * 2019-11
 */

#ifndef PATHVIEW_H
#define PATHVIEW_H

#include <QFileSystemModel>
#include <QListView>
#include <QStringList>
#include <QWidget>

class DragListModel : public QFileSystemModel { //for dragging files into other applications
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QStringList mimeTypes() const override;
};

class PathView : public QWidget {
	Q_OBJECT

	public:
		explicit PathView(QWidget *parent = nullptr);
		void setPath(const QString &path);

	private:
		QListView *file_list_ = nullptr;
		DragListModel *file_model_ = nullptr;
};

#endif //PATHVIEW_H
