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

#include "PathView.h"
#include "src/main/constants.h"

#include <QDir>
#include <QVBoxLayout>

/**
 * @class DragListModel
 * @brief File system model with dragging abilities.
 * @details This class inherits from QFileSystemModel and enables the flags necessary for
 * the built-in dragging abilities.
 */

/**
 * @brief Set flags to enable dragging in the QFileSystemModel.
 * @param[in] index File model index of the item that is being draged.
 * @return Default flags plus dragging flag.
 */
Qt::ItemFlags DragListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags def_flags(QFileSystemModel::flags(index));
	if (index.isValid())
		return Qt::ItemIsDragEnabled | def_flags;
	return def_flags;
}

/**
 * @brief MIME data types enabled for dragging.
 * @details At the moment text files are enabled, for example .smet or .pro.
 * @return A list of the enabled MIME types.
 */
QStringList DragListModel::mimeTypes() const
{
	QStringList mime_types = { "application/vnd.text.list" }; //text
	return mime_types;
}

/**
 * @class PathView
 * @brief Default constructor for a PathView panel.
 * @details This constructor creates a file system model and connects a display list to it.
 * @param[in] parent The PathView's parent panel/window.
 */
PathView::PathView(QWidget *parent) : QWidget(parent)
{
	/* file system model and connected list view */
	file_model_ = new DragListModel; //subclass QFileSystemModel + dragging
	file_model_->setRootPath(""); //model is the whole file system
	file_list_ = new QListView;
	file_list_->setDragEnabled(true);
	file_list_->setModel(file_model_);
	file_list_->setRootIndex(file_model_->index(QDir::currentPath())); //default list view path
	file_list_->setToolTip(QDir::currentPath());

	//TODO: add navigation possibility for the PathView (toolbar or context menu)

	/* main layout */
	auto *main_layout = new QVBoxLayout;
	main_layout->addWidget(file_list_);
	this->setLayout(main_layout);
}

/**
 * @brief Set the path to display in the list.
 * @param path[in] The path to display.
 */
void PathView::setPath(const QString &path)
{
	file_list_->setRootIndex(file_model_->index(path));
	file_list_->setToolTip(path);
}
