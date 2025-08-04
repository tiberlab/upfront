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
 * The main window that is displayed on startup.
 * 2019-10
 */

#include "MainWindow.h"
#include "src/gui/AboutWindow.h"
#include "src/gui_elements/Atomic.h"
#include "src/gui_elements/Group.h" //to exclude Groups from panel search
#include "src/main/colors.h"
#include "src/main/common.h"
#include "src/main/constants.h"
#include "src/main/Error.h"
#include "src/main/dimensions.h"
#include "src/main/INIParser.h"
#include "src/main/inishell.h"
#include "src/main/settings.h"
#include "src/main/XMLReader.h"

#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QMenuBar>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSpacerItem>
#include <QStatusBar>
#include <QSysInfo>
#include <QTimer>
#include <QToolBar>

#ifdef DEBUG
	#include <iostream>
#endif

/**
 * @class MouseEventFilter
 * @brief Mouse event listener for the main program window.
 * @param[in] object Object the event stems from.
 * @param[in] event The type of event.
 * @return True if the event was accepted.
 */
bool MouseEventFilter::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress) {
		if (object->property("mouseclick") == "open_log") {
			getMainWindow()->viewLogger();
		} else if (object->property("mouseclick") == "open_ini") {
			if (!getMainWindow()->ini_filename_->text().isEmpty())
				QDesktopServices::openUrl("file:" + getMainWindow()->ini_filename_->text());
		}
	}
	return QObject::eventFilter(object, event); //pass to actual event of the object
}

/**
 * @class MainWindow
 * @brief Constructor for the main window.
 * @param[in] settings_location Path to the INIshell settings file.
 * @param[in] errors List of errors that have occurred before starting the GUI.
 * @param[in] parent Unused since the main window is kept in scope by the entry point function.
 */
MainWindow::MainWindow(QString &settings_location, const QStringList &errors, QMainWindow *parent)
    : QMainWindow(parent), logger_(this), xml_settings_filename_(settings_location)
{
	/*
	 * Program flowchart sketch:
	 *
	 *   Set application style and properties  <----- main.cc ----->  cmd line options,
	 *                  |                                             translations
	 *         Load INIshell settings  <----- main.cc ----->  store in global XML node
	 *                  |
	 *             Launch GUI  <----- MainWindow.cc's constructor
	 *                  |
	 *  User opens:     v
	 *      application or simulation                   INI file  <----- INIParser::read()
	 *      -------------------------                   --------
	 *                  |                      (Only possible if an application is loaded)
	 *                  |                                   |
	 *                  |    Find panels by matching their IDs to INIs key <--- set file values
	 *                  |                                   |
	 *                  |                                   v      (ready for user interaction)
	 *                  |                   ________________________________________
	 *                  |                   |INI file contents are displayed in GUI|
	 *                  v                   ----------------------------------------
	 *   _________________________________
	 *   |  Build GUI elements from XML  |  <----- Scan file system for application XMLs
	 *   ---------------------------------           (read and check: XMLParser class)
	 *                  |                                  ^
	 *         Find panels in XML  <----- inishell.cc      |_ WorkflowPanel::scanFoldersForApps()
	 *                  |
	 *              Build panels  <----- gui_elements.cc ----->  The panels' constructors
	 *                  |
	 *        Set panels' properties  <----- the panels' setOptions() member functions
	 *                  |
	 * Give panels IDs to interact with INI changes  <----- Atomic::setPrimaryWidget()
	 *                  |                                     ^
	 *                  V                                     |
	 *            _______________             (Base class for all panels, uniform access to
	 *            |GUI is built!|             styling, section/key; used to find all panels)
	 *            ---------------
	 *                  |
	 *                  v
	 *          Set INI values from:
	 *          --------------------
	 *
	 *         XML              INI  <----- INIParser class
	 *   (default values)  (file system)                      VALIDATION
	 *          |                |                           (set styles)
	 *          |                |                                ^
	 *          v                v                               /
	 *    _____________________________________                 /
	 *    |      Panel's onPropertySet()      |  -----> CHECK VALUE
	 *    -------------------------------------           |
	 *          ^                ^        (Property "no_ini" not set? Value not empty?)
	 *          |                |                        |
	 *          |                |                        v    MainPanel::setIniValuesFromGui()
	 *         GUI              CODE                    OUTPUT            |
	 * (user interaction)  (reactive changes)        (on request)         |
	 *          ^                                         |               v
	 *          |           Run through panels by IDs (= INI keys) and fill INIParser
	 *   Detect changes                                   |
	 *   (when closing)  <----- INIParser::operator==     v
	 *                                               Output to file  <----- INIParser class
	 */

	status_timer_ = new QTimer(this); //for temporary status messages
	status_timer_->setSingleShot(true);
	connect(status_timer_, &QTimer::timeout, this, &MainWindow::clearStatus);

	logger_.logSystemInfo();
	for (auto &err : errors) //errors from main.cc before the Logger existed
		logger_.log(err, "error");

	/* retrieve and set main window geometry */
	dim::setDimensions(this, dim::MAIN_WINDOW);
	dim::setDimensions(&logger_, dim::LOGGER);

	/* create basic GUI items */
	this->setUnifiedTitleAndToolBarOnMac(true);
	this->setWindowTitle(QCoreApplication::applicationName());
	createMenu();
	createToolbar();
	createStatusbar();

	preview_ = new PreviewWindow(this);
	/* create the dynamic GUI area */
	control_panel_ = new MainPanel(this);
	this->setCentralWidget(control_panel_);
	ini_.setLogger(&logger_);
	if (errors.isEmpty())
		setStatus(tr("Ready."), "info");
	else
		setStatus(tr("Errors occurred on startup"), "error");
}

/**
 * @brief Destructor for the main window.
 * @details This function is called after all safety checks have already been performed.
 */
MainWindow::~MainWindow()
{ //safety checks are performed in closeEvent()
	setWindowSizeSettings(); //put the main window sizes into the settings XML
	saveSettings();
	delete mouse_events_toolbar_;
	delete mouse_events_statusbar_;
}

/**
 * @brief Build the dynamic GUI.
 * This function initiates the recursive GUI building with an XML document that was
 * parsed beforehand.
 * @param[in] xml XML to build the gui from.
 */
void MainWindow::buildGui(const QDomDocument &xml)
{
	setUpdatesEnabled(false); //disable painting until done
	QDomNode root = xml.firstChild();
	while (!root.isNull()) {
		if (root.isElement()) { //skip over comments
			//give no parent group - tabs will be created for top level:
			recursiveBuild(root, nullptr, QString());
			break;
		}
		root = root.nextSibling();
	}
	setUpdatesEnabled(true);
}

/**
 * @brief Retrieve all panels that handle a certain INI key.
 * @param[in] ini_key The INI key to find.
 * @return A list of all panels that handle the INI key.
 */
