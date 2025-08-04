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

#include "MainPanel.h"
#include "src/main/colors.h"
#include "src/main/constants.h"
#include "src/main/inishell.h"
#include "src/main/os.h"
#include "src/main/settings.h"
#include "src/gui_elements/gui_elements.h"
#include "src/gui/WorkflowPanel.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#ifdef DEBUG
	#include <iostream>
#endif

/**
 * @class ScrollPanel
 * @brief The main scroll panel controlling all dynamically built panels.
 * @param[in] section The section this panel corresponds to. It could be used for styling but is
 * unused otherwise.
 * @param[in] parent The parent widget (the main tab bar).
 */
ScrollPanel::ScrollPanel(const QString &section, const QString &tab_color, QWidget *parent)
    : Atomic(section, "_main_scroll_panel", parent)
{
	/* create a scroll area and put a Group in it */
	main_area_ = new QScrollArea;
	setPrimaryWidget(main_area_);
	main_area_->setWidgetResizable(true); //best to let the manager do its job - troubles ahead if we don't
	main_area_->setStyleSheet("QScrollArea {border: none}"); //we have one from the tabs already
	main_group_ = new Group(QString(), QString(), false, false, false, false, QString(), QString(), tab_color);
	main_area_->setWidget(main_group_);

	/* main layout */
	auto *layout( new QVBoxLayout );
	layout->addWidget(main_area_);
	this->setLayout(layout);
}

/**
 * @brief Retrieve the main grouping element of a scroll panel (there is one per tab).
 * @return The main Group of this scroll panel holding all widgets.
 */
Group * ScrollPanel::getGroup() const
{
	return this->main_group_;
}

/**
 * @class MainPanel
 * @brief Constructor for the main panel, i. e. the tab widget with scroll area children.
 * @param[in] parent The parent widget (the main window).
 */
MainPanel::MainPanel(QWidget *parent) : QWidget(parent)
{
	/* widget(s) on the left side and main tab bar */
	workflow_panel_ = new WorkflowPanel;
	workflow_stack_ = new QStackedWidget;
	section_tab_ = new QTabWidget;
	section_tab_->setTabsClosable(true);
	connect(section_tab_, &QTabWidget::tabCloseRequested, this, &MainPanel::onTabCloseRequest);
	workflow_stack_->addWidget(section_tab_);

	/* main layout */
	auto *main_layout( new QHBoxLayout );
	splitter_ = new QSplitter;
	splitter_->addWidget(workflow_panel_); //allow to resize with mouse
	splitter_->addWidget(workflow_stack_);
	setSplitterSizes();
	main_layout->addWidget(splitter_);
	this->setLayout(main_layout);

	displayInfo(); //startup info text
}

/**
 * @brief Retrieve the ScrollPanel of a section.
 * @details Sections are grouped in tab bar elements. For this, they are put in that specific
 * tab panel's ScrollPanel which can be retrieved by this function (to build top level panels into).
 * If the section/tab does not exist yet it is created.
 * @param[in] section The INI section for which to get the corresponding ScrollPanel.
 * @param[in] background_color Background color to give to a whole section that is being created.
 * @param[in] color Font color to give a new section.
 * @param[in] no_create Set to true when parsing an INI to "force" INI sections matching the XML.
 * @return The found or newly created ScrollPanel.
 */
