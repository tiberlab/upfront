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
 * This is a static panel (that can not be generated from XML) to navigate a folder structure.
 * 2019-10
 */

#ifndef INIFOLDERVIEW_H
#define INIFOLDERVIEW_H

#include <QFileSystemModel>
#include <QLabel>
#include <QMenu>
#include <QStack>
#include <QString>
#include <QTreeView>
#include <QWidget>

class IniFolderView : public QWidget {
	Q_OBJECT

	public:
		explicit IniFolderView(QWidget *parent = nullptr);
		void setEnabled(const bool &enabled);
		QLabel * getInfoLabel() const noexcept { return path_label_; } //for info why the view is disabled

	private:
		void setTreeIndex(const QModelIndex &index, const bool &no_stack = false);
		void createContextMenu();
		void updateLastPath();

		QFileSystemModel *filesystem_model_ = nullptr;
		QTreeView *filesystem_tree_ = nullptr;
		QLabel *path_label_ = nullptr;
		QStack<QModelIndex> prev_index_stack_;
		QMenu ini_folder_context_menu_;

	private slots:
		void onFilesysDoubleclicked(const QModelIndex &index);
		void onContextMenuRequest(QPoint);
		void onUpClicked();
		void onHomeClicked();
		void onBackClicked();
		void onWorkingClicked();
};

#endif //INIFOLDERVIEW_H