QList<Atomic *> MainWindow::getPanelsForKey(const QString &ini_key)
{
	QList<Atomic *> panel_list( control_panel_->findChildren<Atomic *>(Atomic::getQtKey(ini_key)) );
	for (auto it = panel_list.begin(); it != panel_list.end(); ++it) {
		//groups don't count towards finding INI keys (for this reason, they additionally
		//have the "no_ini" key set):
		if (qobject_cast<Group *>(*it))
			panel_list.erase(it);
	}
	return panel_list;
}

/**
 * @brief Save the current GUI to an INI file.
 * @details This function calls the underlying function that does this and displays a message if
 * the user has neglected to set some mandatory INI values.
 * @param[in] filename The file to save to. If not given, the current INI file will be used.
 */
void MainWindow::saveIni(const QString &filename)
{
	/*
	 * We keep the original INIParser as-is, i. e. we always keep the INI file as it was loaded
	 * originally. This is solemnly to be able to check if anything has changed through user
	 * interaction, and if yes, warn before closing.
	 */
	INIParser gui_ini = ini_; //an INIParser is safe to copy around - this is a deep copy
	gui_ini.clear(true);  //only keep unknown keys (which are transported from input to output)
	const QString missing( control_panel_->setIniValuesFromGui(&gui_ini) );

	if (!missing.isEmpty()) {
		QMessageBox msgMissing;
		msgMissing.setWindowTitle("Warning ~ " + QCoreApplication::applicationName());
		msgMissing.setText(tr("<b>Missing mandatory INI values.</b>"));
		msgMissing.setInformativeText(tr(
		    "Some non-optional INI keys are not set.\nSee details for a list or go back to the GUI and set all highlighted fields."));
		msgMissing.setDetailedText(tr("Missing INI keys:\n") + missing);
		msgMissing.setIcon(QMessageBox::Warning);
		msgMissing.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		msgMissing.setDefaultButton(QMessageBox::Cancel);
		int clicked = msgMissing.exec();
		if (clicked == QMessageBox::Cancel)
			return;
	}
	//if no file is specified we save to the currently open INI file (save vs. save as):
	gui_ini.writeIni(filename.isEmpty()? gui_ini.getFilename() : filename);
}

/**
 * @brief Save the currently set values to a new INI file.
 */
void MainWindow::saveIniAs()
{
	QString start_path( getSetting("auto::history::last_ini", "path") );
	if (start_path.isEmpty())
		start_path = getSetting("auto::history::last_preview_write", "path");
	if (start_path.isEmpty())
		start_path = QDir::currentPath();

	const QString filename = QFileDialog::getSaveFileName(this, tr("Save INI file"),
	    start_path + "/" + ini_filename_->text(),
	    "INI files (*.ini *.INI);;All files (*)", nullptr, QFileDialog::DontUseNativeDialog);
	if (filename.isNull()) //cancelled
		return;
	saveIni(filename);
	ini_.setFilename(filename); //the new file is the new current file like in all programs
	ini_filename_->setText(filename);
	autoload_->setVisible(true); //if started from an empty GUI, this could still be disabled
	toolbar_save_ini_->setEnabled(true); //toolbar entry
	file_save_ini_->setEnabled(true); //menu entry

	setSetting("auto::history::last_ini", "path", QFileInfo( filename ).absoluteDir().path());
}

/**
 * @brief Select a path for an INI file to be opened, and then open it.
 */
void MainWindow::openIni()
{
	QString start_path( getSetting("auto::history::last_ini", "path") );
	if (start_path.isEmpty())
		start_path = QDir::currentPath();

	const QString path = QFileDialog::getOpenFileName(this, tr("Open INI file"), start_path,
	    "INI files (*.ini);;All files (*)", nullptr,
	    QFileDialog::DontUseNativeDialog | QFileDialog::DontConfirmOverwrite);
	if (path.isNull()) //cancelled
		return;
	openIni(path);

	setSetting("auto::history::last_ini", "path", QFileInfo( path ).absoluteDir().path());
}

/**
 * @brief Set the loaded panels' values from an INI file.
 * @param[in] ini The INI file in form of an INIParser (usually the main one).
 * @return True if all INI keys are known to the loaded XML.
 */
bool MainWindow::setGuiFromIni(const INIParser &ini)
{
	bool all_ok = true;
	bool first_error_message = true;
	for (auto &sec : ini.getSectionsCopy()) { //run through sections in INI file
		ScrollPanel *tab_scroll = getControlPanel()->getSectionScrollarea(sec.getName(),
		    QString(), QString(), true); //get the corresponding tab of our GUI
		if (tab_scroll != nullptr) { //section exists in GUI
			const auto kv_list( sec.getKeyValueList() );
			for (size_t ii = 0; ii < kv_list.size(); ++ii) {
				//find the corresponding panel, and try to create it for dynamic panels
				//(e. g. Selector, Replicator):
				QWidgetList widgets( findPanel(tab_scroll, sec, *sec[ii]) );
				if (!widgets.isEmpty()) {
					for (int jj = 0; jj < widgets.size(); ++jj) //multiple panels can share the same key
						widgets.at(jj)->setProperty("ini_value", sec[ii]->getValue());
				} else {
					sec[ii]->setIsUnknownToApp();
					writeGuiFromIniHeader(first_error_message, ini);
					logger_.log(tr("%1 does not know INI key \"").arg(current_application_) +
					    sec.getName() + Cst::sep + sec[ii]->getKey() + "\"", "warning");
					all_ok = false;
				}
			} //endfor kv
		} else { //section does not exist
			writeGuiFromIniHeader(first_error_message, ini);
			log(tr(R"(%1 does not know INI section "[%2]")").arg(
			    current_application_, sec.getName()), "warning");
			all_ok = false;
		} //endif section exists
	} //endfor sec
	return all_ok;
}

/**
 * @brief Open an INI file and set the GUI to it's values.
 * @param[in] path The INI file to open.
 * @param[in] is_autoopen Is the INI being loaded through the autoopen mechanism?
 */
void MainWindow::openIni(const QString &path, const bool &is_autoopen, const bool &fresh)
{
	this->getControlPanel()->getWorkflowPanel()->setEnabled(false); //hint at INIshell processing...
	setStatus(tr("Reading INI file..."), "info", true);
	refreshStatus(); //necessary if heavy operations follow
	if (fresh)
		clearGui();
	const bool success = ini_.parseFile(path); //load the file into the main INI parser
	if (!setGuiFromIni(ini_)) //set the GUI to the INI file's values
		setStatus(tr("INI file read with unknown keys"), "warning");
	else
		setStatus(tr("INI file read ") + (success? tr("successfully") : tr("with warnings")),
		    (success? "info" : "warning")); //ill-formatted lines in INI file
	toolbar_save_ini_->setEnabled(true);
	file_save_ini_->setEnabled(true);
	autoload_->setVisible(true);
	ini_filename_->setText(path);
	autoload_box_->setText(tr("autoload this INI for ") + current_application_);
	if (!is_autoopen) //when a user clicks an INI file to open it we ask anew whether to autoopen
		autoload_box_->setCheckState(Qt::Unchecked);

	this->getControlPanel()->getWorkflowPanel()->setEnabled(true);
	QApplication::alert( this ); //notify the user that the task is finished
}