ScrollPanel * MainPanel::getSectionScrollarea(const QString &section, const QString &background_color,
    const QString &color, const bool&no_create)
{ //find or create section tab
	for (int ii = 0; ii < section_tab_->count(); ++ii) {
		//remove system-dependent keyboard shortcuts markers before comparing:
		//HACK this is needed as a workaround for KDE bug https://bugs.kde.org/show_bug.cgi?id=337491
#if defined Q_OS_LINUX || defined Q_OS_FREEBSD //we assume the kde is only used on Linux and FreeBSD
		if (QString::compare(section_tab_->tabBar()->tabText(ii).replace("&", ""), section, Qt::CaseInsensitive) == 0)
			return qobject_cast<ScrollPanel *>(section_tab_->widget(ii)); //cast to access members
#else
		if (QString::compare(section_tab_->tabBar()->tabText(ii), section, Qt::CaseInsensitive) == 0)
			return qobject_cast<ScrollPanel *>(section_tab_->widget(ii)); //cast to access members
#endif
	}
	if (!no_create) {
		auto *new_scroll( new ScrollPanel(section, background_color) );
		section_tab_->addTab(new_scroll, section);
		section_tab_->tabBar()->setTabTextColor(section_tab_->count() - 1, colors::getQColor(color));

		if (section != "Settings") { //HACK: use a flag for this
			//remove tab close buttons:
			section_tab_->tabBar()->tabButton(section_tab_->count() - 1, QTabBar::RightSide)->deleteLater();
			section_tab_->tabBar()->setTabButton(section_tab_->count() - 1, QTabBar::RightSide, nullptr);
		}

		return new_scroll;
	} else { //don't add sections for keys found in ini files
		return nullptr;
	}
}

/**
 * @brief Retrieve a section's ScrollPanel with known section tab index.
 * @param[in] index The section's index in the tab bar.
 * @return ScrollPanel for the section.
 */
ScrollPanel * MainPanel::getSectionScrollArea(const int &index)
{
	return qobject_cast<ScrollPanel *>(section_tab_->widget(index));
}

/**
 * @brief Run through panels and query their user-set values.
 * @details This function finds all panel classes, gets their set values, and modifies the
 * corresponding INI setting. It also performs checks for missing values.
 * Note: Originally INI values were updated live in the panels on input, eliminating the need
 * for this loop. However, more and more intermediate loops needed to be introduced (for example
 * to deactivate panels that are hidden by a parent element like a Dropdown or Choice).
 * Furthermore, dealing with missing mandatory values and keys that are present multiple times
 * (e. g. needing to check if a second panel manipulates the same key before deleting it in a
 * Selector) convoluted the program so heavily that this solution is much cleaner and more robust.
 * @param[in] ini The INIParser for which to set the values (the one that will be output).
 * @return A comma-separated list of missing mandatory INI keys.
 */
QString MainPanel::setIniValuesFromGui(INIParser *ini)
{
	//TODO: For some combination of properties (optional, will be defaulted by the
	//software, ...) we may be able to skip output of some keys to avoid
	//information redundancy. Parameter checks:
	//const QString default_value( panel->property("default_value").toString() ); //default value
	//( !ini->get(section, key).isNull()) ) //key present in input INI file
	//( is_mandatory ) //key is set non-optional

	QString missing;
	for (int ii = 0; ii < section_tab_->count(); ++ii) { //run through sections
		const QWidget *section_tab( section_tab_->widget(ii) );
		const QList<Atomic *> panel_list( section_tab->findChildren<Atomic *>() );
		for (auto &panel : panel_list) { //run through panels in section
			if (!panel->isVisibleTo(section_tab)) //visible if section_tab was visible?
				continue;
			QString section, key;
			const QString value( panel->getIniValue(section, key) );
			if (panel->property("no_ini").toBool())
				continue;
			if (key.isNull()) //GUI element that is not an interactive panel
				continue;

			const bool is_mandatory = panel->property("is_mandatory").toBool();
			if (is_mandatory && value.isEmpty()) //collect missing non-optional keys
				missing += key + ", ";
			if (!value.isEmpty())
				ini->set(section, key, value, is_mandatory);
		} //endfor panel_list
	} //endfor ii
	missing.chop(2); //trailing ", "
	return missing;
}

/**
 * @brief Display some info and the SLF logo on program start.
 */
