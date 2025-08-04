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
 * The ApplicationsView is a panel in the Workflow section of the program which can display a
 * list of suitable XML files, be it applications or simulations.
 */

#include "ApplicationsView.h"
#include "src/main/inishell.h"
#include "src/main/settings.h"

#include <QAction>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QRegularExpression>
#include <QVBoxLayout>

#include <utility>

/**
 * @class ApplicationsView
 * @brief Constructor for an ApplicationsView.
 * @param[in] tag_name A human readable name for this panel which is used to e. g. store settings.
 * It could look like "Applications" or "Simulations".
 * @param[in] parent This panel's parent window.
 */
ApplicationsView::ApplicationsView(QString tag_name, QWidget *parent)
    : QWidget(parent), tag_name_(std::move(tag_name))
{
	application_list_ = new QListWidget;
	application_list_->setWordWrap(true); //for the path hints

	/* connect the context menu and mouse clicks */
	application_list_->setContextMenuPolicy(Qt::CustomContextMenu);
	createContextMenu();
	connect(application_list_, &QListWidget::customContextMenuRequested, this, &ApplicationsView::showListContextMenu);
	connect(application_list_, &QListWidget::itemDoubleClicked, this, &ApplicationsView::onListDoubleClick);
	application_list_->setToolTip(tr("List of your applications/simulations.\nDouble-click to open, right-click for more options."));
	auto *layout( new QVBoxLayout );
	layout->addWidget(application_list_);
	this->setLayout(layout);
}

/**
 * @brief Add an application/simulation XML file to the file list.
 * @param[in] file The file to display in the application list. It is already checked that this
 * should be an INIshell file.
 * @param[in] match_inishell The regular expression that was used to validate the file. It contains
 * other info which we don't have to extract again.
 */
void ApplicationsView::addApplication(const QFile &file, const QRegularExpressionMatch &match_inishell)
{
	static const int idx_name = 2;
	static const int idx_icon = 4;
	auto *app = new QListWidgetItem;
	app->setText(match_inishell.captured(idx_name)); //user given name

	//check if a specified icon is found in the same location:
	const QString icon_file(QFileInfo( file ).path() + "/" + match_inishell.captured(idx_icon));
	if (QFileInfo( icon_file ).isFile()) {
		const QIcon item_icon(icon_file);
		app->setIcon(item_icon);
	} else { //TODO: load nicer, specialized, icons in this case
		QFileIconProvider icon_provider;
		const QIcon item_icon( icon_provider.icon(QFileIconProvider::File) );
		app->setIcon(item_icon);
	}
	app->setData(Qt::UserRole, file.fileName()); //store the full file path
	app->setToolTip(file.fileName()); //(stored twice in case the ToolTip changes at some point)
	application_list_->addItem(app);
}

/**
 * @brief Add a non-clickable item to the list to mark different sections.
 * @param[in] text Text of the item.
 * @param[in] index Position to insert the separator. This can be set because we run through paths
 * first, and retrospectively insert it in case files were actually found.
 */
void ApplicationsView::addInfoSeparator(const QString &text, const int &index)
{
	auto *dir_sep( new QListWidgetItem );
	dir_sep->setText(text);
	dir_sep->setFlags(Qt::NoItemFlags);
	application_list_->insertItem(index, dir_sep);
}

/**
 * @brief Create the list's context menu.
 */
void ApplicationsView::createContextMenu()
{
	list_context_menu_.addAction(tr("Refresh"));
	list_context_menu_.addSeparator();
	list_context_menu_.addAction(tr("Open in editor"));
	list_context_menu_.addAction(tr("Append to current GUI"));
}

/**
 * @brief Event listener for a double-click into the list.
 * @details This function initiates loading of the clicked XML file.
 * @param[in] item The list item that was double-clicked.
 */
void ApplicationsView::onListDoubleClick(QListWidgetItem *item)
{
	getMainWindow()->openXml(item->data(Qt::UserRole).toString(), item->text());
	getMainWindow()->setWindowTitle(QCoreApplication::applicationName() + tr(" for ") + item->text());
	for (int ii = 0; ii < application_list_->count(); ++ii) //highlight selected
		application_list_->item(ii)->setBackground( colors::getQColor("app_bg") );
}

/**
 * @brief Event listener for requests to show the context menu.
 */
void ApplicationsView::showListContextMenu(const QPoint &/*coords*/)
{
	const QAction *selected( list_context_menu_.exec(QCursor::pos()) ); //show at cursor position
	if (selected) { //an item of the menu was clicked
		if (selected->text().startsWith(tr("Refresh"))) {
			getMainWindow()->getControlPanel()->getWorkflowPanel()->scanFoldersForApps();
		//:Translation hint: This is a check to match a context menu click
		} else if (selected->text().startsWith(tr("Append to current")) && application_list_->currentRow() != -1) {
			getMainWindow()->openXml(application_list_->currentItem()->data(Qt::UserRole).toString(),
			    application_list_->currentItem()->text(), false);
			//TODO: check what happens for multiple keys when multiple XMLs are loaded
		//:Translation hint: This is a check to match a context menu click
		} else if (selected->text().startsWith(tr("Open in editor"))) {
			QDesktopServices::openUrl(QUrl(application_list_->currentItem()->data(Qt::UserRole).toString()));
		}
	}
}