/**
 * @brief Close the currently opened INI file.
 * @details This function checks whether the user has made changes to the INI file and if yes,
 * allows to save first, discard changes, or cancel the operation.
 * @return True if the INI file was closed, false if the user has cancelled.
 */
bool MainWindow::closeIni()
{
	if (!help_loaded_ && //unless user currently has the help opened which we always allow to close
	    getSetting("user::inireader::warn_unsaved_ini", "value") == "TRUE") {
		/*
		 * We leave the original INIParser - the one that holds the values like they were
		 * originally read from an INI file - intact and make a copy of it. The currently
		 * set values from the GUI are loaded into the copy, and then the two are compared
		 * for changes. This way we don't display a "settings may be lost" warning if in
		 * fact nothing has changed, resp. the changes cancelled out.
		 */
		INIParser gui_ini = ini_;
		(void) control_panel_->setIniValuesFromGui(&gui_ini);
		if (ini_ != gui_ini) {
			QMessageBox msgNotSaved;
			msgNotSaved.setWindowTitle("Warning ~ " + QCoreApplication::applicationName());
			msgNotSaved.setText(tr("<b>INI settings will be lost.</b>"));
			msgNotSaved.setInformativeText(tr(
			    "Some INI keys will be lost if you don't save the current INI file."));
			msgNotSaved.setDetailedText(ini_.getEqualityCheckMsg());
			msgNotSaved.setIcon(QMessageBox::Warning);
			msgNotSaved.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard);
			msgNotSaved.setDefaultButton(QMessageBox::Cancel);

			auto *show_msg_again = new QCheckBox(tr("Don't show this warning again"));
			show_msg_again->setToolTip(tr("The warning can be re-enabled in the settings"));
			show_msg_again->setStyleSheet("QCheckBox {color: " + colors::getQColor("info").name() + "}");
			msgNotSaved.setCheckBox(show_msg_again);
			connect(show_msg_again, &QCheckBox::stateChanged, [](int state) {
				const bool checked = (state == Qt::CheckState::Checked);
				setSetting("user::inireader::warn_unsaved_ini", "value", !checked? "TRUE" : "FALSE");
			});

			int clicked = msgNotSaved.exec();
			if (clicked == QMessageBox::Cancel)
				return false;
			if (clicked == QMessageBox::Save)
				saveIni();
			delete show_msg_again;
		}
	} //endif help_loaded

	ini_.clear();
	toolbar_save_ini_->setEnabled(false);
	file_save_ini_->setEnabled(false);
	ini_filename_->setText(QString());
	autoload_->setVisible(false);
	return true;
}

/**
 * @brief Reset the GUI to default values.
 * @details This function checks whether there were any changes to the INI values
 * (if yes, the user can save first or discard or cancel), then clears the whole GUI.
 */
void MainWindow::clearGui(const bool &set_default)
{
	const bool perform_close = closeIni();
	if (!perform_close) //user clicked 'cancel'
		return;
	getControlPanel()->clearGui(set_default);
	ini_filename_->setText(QString());
	autoload_->setVisible(false);
}

/**
 * @brief Store the main window sizes in the settings XML.
 */
void MainWindow::setWindowSizeSettings()
{
	setSplitterSizeSettings();

	setSetting("auto::sizes::window_" + QString::number(dim::window_type::PREVIEW),
	    "width", QString::number(preview_->width()));
	setSetting("auto::sizes::window_" + QString::number(dim::window_type::PREVIEW),
	    "height", QString::number(preview_->height()));
	setSetting("auto::sizes::window_" + QString::number(dim::window_type::LOGGER),
	    "width", QString::number(logger_.width()));
	setSetting("auto::sizes::window_" + QString::number(dim::window_type::LOGGER),
	    "height", QString::number(logger_.height()));
	setSetting("auto::sizes::window_" + QString::number(dim::window_type::MAIN_WINDOW),
	    "width", QString::number(this->width()));
	setSetting("auto::sizes::window_" + QString::number(dim::window_type::MAIN_WINDOW),
	    "height", QString::number(this->height()));

	//remember the toolbar position:
	setSetting("auto::position::toolbar", "position", QString::number(this->toolBarArea(toolbar_)));
}

/**
 * @brief Remember the main splitter's space distribution.
 */
void MainWindow::setSplitterSizeSettings()
{
	QList<int> splitter_sizes(control_panel_->getSplitterSizes());
	setSetting("auto::sizes::splitter_workflow", "size", QString::number(splitter_sizes.at(0)));
	setSetting("auto::sizes::splitter_mainpanel", "size", QString::number(splitter_sizes.at(1)));
}

/**
 * @brief Create a context menu for the toolbar.
 */
void MainWindow::createToolbarContextMenu()
{
	/* allow user to enable toolbar dragging */
	auto *fix_toolbar_position = new QAction(tr("Fix toolbar position"), this);
	fix_toolbar_position->setCheckable(true);
	fix_toolbar_position->setChecked(getSetting("user::appearance::fix_toolbar_pos", "value") == "TRUE");
	toolbar_context_menu_.addAction(fix_toolbar_position);
}

/**
 * @brief Event handler for the "View::Preview" menu: open the preview window.
 * @details This is public so that the Preview can be shown from other windows
 * (e. g. the Logger).
 */
void MainWindow::viewPreview()
{
	if (view_preview_->isEnabled()) {
		preview_->addIniTab();
		preview_->show();
		preview_->raise();
	}
}

/**
 * @brief Load the user help XML from the resources onto the GUI.
 */
