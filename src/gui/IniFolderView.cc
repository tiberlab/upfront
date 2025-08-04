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

#include "IniFolderView.h"
#include "src/main/common.h"
#include "src/main/constants.h"
#include "src/main/inishell.h"
#include "src/main/settings.h"

#include <QCursor>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QVBoxLayout>

#ifdef DEBUG
	#include <iostream>
#endif

/**
 * @class IniFolderView
 * @brief Constructor for the folder navigation panel.
 * @details This constructor initializes a file system model and connects a tree view with
 * it, which the user can navigate via separate buttons.
 * @param[in] parent This panel's parent window.
 */
IniFolderView::IniFolderView(QWidget *parent) : QWidget(parent)
{
	static const QStringList filters = {"*.ini"};

	/* info label and file system model */
	path_label_ = new QLabel;
	path_label_->setStyleSheet("QLabel {background-color: " + colors::getQColor("app_bg").name() + "}");
	filesystem_model_ = new QFileSystemModel;
	filesystem_model_->setNameFilters(filters);
	//the model's root path is the computer's root path; only the tree root is changed later
	filesystem_model_->setRootPath("");
	filesystem_model_->setNameFilterDisables(false); //hide unfit items instead of disabling them

	path_label_->setText(".");
	path_label_->setProperty("path", path_label_->text());
	path_label_->setWordWrap(true);
	path_label_->setAlignment(Qt::AlignCenter);

	/* file model tree view */
	filesystem_tree_ = new QTreeView;
	connect(filesystem_tree_, &QTreeView::doubleClicked, this, &IniFolderView::onFilesysDoubleclicked);
	filesystem_tree_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(filesystem_tree_, &QTreeView::customContextMenuRequested, this,
	    &IniFolderView::onContextMenuRequest);

	filesystem_tree_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	filesystem_tree_->setUniformRowHeights(true);
	filesystem_tree_->setWordWrap(true);
	//expand columns instead of abbreviations for the files:
	filesystem_tree_->header()->setSectionResizeMode(QHeaderView::Stretch);
	filesystem_tree_->resizeColumnToContents( 0 );
	filesystem_tree_->setIndentation(Cst::treeview_indentation_);
	filesystem_tree_->setHeaderHidden(true);
	filesystem_tree_->setModel(filesystem_model_);
	filesystem_tree_->setEnabled(false); //will get enabled if an XML is loaded
	for (int ii = 1; ii < filesystem_model_->columnCount(); ++ii)
		filesystem_tree_->hideColumn(ii); //show only name column
	filesystem_tree_->setToolTip(tr("INI files on your computer.\nDouble-click to open, right-click for more options."));
	//set the directory the user has visited last time:
	const QString last_path( getSetting("auto::history::ini_folder", "path") );
	if (!last_path.isEmpty()) {
		setTreeIndex(filesystem_model_->index(last_path), true);
	} else { //by default, show the whole system tree and select the current directory:
		setTreeIndex(filesystem_model_->index(QDir::currentPath()));
		filesystem_tree_->scrollTo(filesystem_model_->index(QDir::currentPath()));
		filesystem_tree_->expand(filesystem_model_->index(QDir::currentPath()));
		filesystem_tree_->setCurrentIndex(filesystem_model_->index(QDir::currentPath()));
	}

	/* folder navigation toolbar */
	auto *button_back = new QToolButton;
	button_back->setIconSize(QSize(Cst::width_button_std, Cst::height_button_std));
	button_back->setAutoRaise(true);
	button_back->setIcon(getIcon("go-previous"));
	button_back->setToolTip(tr("Back"));
	connect(button_back, &QToolButton::clicked, this, &IniFolderView::onBackClicked);
	auto *button_up = new QToolButton;
	button_up->setIconSize(button_back->iconSize());
	button_up->setAutoRaise(true);
	button_up->setIcon(getIcon("go-up"));
	button_up->setToolTip(tr("Parent folder"));
	connect(button_up, &QToolButton::clicked, this, &IniFolderView::onUpClicked);
	auto *button_home = new QToolButton;
	button_home->setIconSize(button_back->iconSize());
	button_home->setAutoRaise(true);
	button_home->setIcon(getIcon("user-home"));
	button_home->setToolTip(tr("Home directory"));
	connect(button_home, &QToolButton::clicked, this, &IniFolderView::onHomeClicked);
	auto *button_working = new QToolButton;
	button_working->setIconSize(button_back->iconSize());
	button_working->setAutoRaise(true);
	button_working->setIcon(getIcon("folder-open"));
	button_working->setToolTip(tr("Working directory"));
	connect(button_working, &QToolButton::clicked, this, &IniFolderView::onWorkingClicked);
	//note: the connection to the file system is live, so we don't need a 'Refresh' button

	createContextMenu();

	/* layout of the navigation buttons */
	auto *toolbar_layout = new QHBoxLayout;
	toolbar_layout->setSpacing(0);
	toolbar_layout->addWidget(button_back);
	toolbar_layout->addWidget(button_up);
	toolbar_layout->addWidget(button_home);
	toolbar_layout->addWidget(button_working);
	toolbar_layout->addSpacerItem(new QSpacerItem(-1, -1, QSizePolicy::Expanding, QSizePolicy::Minimum));

	/* main layout */
	auto *main_layout = new QVBoxLayout;
	main_layout->addLayout(toolbar_layout);
	main_layout->addWidget(filesystem_tree_);
	main_layout->addWidget(path_label_);

	this->setLayout(main_layout);
}