void MainPanel::displayInfo()
{
	static const QString message(tr("<br> \
	    &nbsp;&nbsp;Welcome to <b>INIshell</b>, a dynamic graphical user interface builder to manage INI files.<br> \
	    &nbsp;&nbsp;Double-click an application to the left to begin.<br><br> \
	    &nbsp;&nbsp;For help, click \"Help\" in the menu and visit <a href=\"https://models.slf.ch/p/inishell-ng/\">INIshell's project page</a>.<br> \
	    &nbsp;&nbsp;There, you will also find the well-documented <a href=\"https://models.slf.ch/p/inishell-ng/source/tree/master/\">source code</a>.<br> \
	    &nbsp;&nbsp;If you don't know where to begin, press %1. \
	").arg(os::getHelpSequence()));

	auto *info_scroll( new QScrollArea );
	info_scroll->setWidgetResizable(true);

	/* SLF logo */
	QLabel *info_label( new QLabel );
	info_label->setAlignment(Qt::AlignBottom | Qt::AlignRight);
	info_label->setStyleSheet("QLabel {background-color: " + colors::getQColor("app_bg").name() + "}");
	const QPixmap logo(":/icons/slf_desaturated.svg");
	info_label->setPixmap(logo);

	/* info text */
	QLabel *info_text( new QLabel(info_label) );
	info_text->setTextFormat(Qt::RichText);
	info_text->setTextInteractionFlags(Qt::TextBrowserInteraction);
	info_text->setOpenExternalLinks(true);
	info_text->setText(message);
	info_scroll->setWidget(info_label);

	section_tab_->addTab(info_scroll, "Info");
	//remove close button:
	section_tab_->tabBar()->tabButton(section_tab_->count() - 1, QTabBar::RightSide)->deleteLater();
	section_tab_->tabBar()->setTabButton(section_tab_->count() - 1, QTabBar::RightSide, nullptr);
}

/**
 * @brief Query the splitter's current sizes.
 * @return Current sizes of the splitter's panels.
 */
QList<int> MainPanel::getSplitterSizes() const
{
	return splitter_->sizes();
}

/**
 * @brief Set sizes of the panels the splitter manages.
 * @param state The sizes the splitter should give to its panels.
 */
void MainPanel::setSplitterSizes(QList<int> sizes)
{
	splitter_->setStretchFactor(0, Cst::proportion_workflow_horizontal_percent);
	splitter_->setStretchFactor(1, 100 - Cst::proportion_workflow_horizontal_percent);
	if (!sizes.isEmpty()) {
		splitter_->setSizes(sizes);
		return;
	}
	bool workflow_success, main_success;
	const int width_workflow = getSetting("auto::sizes::splitter_workflow", "size").toInt(&workflow_success);
	const int width_mainpanel = getSetting("auto::sizes::splitter_mainpanel", "size").toInt(&main_success);
	if (workflow_success && main_success)
		splitter_->setSizes(sizes << width_workflow << width_mainpanel);
}

/**
 * @brief Remove all GUI elements.
 * @details This function clears the main GUI itself, i. e. all panels read from XML, including
 * the workflow panels.
 */
void MainPanel::clearGuiElements()
{
	section_tab_->clear();
	workflow_panel_->clearXmlPanels();
}

/**
 * @brief Reset the whole GUI to default values.
 */
void MainPanel::clearGui(const bool &set_default)
{
	clearDynamicPanels<Replicator>(); //clear special panels (the ones that can produce INI keys)
	clearDynamicPanels<Selector>(); //cf. notes there
	const QList<Atomic *> panel_list( section_tab_->findChildren<Atomic *>() ); //clear all others
	for (auto &panel : panel_list)
		panel->clear(set_default);
}

/**
 * @brief Prepare the GUI after settings window has been opened.
 * @details The XML describing INIshell's settings page is loaded by the main window, then this
 * function is called to do some work with it.
 * @return Index of the settings tab. It is propragated so the application XMLs could have an INI
 * section with the same name.
 */