void MainWindow::loadHelp(const QString &tab_name, const QString &frame_name)
{
	clearGui();
	openXml(":doc/help.xml", "Help");
	help_loaded_ = true;

	if (tab_name.isEmpty())
		return;

#ifdef DEBUG
	const bool success = control_panel_->showTab(tab_name);
	if (!success)
		qDebug() << "Help section does not exist:" << tab_name;
#else
	(void) control_panel_->showTab(tab_name);
#endif //def DEBUG

	auto parent = control_panel_->getSectionScrollarea(tab_name);
	const QList<Group *> panel_list( parent->findChildren<Group *>() );
	for (auto &panel : panel_list) { //run through panels in section
		QString section, key;
		const QString value( panel->getIniValue(section, key) );
		if (QString::compare(key, frame_name, Qt::CaseInsensitive) == 0) {
			const QString id( section + Cst::sep + key );
			//this ID must be set in the help XML:
			auto wid = getMainWindow()->findChild<QGroupBox *>("_primary_" + Atomic::getQtKey(id));
			QString stylesheet(wid->styleSheet()); //original (to reset with timer)
			QString stylesheet_copy( stylesheet );
			wid->setStyleSheet(stylesheet_copy.replace( //flash frame border color
			    colors::getQColor("frameborder").name(QColor::HexRgb).toLower(),
			    colors::getQColor("important").name()));
			QTimer::singleShot(Cst::msg_short_length, this,
			    [=]{ wid->setStyleSheet(stylesheet); } );
		}
	} //endfor panel_list
	this->raise();
} //TODO: scroll down to section; for now, tend to put frames that can flash to the top

/**
 * @brief Event handler for the "View::Close settings" menu: close the settings tab.
 */
void MainWindow::closeSettings()
{
	control_panel_->closeSettingsTab();
	toolbar_clear_gui_->setEnabled(false);
	gui_clear_->setEnabled(false);
}

/**
 * @brief Open an XML file containing an application/simulation.
 * @param[in] path Path to the file to open.
 * @param[in] app_name When looking for applications, the application name was parsed and put
 * into a list. This is the app_name so we don't have to parse again.
 * @param[in] fresh If true, reset the GUI before opening a new application.
 * @param[in] is_settings_dialog Is it the settings that are being loaded?
 */
void MainWindow::openXml(const QString &path, const QString &app_name, const bool &fresh,
    const bool &is_settings_dialog)
{
	if (fresh) {
		const bool perform_close = closeIni();
		if (!perform_close)
			return;
		control_panel_->closeSettingsTab();
		control_panel_->clearGuiElements();
		help_loaded_ = false;
	}
	if (!is_settings_dialog)
		current_application_ = app_name;

	if (QFile::exists(path)) {
		setStatus(tr("Reading application XML..."), "info", true);
		refreshStatus();
		XMLReader xml;
		QString xml_error = QString();
		QString autoload_ini( xml.read(path, xml_error) );
		if (!xml_error.isNull()) {
			xml_error.chop(1); //trailing \n
			Error(tr("Errors occured when parsing the XML configuration file"),
			    tr("File: \"") + QDir::toNativeSeparators(path) + "\"", xml_error);
		}
		setStatus("Building GUI...", "info", true);
		buildGui(xml.getXml());
		setStatus("Ready.", "info", false);
		control_panel_->getWorkflowPanel()->buildWorkflowPanel(xml.getXml());
		if (!autoload_ini.isEmpty()) {
			if (QFile::exists(autoload_ini))
				openIni(autoload_ini);
			else
				log(QString("Can not load INI file \"%1\" automatically because it does not exist.").arg(
				    QDir::toNativeSeparators(autoload_ini)), "error");
		}
	} else {
		topLog(tr("An application or simulation file that has previously been found is now missing. Right-click the list to refresh."),
		     "error");
		setStatus(tr("File has been removed"), "error");
		return;
	}

	//run through all INIs that were saved to be autoloaded and check if it's the application we are opening:
	for (auto ini_node = global_xml_settings.firstChildElement().firstChildElement("user").firstChildElement(
	    "autoload").firstChildElement("ini");
	    !ini_node.isNull(); ini_node = ini_node.nextSiblingElement("ini")) {
		if (ini_node.attribute("application").toLower() == app_name.toLower()) {
			autoload_box_->blockSignals(true); //don't re-save the setting we just fetched
			autoload_box_->setCheckState(Qt::Checked);
			autoload_box_->setText(tr("autoload this INI for ") + current_application_);
			autoload_box_->blockSignals(false);
			openIni(ini_node.text(), true); //TODO: delete if non-existent
			break;
		}
	}

	toolbar_clear_gui_->setEnabled(true);
	gui_reset_->setEnabled(true);
	gui_clear_->setEnabled(true);
	if (is_settings_dialog) //no real XML - don't enable XML options
		return;
	toolbar_save_ini_as_->setEnabled(true);
	file_save_ini_as_->setEnabled(true);
	toolbar_open_ini_->setEnabled(true); //toolbar entry
	file_open_ini_->setEnabled(true); //menu entry
	view_preview_->setEnabled(true);
	toolbar_preview_->setEnabled(true);

	auto *file_view = getControlPanel()->getWorkflowPanel()->getFilesystemView();
	file_view->setEnabled(true);
	auto *path_label = file_view->getInfoLabel();
	path_label->setText(file_view->getInfoLabel()->property("path").toString());
	path_label->setWordWrap(true);
	path_label->setStyleSheet("QLabel {font-style: italic}");
	QApplication::alert( this ); //notify the user that the task is finished
}

/**
 * @brief Find the panels that control a certain key/value pair.
 * @details This function includes panels that could be created by dynamic panels (Selector etc.).
 * @param[in] parent The parent panel or window to search, since usually we have some information
 * about where to find it. This function is used when iterating through INIParser elements
 * to load the GUI with values.
 * @param[in] section The section to find.
 * @param[in] keyval The key/value pair to find.
 * @return A list of all panels controlling the key/value pair.
 */
QWidgetList MainWindow::findPanel(QWidget *parent, const Section &section, const KeyValue &keyval)
{
	QWidgetList panels;
	//first, check if there is a panel for it loaded and available:
	panels = findSimplePanel(parent, section, keyval);
	int panel_count = parent->findChildren<Atomic *>().count();
	//if not found, check if one of our Replicators can create it:
	if (panels.isEmpty())
		panels = prepareReplicator(parent, section, keyval);
	//if still not found, check if one of our Selectors can create it:
	if (panels.isEmpty())
		panels = prepareSelector(parent, section, keyval);
	//TODO: For the current MeteoIO XML the order is important here so that a TIME filter
	//does not show up twice (because both the normal and time filter's panel fit the pattern).
	//Cleaning up here would also increase performance.
	if (parent->findChildren<Atomic *>().count() != panel_count) //new elements were created
		return findPanel(parent, section, keyval); //recursion through Replicators that create Selectors that...
	return panels; //no more suitable dynamic panels found
}

/**
 * @brief Find the panels that control a certain key/value pair.
 * @details This function does not include dynamic panels (Selector etc.) and searches only
 * what has already been created.
 * @param[in] parent The parent panel or window to search, because usually we have some information.
 * @param[in] section Section to find.
 * @param[in] keyval Key-value pair to find.
 * @return A list of all panels controlling the key/value pair.
 */