/**
 * @brief Public access to enable/disable to folder tree view.
 * @param[in] enabled True to enable, false to disable.
 */
void IniFolderView::setEnabled(const bool &enabled)
{
	filesystem_tree_->setEnabled(enabled);
}

/**
 * @brief Set the root path of the tree view on user interaction.
 * @details This function basically hides the tree and switches to a clear list view of the
 * desired folder. This way, users can navigate to their main INI folder and have a cleaner
 * look of it until they navigate back out.
 * @param[in] index File model / tree view index.
 * @param[in] no_stack Don't add to ad-hoc history.
 */
void IniFolderView::setTreeIndex(const QModelIndex &index, const bool &no_stack)
{
	if (!no_stack) //current path for the back button
		prev_index_stack_.push(filesystem_tree_->rootIndex());
	filesystem_tree_->setRootIndex(index); //new path
	path_label_->setText(filesystem_model_->filePath(index));
	//remember the path for when the panel gets enabled and an info text disappears:
	path_label_->setProperty("path", path_label_->text());
}

/**
 * @brief Create the IniFolderView's context menu.
 */
void IniFolderView::createContextMenu()
{
	ini_folder_context_menu_.addAction(tr("Open in editor"));
	//"append" would be more logical (like for XMLs), but then we have to handle keeping the
	//original file path in the INIParser etc. pp. while this way it's a free feature:
	ini_folder_context_menu_.addAction(tr("Load on top of current INI values"));
}

/**
 * @brief Helper function to remember the last INI path the user has selected.
 */
void IniFolderView::updateLastPath()
{
	setSetting("auto::history::ini_folder", "path",
	    filesystem_model_->filePath(filesystem_tree_->currentIndex()));
}
/**
 * @brief Event listener for double-clicks in the file system.
 * @details This function switches folders and opens files, depending on what is double-clicked.
 * @param[in] index Clicked file model / tree view index.
 */
void IniFolderView::onFilesysDoubleclicked(const QModelIndex &index)
{
	auto finfo = QFileInfo(filesystem_model_->filePath(index));
	if (finfo.isDir()) {
		setTreeIndex(index);
		updateLastPath();
	} else {
		getMainWindow()->openIni(finfo.filePath());
	}
}

/**
 * @brief Event listener for when the context menu is requested.
 */
void IniFolderView::onContextMenuRequest(QPoint /*coords*/)
{
	QAction *selected = ini_folder_context_menu_.exec(QCursor::pos());
	if (selected) { //something was clicked
		auto finfo = QFileInfo(filesystem_model_->filePath(filesystem_tree_->currentIndex()));
		if (!finfo.exists())
			return;
		//:Translation hint: This is a check to match a context menu click
		if (selected->text().startsWith(tr("Open in editor"))) {
			QDesktopServices::openUrl(QUrl(finfo.filePath()));
		//:Translation hint: This is a check to match a context menu click
		} else if (selected->text().startsWith(tr("Load on top"))) {
			getMainWindow()->openIni(finfo.filePath(), false, false);
		}
	}
}

/**
 * @brief Navigate one level up in the folder hirarchy.
 */
void IniFolderView::onUpClicked()
{
	setTreeIndex(filesystem_tree_->rootIndex().parent());
	updateLastPath();
}

/**
 * @brief Navigate to the user's home folder.
 */
void IniFolderView::onHomeClicked()
{
	setTreeIndex(filesystem_model_->index(QDir::homePath()));
	updateLastPath();
}

/**
 * @brief Navigate back one step in the folder hierarchy.
 * @details Note that this only navigates back to folders that have been double-clicked.
 */
void IniFolderView::onBackClicked()
{
	if (!prev_index_stack_.isEmpty())
		setTreeIndex(prev_index_stack_.pop(), true);
	updateLastPath();
}

/**
 * @brief Navigate to the current working directory.
 */
void IniFolderView::onWorkingClicked()
{
	setTreeIndex(filesystem_model_->index(QDir::currentPath()));
	updateLastPath();
}