int MainPanel::prepareSettingsTab()
{
	if (settings_tab_idx_ == -1) {
		settings_tab_idx_ = section_tab_->count() - 1;
		createExtraSettingsWidgets(); //'save' button and info
	}
	section_tab_->setCurrentIndex(settings_tab_idx_);
	return settings_tab_idx_;
}

/**
 * @brief Close INIshell's settings page.
 */
void MainPanel::closeSettingsTab()
{
	if (settings_tab_idx_ != -1) {
		section_tab_->removeTab(settings_tab_idx_);
		settings_tab_idx_ = -1;
	}
}

/**
 * @brief Display INIshell's settings (XML file) in the Settings page.
 * @param[in] settings_tab_idx Tab index of the Settings page.
 */
void MainPanel::displaySettings(const int &settings_tab_idx)
{
	const QStringList settings_list(getSimpleSettingsNames());
	for (auto &set : settings_list) {
		const QString string_value( getSetting(set, "value") );
		auto *panel( getSectionScrollArea(settings_tab_idx)->findChild<Atomic *>(
		    Atomic::getQtKey("SETTINGS::" + set)) );
		if (panel) //no crash on XML errors
			panel->setProperty("ini_value", string_value);
	}
	const QStringList search_dirs( getListSetting("user::xmlpaths", "path") );
	for (int ii = 1; ii <= search_dirs.size(); ++ii) {
		auto *xml_panel = getSectionScrollArea(settings_tab_idx)->findChild<Replicator *>(Atomic::getQtKey("SETTINGS::user::xmlpaths::path#"));
		xml_panel->setProperty("ini_value", ii);
		auto *path_panel = getSectionScrollArea(settings_tab_idx)->findChild<Atomic *>(
		    Atomic::getQtKey(QString("SETTINGS::user::xmlpaths::path%1").arg(ii)));
		path_panel->setProperty("ini_value", search_dirs.at(ii-1));
	}
}

/**
 * @brief Select tab specified by its caption.
 * @param[in] tab_name The tab/section to select.
 * @return True if the tab could be selected.
 */
bool MainPanel::showTab(const QString &tab_name)
{
	for (int ii = 0; ii < section_tab_->count(); ++ii) {
//remove system-dependent keyboard shortcuts markers before comparing:
//HACK this is needed as a workaround for KDE bug https://bugs.kde.org/show_bug.cgi?id=337491
#if defined Q_OS_LINUX || defined Q_OS_FREEBSD //we assume the kde is only used on Linux and FreeBSD
		if (QString::compare(section_tab_->tabBar()->tabText(ii).replace("&", ""),
			    tab_name, Qt::CaseInsensitive) == 0) {
			section_tab_->setCurrentIndex(ii);
			return true;
		}
#else
		if (QString::compare(section_tab_->tabBar()->tabText(ii), tab_name,
		    Qt::CaseInsensitive) == 0) {
			section_tab_->setCurrentIndex(ii);
			return true;
		}
#endif
	} //endfor ii
	return false; //not found
}

/**
 * @brief Save INIshell settings from the Settings page to the file system.
 * @details This function is called by a lambda from the save button.
 * @param[in] settings_tab_idx Index of the Settings page tab.
 */
void MainPanel::saveSettings(const int &settings_tab_idx)
{
	QWidget *section_tab( section_tab_->widget(settings_tab_idx) );
	const QStringList settings_list(getSimpleSettingsNames());
	for (auto &option : settings_list) {
		const QString value( getShellSetting(section_tab, option) );
		//there are user settings not in the settings page (e. g. "fix toolbar position"), so don't overwrite:
		if (!value.isEmpty())
			setSetting(option, "value", value);
	}

	QStringList search_dirs;
	const QList<Atomic *>panel_list( section_tab->findChildren<Atomic *>() );
	for (auto &pan : panel_list) {
		QString section, key;
		const QString value( pan->getIniValue(section, key) );
		if (key.startsWith("user::xmlpaths::path") && !value.isEmpty())
			search_dirs.push_back(value);
	}
	setListSetting("user::xmlpaths", "path", search_dirs);
	getMainWindow()->getControlPanel()->getWorkflowPanel()->scanFoldersForApps(); //auto-refresh apps
}