QWidgetList MainWindow::findSimplePanel(QWidget *parent, const Section &section, const KeyValue &keyval)
{
	QString id(section.getName() + Cst::sep + keyval.getKey());
	/* simple, not nested, keys */
	return parent->findChildren<QWidget *>(Atomic::getQtKey(id));
}

/**
 * @brief Find a Selector in our GUI that could handle a given INI key.
 * @details This function looks for Selectors handling keys like "%::COPY". If one is found,
 * it requests the Selector to create a new panel for the key which will then be available
 * to the parent function that is currently looking for a suitable panel for the INI key.
 * @param[in] parent The parent panel or window to search.
 * @param[in] section Section to find.
 * @param[in] keyval Key/value pair to find.
 * @return A list of panels for the INI key including the possibly newly created ones.
 */
QWidgetList MainWindow::prepareSelector(QWidget *parent, const Section &section, const KeyValue &keyval)
{
	QString id(section.getName() + Cst::sep + keyval.getKey());
	//INI key as it would be given in the file, i. e. with the parameters in place instead of the Selector's %:
	const QString regex_selector("^" + section.getName() + Cst::sep + R"(([\w\*\-\.]+)()" + Cst::sep + R"()(\w+?)([0-9]*$))");
	const QRegularExpression rex(regex_selector);
	const QRegularExpressionMatch matches = rex.match(id);

	if (matches.captured(0) == id) { //it could be from a selector with template children
		static const size_t idx_parameter = 1;
		static const size_t idx_keyname = 3;
		static const size_t idx_optional_number = 4;
		const QString parameter(matches.captured(idx_parameter));
		const QString key_name(matches.captured(idx_keyname));
		const QString number(matches.captured(idx_optional_number));
		//this would be the ID given in an XML, i. e. with a "%" staing for a parameter:
		const QString gui_id = section.getName() + Cst::sep + "%" + Cst::sep + key_name + (number.isEmpty()? "" : "#");

		//try to find Selector:
		QList<Selector *> selector_list = parent->findChildren<Selector *>(Atomic::getQtKey(gui_id));
		for (int ii = 0; ii < selector_list.size(); ++ii)
			selector_list.at(ii)->setProperty("ini_value", parameter); //cf. notes in prepareReplicator
		//now all the right child widgets should exist and we can try to find again:
		const QString key_id = section.getName() + Cst::sep + keyval.getKey(); //the one we were originally looking for

		return parent->findChildren<QWidget *>(Atomic::getQtKey(key_id));
	}
	return QWidgetList();
}

/**
 * @brief Find a Replicator in our GUI that could handle a given INI key.
 * @details This function looks for Replicators handling keys like "STATION#". If one is found,
 * it requests the Replicator to create a new panel for the key which will then be available
 * to the parent function that is currently looking for a suitable panel for the INI key.
 * @param[in] parent The parent panel or window to search.
 * @param[in] section Section to find.
 * @param[in] keyval Key/value pair to find.
 * @return A list of panels for the INI key including the possibly newly created ones.
 */
QWidgetList MainWindow::prepareReplicator(QWidget *parent, const Section &section, const KeyValue &keyval)
{
	QString id(section.getName() + Cst::sep + keyval.getKey());
	//the key how it would look in an INI file:
	const QString regex_selector("^" + section.getName() + Cst::sep +
	    R"(([\w\.]+)" + Cst::sep + R"()*(\w*?)(?=\d)(\d*)$)");
	const QRegularExpression rex(regex_selector);
	const QRegularExpressionMatch matches = rex.match(id);

	if (matches.captured(0) == id) { //it's from a replicator with template children
		/*
		 * If we arrive here, we try to find the panel for an INI key that could belong
		 * to the child of a Replicator, e. g. "INPUT::STATION1" or "FILTERS::TA::FILTER1".
		 * We extract the name the Replicator would have (number to "#") and try finding it.
		 */
		static const size_t idx_parameter = 1;
		static const size_t idx_key = 2;
		static const size_t idx_number = 3;
		QString parameter(matches.captured(idx_parameter)); //has trailing "::" if existent
		const QString key(matches.captured(idx_key));
		const QString number(matches.captured(idx_number));
		//the key how it would look in an XML:
		QString gui_id = section.getName() + Cst::sep + parameter + key + "#"; //Replicator's name

		/*
		 * A replicator can't normally be accessed via INI keys, because there is no
		 * standalone XML code for it. It always occurs together with child panels,
		 * and those are the ones that will be sought by the INI parser to set values.
		 * Therefore we can use the "ini_value" property listener for a Replicator to
		 * tell it to replicate its panel, thus creating the necessary child panels.
		 */
		QList<Replicator *> replicator_list = parent-> //try to find replicator:
		    findChildren<Replicator *>(Atomic::getQtKey(gui_id));
		for (int ii = 0; ii < replicator_list.count(); ++ii)
			replicator_list.at(ii)->setProperty("ini_value", number);
		//now all the right child panels should exist and we can try to find again:
		const QString key_id(section.getName() + Cst::sep + keyval.getKey()); //the one we were originally looking for
		return parent->findChildren<QWidget *>(Atomic::getQtKey(key_id));
	} //endif has match

	return QWidgetList(); //no suitable Replicator found
}

/**
 * @brief Build the main window's menu items.
 */
