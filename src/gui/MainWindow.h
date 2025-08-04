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
 * The main program window.
 * Every widget that has this as ancestor will be destroyed when the application quits.
 * 2019-10
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Logger.h"
#include "src/gui/MainPanel.h"
#include "src/gui/PreviewWindow.h"
#include "src/main/constants.h"
#include "src/main/INIParser.h"

#include <QAction>
#include <QCheckBox>
#include <QCommandLineParser>
#include <QKeyEvent>
#include <QIcon>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QString>
#include <QTabBar>
#include <QTimer>
#include <QtXml>
#include <QWidgetList>

class MouseEventFilter : public QObject { //handles all mouse clicks in the main window
	public:
		bool eventFilter(QObject *object, QEvent *event) override;
};

class MainWindow : public QMainWindow {
	Q_OBJECT //Qt macro to make code g++ ready

	public:
		explicit MainWindow(QString &settings_location, const QStringList &errors,
		    QMainWindow *parent = nullptr);
		~MainWindow() override;
		MainWindow(const MainWindow&) = delete; //we have non-Qt pointers, but copying does not make sense anyway
		MainWindow& operator =(MainWindow const&) = delete;
		MainWindow(MainWindow&&) = delete;
		MainWindow& operator=(MainWindow&&) = delete;
		
		void buildGui(const QDomDocument &xml);
		MainPanel * getControlPanel() const { return control_panel_; }
		QList<Atomic *> getPanelsForKey(const QString &ini_key);
		void setStatus(const QString &message, const QString &color = "normal", const bool &status_light = false,
		    const int &time = -1);
		void setStatusLight(const bool &on);
		void refreshStatus();
		void log(const QString &message, const QString &color = "normal") { logger_.log(message, color); }
		void setIni(const INIParser &ini_in) {ini_ = ini_in;}
		INIParser * getIni() { return &ini_; }
		INIParser getIniCopy() { return ini_; }
		void openIni(const QString &path, const bool &is_autoopen = false, const bool &fresh = true);
		bool setGuiFromIni(const INIParser &ini);
		void openXml(const QString &path, const QString &app_name, const bool &fresh = true,
		    const bool &is_settings_dialog = false);
		QString getCurrentApplication() const noexcept { return current_application_; }
		QString getXmlSettingsFilename() const noexcept { return xml_settings_filename_; }
		Logger * getLogger() noexcept { return &logger_; }

	public slots:
		void viewLogger();

	protected:
		void closeEvent(QCloseEvent* event) override;
		void keyPressEvent(QKeyEvent *event) override;

	private:
		void createMenu();
		void createToolbar();
		void createStatusbar();
		QWidgetList findPanel(QWidget *parent, const Section &section, const KeyValue &keyval);
		QWidgetList findSimplePanel(QWidget *parent, const Section &section, const KeyValue &keyval);
		QWidgetList prepareSelector(QWidget *parent, const Section &section, const KeyValue &keyval);
		QWidgetList prepareReplicator(QWidget *parent, const Section &section, const KeyValue &keyval);
		void saveIni(const QString &filename = QString());
		void saveIniAs();
		void openIni();
		bool closeIni();
		void clearGui(const bool &set_default = true);
		void setWindowSizeSettings();
		void setSplitterSizeSettings();
		void createToolbarContextMenu();

		QToolBar *toolbar_ = nullptr; //toolbar items
		QAction *toolbar_open_ini_ = nullptr;
		QAction *toolbar_clear_gui_ = nullptr;
		QAction *toolbar_save_ini_ = nullptr;
		QAction *toolbar_save_ini_as_ = nullptr;
		QAction *toolbar_preview_ = nullptr;
		QAction *file_open_ini_ = nullptr; //menu items
		QAction *file_save_ini_ = nullptr;
		QAction *file_save_ini_as_ = nullptr;
		QAction *gui_reset_ = nullptr;
		QAction *gui_close_all_ = nullptr;
		QAction *gui_clear_ = nullptr;
		QAction *view_preview_ = nullptr;
		QMenu toolbar_context_menu_;

		MainPanel *control_panel_ = nullptr;
		PreviewWindow *preview_ = nullptr;
		Logger logger_;
		INIParser ini_;
		QString xml_settings_filename_; //OS-specifc path to the settings file
		QLabel *status_label_ = nullptr;
		QLabel *status_icon_ = nullptr;
		QTimer *status_timer_ = nullptr;
		QLabel *ini_filename_ = nullptr;
		QCheckBox *autoload_box_ = nullptr;
		QAction *autoload_ = nullptr;
		bool help_loaded_ = false; //disable checks for help XML
		QString current_application_; //name of currently loaded XML

		MouseEventFilter *mouse_events_toolbar_ = nullptr;
		MouseEventFilter *mouse_events_statusbar_ = nullptr;

	public slots:
		void viewPreview(); //can be called from the Logger
		void loadHelp(const QString &tab_name = QString(),
		    const QString &frame_name = QString());
		void closeSettings();

	private slots:
		void clearStatus();
		void quitProgram();
		void resetGui();
		void viewSettings();
		void showWorkflow();
		void hideWorkflow();
		void loadHelpDev();
		void helpAbout();
		void toolbarClick(const QString &function);
		void onAutoloadCheck(const int &state);
		void onToolbarContextMenuRequest(const QPoint &pos);
		void writeGuiFromIniHeader(bool &first_error_message, const INIParser &ini);
#ifdef DEBUG
		void z_onDebugRunClick();
#endif //def DEBUG

	friend class MouseEventFilter; //mouse clicks may access everything
};

#endif //MAINWINDOW_H