/**
 * @brief Close a tab that has the close button enabled.
 * @param[in] idx Tab close index.
 */
void MainPanel::onTabCloseRequest(const int &idx)
{
	if (idx == settings_tab_idx_)
		getMainWindow()->closeSettings();
}

/**
 * @brief Helper function to get a Settings page panel's value.
 * @param[in] parent The parent widget to search.
 * @param[in] option The setting name to retrieve.
 * @return The setting's current value in the GUI.
 */
QString MainPanel::getShellSetting(QWidget *parent, const QString &option)
{
	const Atomic *option_element( parent->findChild<Atomic *>(
	    Atomic::getQtKey("SETTINGS::" + option)) );
	if (option_element) {
		QString section, key; //unused here
		return option_element->getIniValue(section, key);
	}
	return QString();
}

/**
 * @brief Helper function to create additional elements for the Settings page.
 */
void MainPanel::createExtraSettingsWidgets()
{
	/* add a 'save' button to the settings page */
	Group *settings_group( qobject_cast<ScrollPanel *>(section_tab_->widget(settings_tab_idx_))->getGroup() );

	auto extra_elements( new QGroupBox(settings_group) );
	extra_elements->setObjectName("_settings_group_box_");
	extra_elements->setStyleSheet("QGroupBox {border: none; background-color: " + colors::getQColor("app_bg").name() + "}");
	auto *extra_layout( new QVBoxLayout );
	auto *save_button( new QPushButton(tr("Click to save settings"), settings_group) );
	connect(save_button, &QPushButton::clicked, this, [=]{ saveSettings(settings_tab_idx_); });
	save_button->setStyleSheet("QPushButton {font-weight: bold}");
	auto *settings_location = new QLabel(
	    tr(R"(The settings file is located at path <i>"%1"</i> and can be deleted at any time, but will be re-created when running <i>INIshell</i>.)").arg(
	    getSettingsFileName()));
	settings_location->setStyleSheet("QLabel {color: " + colors::getQColor("helptext").name() + "}");
	settings_location->setWordWrap(true);
	settings_location->setAlignment(Qt::AlignCenter);

	extra_layout->addWidget(save_button);
	extra_layout->addWidget(settings_location);
	extra_elements->setLayout(extra_layout);
	settings_group->addWidget(extra_elements);
}

/**
 * @brief Clear panels which can add/remove children at will.
 * @details This function clears panels that have the ability to create and more importantly delete
 * an arbitrary number of child panels. Those stand for a group of INI keys rather than a single one,
 * and can create child panels for INI keys such as "STATION1, STATION2, ...".
 */
template <class T>
void MainPanel::clearDynamicPanels()
{
	/*
	 * Fetching a list of suitable panels and iterating through them does not work out
	 * because clearing one dynamic panel can delete another one, which means the pointers
	 * we receive could be invalidated.
	 * So, we look for the first dynamic panel with children, clear that, and start the
	 * search again until there are no more dynamic panels with children found.
	 */
	const QList<T *> panel_list( section_tab_->findChildren<T *>() );
	for (auto &pan : panel_list) {
		//the panels themselves are not deleted, so we stop if it's empty:
		if (pan->count() > 0) {
			pan->clear();
			clearDynamicPanels<T>(); //repeat until all dynamic panels are empty
			return;
		}
	}
}
//this is a list of panels that can produce and delete an aribtrary number of child panels:
template void MainPanel::clearDynamicPanels<Selector>();
template void MainPanel::clearDynamicPanels<Replicator>();