void MainWindow::createMenu()
{
	/* FILE menu */
	QMenu *menu_file = this->menuBar()->addMenu(tr("&File"));
	file_open_ini_ = new QAction(getIcon("document-open"), tr("&Open INI file..."), menu_file);
	menu_file->addAction(file_open_ini_); //note that this does not take ownership
	connect(file_open_ini_, &QAction::triggered, this, [=]{ toolbarClick("open_ini"); });
	file_open_ini_->setShortcut(QKeySequence::Open);
	//file_open_ini_->setMenuRole(QAction::ApplicationSpecificRole);
	file_save_ini_ = new QAction(getIcon("document-save"), tr("&Save INI file"), menu_file);
	file_save_ini_->setShortcut(QKeySequence::Save);
	//file_save_ini_->setMenuRole(QAction::ApplicationSpecificRole);
	menu_file->addAction(file_save_ini_);
	file_save_ini_->setEnabled(false); //enable after an INI is opened
	connect(file_save_ini_, &QAction::triggered, this, [=]{ toolbarClick("save_ini"); });
	file_save_ini_as_ = new QAction(getIcon("document-save-as"), tr("Save INI file &as..."), menu_file);
	file_save_ini_as_->setShortcut(QKeySequence::SaveAs);
	//file_save_ini_as_->setMenuRole(QAction::ApplicationSpecificRole);
	menu_file->addAction(file_save_ini_as_);
	file_save_ini_as_->setEnabled(false);
	connect(file_save_ini_as_, &QAction::triggered, this, [=]{ toolbarClick("save_ini_as"); });
	menu_file->addSeparator();
	auto *file_quit_ = new QAction(getIcon("application-exit"), tr("&Exit"), menu_file);
	file_quit_->setShortcut(QKeySequence::Quit);
	file_quit_->setMenuRole(QAction::QuitRole);
	menu_file->addAction(file_quit_);
	connect(file_quit_, &QAction::triggered, this, &MainWindow::quitProgram);

	/* GUI menu */
	QMenu *menu_gui = this->menuBar()->addMenu(tr("&GUI"));
	gui_reset_ = new QAction(getIcon("document-revert"), tr("&Reset GUI to default values"), menu_gui);
	menu_gui->addAction(gui_reset_);
	gui_reset_->setEnabled(false);
	gui_reset_->setShortcut(Qt::CTRL + Qt::Key_Backspace);
	//gui_reset_->setMenuRole(QAction::ApplicationSpecificRole);
	connect(gui_reset_, &QAction::triggered, this, [=]{ toolbarClick("reset_gui"); });
	gui_clear_ = new QAction(getIcon("edit-delete"), tr("&Clear GUI"), menu_gui);
	menu_gui->addAction(gui_clear_);
	gui_clear_->setEnabled(false);
	gui_clear_->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Backspace);
	//gui_clear_->setMenuRole(QAction::ApplicationSpecificRole);
	connect(gui_clear_, &QAction::triggered, this, [=]{ toolbarClick("clear_gui"); });
	menu_gui->addSeparator();
	gui_close_all_ = new QAction(getIcon("window-close"), tr("Close all content"), menu_gui);
	menu_gui->addAction(gui_close_all_);
	connect(gui_close_all_, &QAction::triggered, this, &MainWindow::resetGui);
	//gui_close_all_->setMenuRole(QAction::ApplicationSpecificRole);

	/* VIEW menu */
	QMenu *menu_view = this->menuBar()->addMenu(tr("&View"));
	view_preview_ = new QAction(getIcon("document-print-preview"), tr("P&review"), menu_view);
	menu_view->addAction(view_preview_);
	view_preview_->setShortcut(QKeySequence::Print);
	view_preview_->setEnabled(false); //enable with loaded application
	//view_preview->setMenuRole(QAction::ApplicationSpecificRole);
	connect(view_preview_, &QAction::triggered, this, &MainWindow::viewPreview);
	auto *view_log = new QAction(getIcon("utilities-system-monitor"), tr("&Log"), menu_view);
	menu_view->addAction(view_log);
	connect(view_log, &QAction::triggered, this, &MainWindow::viewLogger);
	view_log->setShortcut(Qt::CTRL + Qt::Key_L);
	//view_log->setMenuRole(QAction::ApplicationSpecificRole);
	auto *view_refresh = new QAction(tr("&Refresh Applications"), menu_view);
	menu_view->addAction(view_refresh);
	connect(view_refresh, &QAction::triggered, this,
	    [=]{ control_panel_->getWorkflowPanel()->scanFoldersForApps(); });
	view_refresh->setShortcut(QKeySequence::Refresh);

	menu_view->addSeparator();
	auto *view_settings = new QAction(getIcon("preferences-system"), tr("&Settings"), menu_view);
	view_settings->setMenuRole(QAction::PreferencesRole);
	menu_view->addAction(view_settings);
	connect(view_settings, &QAction::triggered, this, &MainWindow::viewSettings);
#if defined Q_OS_MAC //only Mac has this as of now: https://doc.qt.io/archives/qt-4.8/qkeysequence.html
//equivalent at runtime: if (QKeySequence(QKeySequence::Preferences).toString(QKeySequence::NativeText).isEmpty())
	view_settings->setShortcut(QKeySequence::Preferences);
#else
	view_settings->setShortcut(Qt::Key_F3);
#endif
	/* WINDOW menu */
	QMenu *menu_window = this->menuBar()->addMenu(tr("&Window"));
	menu_window->addSeparator();
	auto *window_show_workflow = new QAction(tr("Show wor&kflow"), menu_window);
	menu_window->addAction(window_show_workflow);
	connect(window_show_workflow, &QAction::triggered, this, &MainWindow::showWorkflow);
	window_show_workflow->setShortcut(Qt::CTRL + Qt::Key_K);
	//window_show_workflow->setMenuRole(QAction::ApplicationSpecificRole);
	auto *window_hide_workflow = new QAction(tr("&Hide workflow"), menu_window);
	menu_window->addAction(window_hide_workflow);
	connect(window_hide_workflow, &QAction::triggered, this, &MainWindow::hideWorkflow);
	window_hide_workflow->setShortcut(Qt::CTRL + Qt::Key_H);
	//window_hide_workflow->setMenuRole(QAction::ApplicationSpecificRole);

#ifdef DEBUG
	/* DEBUG menu */
	QMenu *menu_debug = this->menuBar()->addMenu("&Debug");
	auto *debug_run = new QAction("&Run action", menu_debug); //to run arbitrary code with a click
	menu_debug->addAction(debug_run);
	connect(debug_run, &QAction::triggered, this, &MainWindow::z_onDebugRunClick);
#endif //def DEBUG

	/* HELP menu */
#if !defined Q_OS_MAC
	auto *menu_help_main = new QMenuBar(this->menuBar());
	this->menuBar()->setCornerWidget(menu_help_main); //push help menu to the right
#else
	auto *menu_help_main = this->menuBar()->addMenu("&Help");
#endif
	QMenu *menu_help = menu_help_main->addMenu(tr("&Help"));

	auto *help = new QAction(getIcon("help-contents"), tr("&Help"), menu_help_main);
	help->setShortcut(QKeySequence::HelpContents);
	help->setMenuRole(QAction::ApplicationSpecificRole);
	menu_help->addAction(help);
	connect(help, &QAction::triggered, this, [=]{ loadHelp(); });
	auto *help_about = new QAction(getIcon("help-about"), tr("&About"), menu_help_main);
	help_about->setMenuRole(QAction::AboutRole);
	menu_help->addAction(help_about);
	connect(help_about, &QAction::triggered, this, &MainWindow::helpAbout);
	help_about->setShortcut(QKeySequence::WhatsThis);
	menu_help->addSeparator();
	auto *help_dev = new QAction(tr("&Developer's help"), menu_help_main);
	help_dev->setShortcut(Qt::Key_F2);
	help_dev->setMenuRole(QAction::ApplicationSpecificRole);
	menu_help->addAction(help_dev);
	connect(help_dev, &QAction::triggered, this, &MainWindow::loadHelpDev);
	auto *help_bugreport = new QAction(tr("File &bug report..."), menu_help_main);
	help_bugreport->setMenuRole(QAction::ApplicationSpecificRole);
	menu_help->addAction(help_bugreport);
	connect(help_bugreport, &QAction::triggered,
	    []{ QDesktopServices::openUrl(QUrl("https://models.slf.ch/p/inishell-ng/issues/")); });
}

