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
 * The main information grouping element including the Workflow panel and main tab panel.
 * It is the central widget of the main window and is size-controlled by it.
 * All main content goes here.
 * 2019-10
 */

#ifndef MAINPANEL_H
#define MAINPANEL_H

#include "src/gui_elements/gui_elements.h"
#include "src/gui/WorkflowPanel.h"

#include <QList>
#include <QStackedWidget>
#include <QScrollArea>
#include <QSplitter>
#include <QString>
#include <QTabWidget>
#include <QWidget>

class ScrollPanel : public Atomic {
	Q_OBJECT

	public:
		explicit ScrollPanel(const QString &section, const QString &tab_color,
		    QWidget *parent = nullptr);
		Group * getGroup() const;

	private:
		QScrollArea *main_area_ = nullptr;
		Group *main_group_ = nullptr;
};

class MainPanel : public QWidget {
	Q_OBJECT

	public:
		explicit MainPanel(QWidget *parent = nullptr);
		ScrollPanel * getSectionScrollarea(const QString &section, const QString &background_color = QString(),
		    const QString &color = "normal", const bool &no_create = false);
		ScrollPanel * getSectionScrollArea(const int &index);
		WorkflowPanel * getWorkflowPanel() const { return workflow_panel_; }
		QStackedWidget * getWorkflowStack() const { return workflow_stack_; }
		QString setIniValuesFromGui(INIParser *ini);
		void displayInfo();
		QList<int> getSplitterSizes() const;
		void setSplitterSizes(QList<int> sizes = QList<int>());
		void clearGuiElements();
		void clearGui(const bool &set_default = true);
		int prepareSettingsTab();
		void closeSettingsTab();
		bool hasSettingsLoaded() { return settings_tab_idx_ != -1; }
		void displaySettings(const int &settings_tab_idx);
		bool showTab(const QString &tab_name);
		template <class T> void clearDynamicPanels();

	private:
		QString getShellSetting(QWidget *parent, const QString &option);
		void createExtraSettingsWidgets();

		WorkflowPanel *workflow_panel_ = nullptr;
		QStackedWidget *workflow_stack_ = nullptr;
		QTabWidget *section_tab_ = nullptr;
		QSplitter *splitter_ = nullptr;
		int settings_tab_idx_ = -1; //index of settings tab if loaded

	private slots:
		void saveSettings(const int &settings_tab_idx);
		void onTabCloseRequest(const int &idx);
};

#endif //MAINPANEL_H
