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
 * The left part of the main window offering numerous possibilites to set up a workflow around
 * INI-controlled software from creating the INI to displaying simulation results.
 * 2019-11
 */

#ifndef WORKFLOW_H
#define WORKFLOW_H

#include "src/gui/ApplicationsView.h"
#include "src/gui/TerminalView.h"
#include "src/gui/IniFolderView.h"
#include "src/main/colors.h"

#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QTimer>
#include <QToolBox>
#include <QWidget>
#include <QtXml>

class WorkflowPanel : public QWidget {
	Q_OBJECT

	public:
		explicit WorkflowPanel(QWidget *parent = nullptr);
		void buildWorkflowPanel(const QDomDocument &xml);
		void scanFoldersForApps();
		void clearXmlPanels();
		IniFolderView * getFilesystemView() const noexcept { return filesystem_; }

	private:
		void buildWorkflowSection(QDomElement &section);
		QWidget * workflowElementFactory(QDomElement &item, const QString& appname);
		void readAppsFromDirs(bool &applications_found, bool &simulations_found);
		QString parseCommand(const QString &action, QPushButton *button, QLabel *status_label);
		void commandSubstitutions(QString &command, QLabel *status_label);
		bool actionOpenUrl(const QString &command) const;
		bool actionSwitchPath(const QString &command, QLabel *status_label, const QString &ref_path);
		bool actionClickButton(const QString &command, QPushButton *button, QLabel *status_label);
		bool actionSystemCommand(const QString& command, QPushButton *button, const QString &ref_path, const QString &appname);
		QString getWidgetValue(QWidget *widget) const;
		void processFinished(int exit_code, QProcess::ExitStatus exit_status, TerminalView *terminal,
		    QPushButton *button);
		void processStandardOutput(TerminalView *terminal);
		void processStandardError(TerminalView *terminal);
		void processErrorOccured(const QProcess::ProcessError &error, TerminalView *terminal, QPushButton *button);
		void workflowStatus(const QString &message, QLabel *status_label);
		QString setReferencePath(const QPushButton *button, const QString &ini_path) const;

		QToolBox *workflow_container_ = nullptr;
		ApplicationsView *applications_ = nullptr;
		ApplicationsView *simulations_ = nullptr;
		IniFolderView *filesystem_ = nullptr;
		bool clicked_button_running_ = false;

	private slots:
		void buttonClicked(QPushButton *button, const QStringList &action_list, const QString& appname);
		void toolboxClicked(int index);
};

#endif //WORKFLOW_H