/**
 * @brief Create the main window's toolbar.
 */
void MainWindow::createToolbar()
{
	/* toolbar */
	toolbar_ = new QToolBar("Shortcuts toolbar");
	toolbar_->setContextMenuPolicy(Qt::CustomContextMenu);
	createToolbarContextMenu();
	connect(toolbar_, &QToolBar::customContextMenuRequested, this, &MainWindow::onToolbarContextMenuRequest);
	toolbar_->setMovable(getSetting("user::appearance::fix_toolbar_pos", "value") == "FALSE");
	toolbar_->setFloatable(false);

	bool toolbar_success;
	Qt::ToolBarArea toolbar_area = static_cast<Qt::ToolBarArea>( //restore last toolbar position
	    getSetting("auto::position::toolbar", "position").toInt(&toolbar_success));
	if (!toolbar_success)
		toolbar_area = Qt::ToolBarArea::TopToolBarArea;
	this->addToolBar(toolbar_area, toolbar_);

	/* user interaction buttons */
	toolbar_->setIconSize(QSize(32, 32));
	toolbar_open_ini_ = toolbar_->addAction(getIcon("document-open"), tr("Open INI file"));
	connect(toolbar_open_ini_, &QAction::triggered, this, [=]{ toolbarClick("open_ini"); });
	toolbar_open_ini_->setEnabled(false); //toolbar entry
	file_open_ini_->setEnabled(false); //menu entry
	toolbar_->addSeparator();
	toolbar_save_ini_ = toolbar_->addAction(getIcon("document-save"), tr("Save INI"));
	toolbar_save_ini_->setEnabled(false); //enable when an INI is open
	connect(toolbar_save_ini_, &QAction::triggered, this, [=]{ toolbarClick("save_ini"); });
	toolbar_save_ini_as_ = toolbar_->addAction(getIcon("document-save-as"), tr("Save INI file as"));
	toolbar_save_ini_as_->setEnabled(false);
	connect(toolbar_save_ini_as_, &QAction::triggered, this, [=]{ toolbarClick("save_ini_as"); });
	toolbar_preview_ = toolbar_->addAction(getIcon("document-print-preview"), tr("Preview INI"));
	toolbar_preview_->setEnabled(false);
	connect(toolbar_preview_, &QAction::triggered, this, [=]{ toolbarClick("preview"); });
	toolbar_->addSeparator();
	toolbar_clear_gui_ = toolbar_->addAction(getIcon("document-revert"), tr("Clear INI settings"));
	connect(toolbar_clear_gui_, &QAction::triggered, this, [=]{ toolbarClick("reset_gui"); });
	toolbar_clear_gui_->setEnabled(false);

	/* info label and autoload checkbox */
	QWidget *spacer = new QWidget;
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	QWidget *small_spacer = new QWidget;
	small_spacer->setFixedWidth(25);
	ini_filename_ = new QLabel();
	ini_filename_->setProperty("mouseclick", "open_ini");
	mouse_events_toolbar_ = new MouseEventFilter;
	ini_filename_->installEventFilter(mouse_events_toolbar_);
	ini_filename_->setCursor(QCursor(Qt::PointingHandCursor));
	autoload_box_= new QCheckBox;

	toolbar_->addWidget(spacer);
	toolbar_->addWidget(ini_filename_);
	toolbar_->addWidget(small_spacer);
	connect(autoload_box_, &QCheckBox::stateChanged, this, &MainWindow::onAutoloadCheck);

	autoload_ = toolbar_->addWidget(autoload_box_);
	autoload_->setVisible(false);
	toolbar_->addAction(autoload_);
}

/**
 * @brief Create the main window's status bar.
 */
void MainWindow::createStatusbar()
{ //we use our own because Qt's statusBar is for temporary messages only (they will vanish)
	auto *spacer_widget = new QWidget;
	spacer_widget->setFixedSize(5, 0);
	status_label_ = new QLabel;
	status_label_->setProperty("mouseclick", "open_log");
	mouse_events_statusbar_ = new MouseEventFilter;
	status_label_->installEventFilter(mouse_events_statusbar_);
	status_label_->setCursor(QCursor(Qt::PointingHandCursor));
	status_icon_ = new QLabel;
	statusBar()->addWidget(spacer_widget);
	statusBar()->addWidget(status_label_);
	statusBar()->addPermanentWidget(status_icon_);
}

/**
 * @brief Display a text in the main window's status bar.
 * @details Events such as hovering over a menu may clear the status. It is meant
 * for temporary messages.
 * @param[in] message The text to display.
 * @param[in] time Time to display the text for.
 */
void MainWindow::setStatus(const QString &message, const QString &color, const bool &status_light, const int &time)
{ //only temporary messages are possible via statusBar()->showMessage
	status_label_->setText(message);
	status_label_->setStyleSheet("QLabel {color: " + colors::getQColor(color).name() + "}");
	setStatusLight(status_light);
	status_timer_->stop(); //stop pending clearing
	if (time > 0) {
		status_timer_->setInterval(time); //start new interval
		status_timer_->start();
	}
}

/**
 * @brief Set the status light indicating activity on or off.
 * @param[in] on True if it should be active, false for inactive.
 */
void MainWindow::setStatusLight(const bool &on)
{
	const QPixmap icon((on? ":/icons/active.svg" : ":/icons/inactive.svg"));
	status_icon_->setPixmap(icon.scaled(16, 16));
}

/**
 * @brief Refresh the status label.
 * @details Heavy operations can block the painting which these calls initiate manually.
 */
void MainWindow::refreshStatus()
{
	status_label_->adjustSize();
	status_label_->repaint();
}

/**
 * @brief Clear the main status label.
 */
void MainWindow::clearStatus()
{
	status_label_->setText(QString());
}

/**
 * @brief Event handler for the "File::Quit" menu: quit the main program.
 */
void MainWindow::quitProgram()
{
	QApplication::quit();
}

/**
 * @brief Event handler for the "GUI::resetGui" menu: reset thr GUI completely.
 * @details This is the only action that resets the GUI to the starting point.
 */
void MainWindow::resetGui()
{
	toolbarClick("clear_gui");
	control_panel_->clearGuiElements();
	control_panel_->displayInfo();
	help_loaded_ = false;
	current_application_ = QString();
	this->setWindowTitle(QCoreApplication::applicationName());
	auto *file_view = getControlPanel()->getWorkflowPanel()->getFilesystemView();
	file_view->setEnabled(false);
	auto *path_label = file_view->getInfoLabel();
	path_label->setText(tr("Open an application or simulation before opening INI files."));
	path_label->setWordWrap(true);
	path_label->setStyleSheet("QLabel {color: " + colors::getQColor("important").name() + "}");

	toolbar_open_ini_->setEnabled(false);
	toolbar_save_ini_->setEnabled(false);
	toolbar_save_ini_as_->setEnabled(false);
	toolbar_clear_gui_->setEnabled(false);
	file_open_ini_->setEnabled(false);
	file_save_ini_->setEnabled(false);
	file_save_ini_as_->setEnabled(false);
	gui_reset_->setEnabled(false);
	gui_clear_->setEnabled(false);
	view_preview_->setEnabled(false);
	toolbar_preview_->setEnabled(false);
}

/**
 * @brief Event handler for the "View::Log" menu: open the Logger window.
 */
void MainWindow::viewLogger()
{
	logger_.show(); //the logger is always kept in scope
	logger_.raise(); //bring to front
}

/**
 * @brief Event listener for when the program is closed.
 * @details This function checks whether there are unsaved changes before exiting the program,
 * and if yes, offers the usual options.
 * @param[in] event The received close event.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
	const bool perform_close = closeIni();
	if (perform_close)
		event->accept();
	else
		event->ignore();
}

/**
 * @brief Event listener for key events.
 * @param event The key press event that is received.
 */
void MainWindow::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape) {
		if (help_loaded_) //ESC to get out of the help file
			resetGui();
		if (control_panel_->hasSettingsLoaded()) { //ESC out of settings
			control_panel_->closeSettingsTab();
			if (current_application_.isNull()) { //only settings were loaded
				toolbar_clear_gui_->setEnabled(false);
				gui_reset_->setEnabled(false);
				gui_clear_->setEnabled(false);
			}
		}
	}
}

/**
 * @brief Event handler for the "View::Settings" menu: open the settings tab.
 */
void MainWindow::viewSettings()
{
	if (!control_panel_->hasSettingsLoaded()) {
		openXml(":settings_dialog.xml", "Settings", false, true);
		const int settings_tab_idx = control_panel_->prepareSettingsTab();
		const QList<Atomic *> panel_list( control_panel_->
		    getSectionScrollArea(settings_tab_idx)->findChildren<Atomic *>() );
		for (auto &pan : panel_list) //INIshell settings should not contribute to INI file
			pan->setProperty("no_ini", "true");
		control_panel_->displaySettings(settings_tab_idx);
	} else {
		(void) control_panel_->prepareSettingsTab(); //switch to existent settings tab
	}
}

/**
 * @brief Show the workflow panel in case it is collapsed.
 */
void MainWindow::showWorkflow()
{
	control_panel_->setSplitterSizes(QList<int>( ));
	const QList<int> sizes( control_panel_->getSplitterSizes());
	if (sizes.at(0) < 20) //Workflow was dragged to 0 before collapsing - make visible again
		//the splitter has size constraints so we can just set something big:
		control_panel_->setSplitterSizes(QList<int>( {this->width() / 2, this->width() / 2} ));
}

/**
 * @brief Hide the workflow panel.
 */
void MainWindow::hideWorkflow()
{
	setSplitterSizeSettings();
	control_panel_->setSplitterSizes(QList<int>( {0, this->width()} ));
}

/**
 * @brief Load the developer help XML from the resources onto the GUI.
 */
void MainWindow::loadHelpDev()
{
	clearGui();
	openXml(":doc/help_dev.xml", "Help");
	help_loaded_ = true;
}

/**
 * @brief Display the 'About' window.
 */
void MainWindow::helpAbout()
{
	static auto *about = new AboutWindow(this);
	about->show();
	about->raise();
}

/**
 * @brief Delegate the toolbar button clicks to the appropriate calls.
 * @param[in] function Tag name for the function to perform.
 */
void MainWindow::toolbarClick(const QString &function)
{
	if (function == "save_ini")
		saveIni();
	else if (function =="save_ini_as")
		saveIniAs();
	else if (function == "open_ini")
		openIni();
	else if (function == "reset_gui") //reset to default
		clearGui();
	else if (function == "clear_gui") //reset to nothing
		clearGui(false); //not actually a toolbar button but a menu
	else if (function == "preview")
		viewPreview();
}

/**
 * @brief Event listener for when the 'autoload' checkbox is clicked.
 * @details This function sets settings for the INI files to be automatically loaded for certain XMLs.
 * They are saved when the program quits.
 * @param[in] state The checkbox state (checked/unchecked).
 */
void MainWindow::onAutoloadCheck(const int &state)
{
	//Run through <ini> tags in appropriate settings file section and look for the current
	//application. If it is found, when the checkbox is checked it will be set to the current
	//INI file. If the box is unchecked the entry is deleted:
	QDomNode autoload_node( global_xml_settings.firstChildElement().firstChildElement("user").firstChildElement("autoload") );
	for (auto ini = autoload_node.firstChildElement("ini"); !ini.isNull(); ini = ini.nextSiblingElement("ini")) {
		if (ini.attribute("application").toLower() == current_application_.toLower()) {
			if (state == Qt::Checked)
				ini.firstChild().setNodeValue(ini_filename_->text());
			else
				ini.parentNode().removeChild(ini);
			return;
		}
	}

	//not found - create new settings node if user is enabling:
	if (state == Qt::Checked) {
		QDomNode ini( autoload_node.appendChild(global_xml_settings.createElement("ini")) );
		ini.toElement().setAttribute("application", current_application_);
		ini.appendChild(global_xml_settings.createTextNode(ini_filename_->text()));
	}
}

/**
 * @brief Event listener for requests to show the context menu for our toolbar.
 */
void MainWindow::onToolbarContextMenuRequest(const QPoint &/*pos*/)
{
	const QAction *selected( toolbar_context_menu_.exec(QCursor::pos()) ); //show at cursor position
	if (selected) { //an item of the menu was clicked
		if (selected->text() == (tr("Fix toolbar position"))) {
			toolbar_->setMovable(!selected->isChecked());
			setSetting("user::appearance::fix_toolbar_pos", "value",
			    selected->isChecked()? "TRUE" : "FALSE");
		}
	}
}

/**
 * @brief Helper function to write file info only once if something goes wrong.
 * @param[in] first_error_message Boolean that is set to true for the current run
 * until this function is called the first time.
 * @param[in] ini INIParser that is currently processing
 */
void MainWindow::writeGuiFromIniHeader(bool &first_error_message, const INIParser &ini)
{
	if (first_error_message) {
		logger_.log(tr(R"(Loading INI file "%1" into %2...)").arg(
		    ini.getFilename(), current_application_));
		first_error_message = false;
	}
}

#ifdef DEBUG
/**
 * @brief This is a dummy menu entry to execute arbitrary test code from.
 */
void MainWindow::z_onDebugRunClick()
{
	setStatus("Debug menu clicked", "warning", false, 5000);
}
#endif //def DEBUG
