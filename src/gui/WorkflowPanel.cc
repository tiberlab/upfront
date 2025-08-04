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

#include "WorkflowPanel.h"
#include "src/gui/PathView.h"
#include "src/gui_elements/Atomic.h" //for getQtKey()
#include "src/main/colors.h"
#include "src/main/common.h"
#include "src/main/constants.h"
#include "src/main/inishell.h"
#include "src/main/os.h"
#include "src/main/settings.h"

#include <QtGlobal>
#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDir>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QRegularExpression>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>

#ifdef DEBUG
	#include <iostream>
#endif

/**
 * @class WorkflowPanel
 * @brief The WorkflowPanel's default constructor.
 * @details This constructor creates the static part of the workflow portion of the GUI, i. e.
 * panels that are always present and not loaded through XML.
 * @param parent
 */
WorkflowPanel::WorkflowPanel(QWidget *parent) : QWidget(parent)
{
	/* main toolbox */
	this->setMaximumWidth(Cst::width_workflow_max); //to stop the splitter from making it huge
	workflow_container_ = new QToolBox;
	connect(workflow_container_, &QToolBox::currentChanged, this, &WorkflowPanel::toolboxClicked);

	//add a list view for applications, one for simulations, and one for INI files:
	applications_ = new ApplicationsView(tr("Applications"));
	simulations_ = new ApplicationsView(tr("Simulations"));
	filesystem_ = new IniFolderView;
	auto *path_label = filesystem_->getInfoLabel();
	path_label->setText(tr("Open an application or simulation before opening INI files."));
	path_label->setWordWrap(true);
	path_label->setStyleSheet("QLabel {color: " + colors::getQColor("important").name() + "}");

	workflow_container_->addItem(applications_, tr("Applications"));
	//: Translation hint: This is for the workflow panel on the left side.
	workflow_container_->addItem(simulations_, tr("Simulations"));
	workflow_container_->addItem(filesystem_, tr("INI files"));

	/* main layout */
	auto *layout = new QVBoxLayout;
	layout->addWidget(workflow_container_);
	this->setLayout(layout);

	scanFoldersForApps(); //perform the search for XMLs that are an application or simulation
}

/**
 * @brief Build the dynamic part of the workflow panel.
 * @details This function reads the "workflow" part of an XML which is therefore user setable.
 * It is much like the dynamic GUI building of the main portion, but we keep it completely separate
 * from that code for stability / clarity reasons, and because the demands are a bit different.
 * @param[in] xml The XML document to parse for a "<workflow>" section.
 */
void WorkflowPanel::buildWorkflowPanel(const QDomDocument &xml)
{
	for (QDomNode workroot = xml.firstChildElement().firstChildElement("workflow");
	    !workroot.isNull(); workroot = workroot.nextSiblingElement("workflow")) {
		for (QDomElement work = workroot.toElement().firstChildElement("section"); !work.isNull();
		    work = work.nextSiblingElement("section")) {
			buildWorkflowSection(work); //one tab in the workflow tool bar
		}
	}
	int counter = 0; //yet another variation of how we must handle coloring...
	QList<QWidget *> widget_list = workflow_container_->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
	for (auto *wid : widget_list) {
		if (wid->metaObject()->className() == QString("QToolBoxButton")) {
			counter++;
			if (counter > 3) //color dynamic panels different to the static (always present) ones
				wid->setStyleSheet("* {color: " + colors::getQColor("special").name() + "}");
		}
	}
}

/**
 * @brief Iterate through a number of folders and look for applicable XML files.
 */
void WorkflowPanel::scanFoldersForApps()
{
	topStatus(tr("Scanning for applications and simulations..."));
	bool applications_found, simulations_found;
	readAppsFromDirs(applications_found, simulations_found); //populate the applications and simulations lists
	topStatus(tr("Done scanning, ") +
	    ((applications_found || simulations_found)? tr("items") : tr("nothing")) + tr(" found."));
	if (!applications_found)
		applications_->addInfoSeparator(tr(
		    "No applications found. Please check the help section \"Applicatons\" to obtain the necessary files."), 0);
	if (!simulations_found) //no files containing "<inishell_config simulation=..." found
		simulations_->addInfoSeparator(tr(
		    "No simulations found. Please check the help section \"Simulations\" to set up your simulations."), 0);
}

/**
 * @brief Clear the dynamic part of the workflow panel, i. e. the part that is read from XML.
 */
void WorkflowPanel::clearXmlPanels()
{
	for (int ii = 0; ii < workflow_container_->count(); ++ii) {
		if (workflow_container_->widget(ii)->property("from_xml").toBool())
			workflow_container_->widget(ii)->deleteLater();
	}
}

/**
 * @brief Parse an XML node and build a workflow tab from it.
 * @param[in] section XML node containing the section with all its options
 */
void WorkflowPanel::buildWorkflowSection(QDomElement &section)
{
	/*
	 * This function is the pendant to our element factory for the main part of the GUI.
	 * It populates the workflow panel with elements read from XML that can control simulations
	 * and generally perform actions on the operating system.
	 */
	auto *workflow_frame = new QFrame;
	auto *workflow_layout = new QVBoxLayout;
	const QString caption( section.attribute("caption") );
	for (QDomElement el = section.firstChildElement("element"); !el.isNull(); el = el.nextSiblingElement("element")) {
		QWidget *section_item = workflowElementFactory(el, caption); //construct the element to insert
		workflow_frame->setProperty("from_xml", true); //to be able to delete all dynamic ones
		if (section_item != nullptr) {
			//for once setting the parent is important (to check which terminal view is associated with a button):
			section_item->setParent(workflow_frame);
			workflow_layout->addWidget(section_item, 0, Qt::AlignTop);
			//if there is a system command associated to this element we remember to show a TerminalView for the tab:
			if (section_item->property("action").toString() == "terminal")
				workflow_frame->setProperty("action", "terminal");
		} else {
			topLog(tr(R"(Workflow element "%1" unknown)").arg(el.attribute("type")), "error");
		}
	}

	//only sections that contain a command line action receive their individual TerminalView,
	//simpler panels (e. g. only opening an URL) will show the main GUI:
	if (workflow_frame->property("action").toString() == "terminal") {
		auto *new_terminal = new TerminalView;
		workflow_frame->setProperty("stack_index", //connect the tab index to the terminal view index
		    getMainWindow()->getControlPanel()->getWorkflowStack()->count());
		getMainWindow()->getControlPanel()->getWorkflowStack()->addWidget(new_terminal);

		/* create working directory choice */
		auto *cwd_label = new QLabel(tr("Set working directory from:"));
		cwd_label->setWordWrap(true);
		workflow_layout->addWidget(cwd_label, 0, Qt::AlignBottom);
		auto *panel_working_directory = new QComboBox;
		panel_working_directory->setObjectName("_working_directory_");
		panel_working_directory->addItem( "{inifile}" );
		panel_working_directory->addItem( "{inifile}/../" );
		panel_working_directory->addItem( QDir::currentPath() );
		panel_working_directory->setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLength );
		panel_working_directory->setEditable(true);

		auto help_button = new QToolButton;
		help_button->setAutoRaise(true);
		help_button->setIcon(getIcon("help-contents"));
		connect(help_button, &QToolButton::clicked, this,
		    [=]{ getMainWindow()->loadHelp("Example Workflow", "help-workingdir"); });

		auto help_layout = new QHBoxLayout;
		help_layout->addWidget(panel_working_directory);
		help_layout->addWidget(help_button);
		workflow_layout->addLayout(help_layout);

		//remember the selection for next one that is loaded:
		panel_working_directory->setCurrentIndex(getSetting("user::working_dir", "value").toInt());
		connect(panel_working_directory, QOverload<const int>::of(&QComboBox::currentIndexChanged),
		    [=](int index){ setSetting("user::working_dir", "value", QString::number(index)); });
	}
	auto *spacer = new QSpacerItem(-1, -1, QSizePolicy::Expanding, QSizePolicy::Expanding);
	workflow_layout->addSpacerItem(spacer); //keep widgets to the top

	/*
	 * Create a separate status label because otherwise we would overwrite status messages about
	 * processes. This label is used to display errors / warnings about the workflow elements,
	 * for example if the INI file is not saved or a key is not found.
	 * This way the main status can still do its work and e. g. show that a process has terminated.
	 * TODO: Longer workflows may hide this label until scrolled down -> display or rotate in main status bar?
	*/
	QLabel *panel_status_label = new QLabel(workflow_frame);
	panel_status_label->setWordWrap(true);
	panel_status_label->setObjectName("_status_label_");
	panel_status_label->setStyleSheet("QLabel {color: " + colors::getQColor("error").name() + "}");
	workflow_layout->addWidget(panel_status_label, 0, Qt::AlignBottom);

	/* main layout */
	workflow_frame->setLayout(workflow_layout);
	workflow_container_->addItem(workflow_frame, caption);
}

/**
 * @brief Element factory routine for the workflow panel elements.
 * @param[in] item XML node containing the element with its options.
 * @return The constructed element.
 */
QWidget * WorkflowPanel::workflowElementFactory(QDomElement &item, const QString& appname)
{
	QWidget *element = nullptr;
	const QString identifier( item.attribute("type") );
	const QString id( item.attribute("id") ); //elements can be referred to by the user via their IDs
	const QString caption( item.attribute("caption") );

	if (identifier == "datetime") { //a date / time picker
		QDateTime default_date(QDateTime::fromString(item.attribute("default"), Qt::ISODate));
		if (!default_date.isValid()) {
			default_date = QDateTime::currentDateTime();
			default_date.setTime( QTime() ); //round to start of current day
		}
		auto *tmp = new QDateTimeEdit(default_date);
		tmp->setCalendarPopup(true);
		tmp->setToolTip(tr("Pick a date/time"));
		tmp->setDisplayFormat("yyyy-MM-ddThh:mm:ss");
		element = tmp;
	} else if (identifier == "checkbox") { //a checkbox to get a boolean
		element = new QCheckBox(caption);
	} else if (identifier == "button") { //a simple push button that can execute multiple commands
		element = new QPushButton(caption);
		element->setProperty("caption", caption); //remember the caption for later
		static const QString regex_openurl(R"((openurl|setpath)\((.*)\))");
		static const QRegularExpression rex(regex_openurl);
		QString commands;
		for (QDomElement cmd = item.firstChildElement("command"); !cmd.isNull();
		    cmd = cmd.nextSiblingElement("command")) {
			commands += cmd.text() + "\n";
			const QRegularExpressionMatch url_match = rex.match(cmd.text());
			if (!url_match.hasMatch())
				element->setProperty("action", "terminal");
		}
		commands.chop(1); //remove trailing "\n"
		const QStringList action_list( commands.split("\n") );
		if (!(action_list.size() == 1 && action_list.at(0).isEmpty())) {
			connect(static_cast<QPushButton *>(element), &QPushButton::clicked, this,
			    [=] { buttonClicked(static_cast<QPushButton *>(element), action_list, appname); });
			element->setToolTip(commands);
		} else {
			topLog(tr(R"(No command given for button "%1" (ID: "%2"))").arg(caption, id), "error");
			element->setToolTip(tr("No command"));
		}
	} else if (identifier == "label") { //info display element that can be styled with HTML
		element = new QLabel(caption);
		element->setStyleSheet("QLabel {color: " + colors::getQColor("normal").name() + "}");
		static_cast<QLabel *>(element)->setWordWrap(true); //multi-line
	} else if (identifier == "text") { //plain text input
		element = new QLineEdit;
		static_cast<QLineEdit *>(element)->setText(item.attribute("default"));
	} else if (identifier == "path") { //listing of a single folder on the file system
		element = new PathView;
		if (!item.attribute("path").isNull())
			static_cast<PathView *>(element)->setPath(item.attribute("path"));
	}
	if (element) //give the element a unique internal ID
		element->setObjectName("_workflow_" + Atomic::getQtKey(id));
	return element;
}

/**
 * @brief Scan a list of directories for suitable XML files to load.
 * @details This function runs through files of numerous directories and looks for the
 * "<inishell_config...>" header. It discerns between applications and simulations and populates
 * the corresponding list view in the workflow panel.
 * @param[out] applications_found True if at least one application was found.
 * @param[out] simulations_found True if at least one simulation was found.
 */
void WorkflowPanel::readAppsFromDirs(bool &applications_found, bool &simulations_found)
{
	static const QStringList filters = {"*.xml", "*.XML"};
	applications_->clear();
	simulations_->clear();

	const QStringList directory_list( getSearchDirs() ); //hardcoded and user set directories
	for (auto &directory : directory_list) {
		if (directory.isEmpty())
			continue;

		//display only (XML) files:
		const QStringList apps( QDir(directory).entryList(filters, QDir::Files) );
		//we check how much individual directories contribute for appropriate folder info display:
		const int count_before_applications = applications_->count();
		const int count_before_simulations = simulations_->count();

		for (auto &filename : apps) {
			QFile infile(directory + "/" + filename);
			if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
				topLog(tr(R"(Could not check application file: unable to read "%1" (%2))").arg(
				    QDir::toNativeSeparators(filename), infile.errorString()), "error");
				continue;
			}
			QTextStream tstream(&infile);
			int linecount = 0;
			while (!tstream.atEnd()) {
				linecount++;
				if (linecount > 50) //allow this many lines of comments, but then assume it's not our file
					break;
				const QString line( tstream.readLine() );
				static const QRegularExpression regex_inishell(R"(^\<inishell_config (application|simulation)=\"(.*?)\".*?(icon=\"(.*)\")*>.*)");
				const QRegularExpressionMatch match_inishell(regex_inishell.match(line)); //^ allow XML comment at the end
				static const int idx_type = 1;
				if (match_inishell.captured(0) == line && !line.isEmpty()) {
					/*
					 * There is no logical difference between a file containing
					 * an application and one containing a simulation. The attribute
					 * is used solemnly for clarity and to keep two separate lists
					 * for the two.
					 */
					if (match_inishell.captured(idx_type).toLower() == "application")
						applications_->addApplication(infile, match_inishell);
					else
						simulations_->addApplication(infile, match_inishell);
					break;
				}
			} //endwhile
		} //endfor filename

		//display the directory's path as a list separator if it contains valid XMLs:
		if (applications_->count() > count_before_applications)
			applications_->addInfoSeparator(directory, count_before_applications);
		if (simulations_->count() > count_before_simulations)
			simulations_->addInfoSeparator(directory, count_before_simulations);

	} //endfor directory_list

	applications_found = (applications_->count() > 0); //to display an info when empty
	simulations_found = (simulations_->count() > 0);
}

/**
 * @brief Parse a system command associated with a custom button.
 * @details This function performs substitutions to refer to other elements in the workflow
 * panel, and hardcoded substitutions that for example look for an INI key.
 * @param[in] action The system command to parse.
 * @param[in] status_label QLabel to display status info for this command.
 * @return The processed command that's ready to run on the system.
 */
QString WorkflowPanel::parseCommand(const QString &action, QPushButton *button, QLabel *status_label)
{
	/*
	 * IDs of the workflow panel elements are referred to with "%id".
	 * The values of these panels are retrieved, and within them the various
	 * available substitutions via the "${...}" syntax are performed.
	 */
	QString command(action);
	static const QString regex_substitution(R"(%\w+(?=\s|$))");
	static const QRegularExpression rex(regex_substitution);

	QRegularExpressionMatchIterator rit = rex.globalMatch(action);
	while (rit.hasNext()) {
		const QRegularExpressionMatch match( rit.next() );
		const QString id(match.captured(0).mid(1)); //ID without %

		const QString internal_id( "_workflow_" + Atomic::getQtKey(id) );
		QWidgetList input_widget_list( button->parent()->findChildren<QWidget *>(internal_id) );
		if (input_widget_list.isEmpty()) //current tab does not have it - look everywhere
			input_widget_list = this->findChildren<QWidget *>(internal_id);
		if (input_widget_list.size() > 1)
			workflowStatus(tr(R"(Multiple elements found for ID "%1")").arg(id), status_label);
		if (input_widget_list.isEmpty()) {
			workflowStatus(tr(R"(Element ID "%1" not found)").arg(id), status_label);
		} else {
			//get the element's value:
			QString substitution = getWidgetValue(input_widget_list.at(0));
			//substitutions are not only for commands but also available in widget values:
			commandSubstitutions(substitution, status_label);
			command.replace(match.captured(0), substitution);
		}
	} //end while rit.hasNext()

	//substitutions are also available in the command itself:
	commandSubstitutions(command, status_label);
	return command;
}

/**
 * @brief Perform a number of substitutions in a user-set system command.
 * @param[in] command Command to perform substitutions for.
 * @param[in] status_label The QLabel associated to the command's workflow section.
 */
void WorkflowPanel::commandSubstitutions(QString &command, QLabel *status_label)
{
	/*
	 * Currently available are "${inifile}" for the current INI file path,
	 * "${key:<ini_key>}" for INI values available in the GUI.
	 */

	/* substitute the current INI file's path */
	if (command.contains("${inifile}")) {
		const QString current_ini( getMainWindow()->getIni()->getFilename()) ;
		if (current_ini.isEmpty())
			workflowStatus(tr("Empty INI file - you need to save first"), status_label);
		else
			command.replace("${inifile}", current_ini);
	}

	/* substitute INI keys from the current GUI values */
	static const QString regex_key(R"(\${key:(.+)})");
	static const QRegularExpression rex_key(regex_key);
	const QRegularExpressionMatch match_key(rex_key.match(command));
	static const int idx_key = 1;
	if (match_key.hasMatch()) {
		QStringList section_and_key( match_key.captured(idx_key).split(Cst::sep) );
		if (section_and_key.size() != 2) {
			workflowStatus(tr("INI key must be SECTION") + Cst::sep + "KEY", status_label);
			return;
		}
		const QString value( getMainWindow()->getIni()->get(section_and_key.at(0), section_and_key.at(1)) );
		command.replace("${key:" + match_key.captured(idx_key) + "}", value, Qt::CaseInsensitive);
		if (value.isEmpty())
			workflowStatus(tr(R"(INI key "%1" not found)").arg(match_key.captured(idx_key)), status_label);
	}

}

/**
 * @brief Parse button command and execute "open URL" if applicable.
 * @param[in] command The command to parse.
 * @return True if the command was indeed a request to open an URL.
 */
bool WorkflowPanel::actionOpenUrl(const QString &command) const
{
	static const QString regex_openurl(R"(openurl\((.*)\))");
	static const QRegularExpression rex(regex_openurl);
	const QRegularExpressionMatch match_url( rex.match(command) );
	if (match_url.captured(0) == command && !command.isEmpty()) {
		QDesktopServices::openUrl(QUrl(match_url.captured(1)));
		return true;
	}
	return false;
}

/**
 * @brief Parse button command and execute "switch PathView path" if applicable.
 * @param[in] command The command to parse.
 * @return True if the command was indeed a request to switch a PathView's path.
 */
bool WorkflowPanel::actionSwitchPath(const QString &command, QLabel *status_label, const QString &ref_path)
{
	static const QString regex_setpath(R"(setpath\(%(.*),\s*(.*)\))");
	static const QRegularExpression rex_setpath(regex_setpath);
	const QRegularExpressionMatch match_setpath(rex_setpath.match(command));
	static const int idx_element = 1;
	static const int idx_path = 2;
	if (match_setpath.captured(0) == command && !command.isEmpty()) {
		auto *path_view = this->findChild<PathView *>("_workflow_" +
		    Atomic::getQtKey(match_setpath.captured(idx_element)));
		/*
		 * On startup, the loaded INI is not available yet so we need a mechanism
		 * to change directory, for example to switch to the output folder
		 * of simulation software that is set via an INI key.
		 */
		if (path_view) {
			//if the provided path is relative, we make it relative to the reference file path
			//which is where the application was run
			if ( QFileInfo(match_setpath.captured(idx_path)).isRelative() ) {
				QDir iniDir( ref_path );
				const QString AbsolutePath( QDir::cleanPath(iniDir.absoluteFilePath( match_setpath.captured(idx_path) )) );
				path_view->setPath( AbsolutePath );
			} else {
				path_view->setPath(match_setpath.captured(idx_path));
			}
		} else {
			workflowStatus(tr(R"(Path element ID "%1" not found)").arg(
			    match_setpath.captured(idx_path)), status_label);
		}
		return true;
	}
	return false;
}

/**
 * @brief Parse button command and execute "click other button" if applicable.
 * @param[in] command The command to parse.
 * @return True if the command was indeed a request to click another button.
 */
bool WorkflowPanel::actionClickButton(const QString &command, QPushButton *button, QLabel *status_label)
{
	static const QString regex_clickbutton(R"(button\(%(.*?)\s*\))");
	static const QRegularExpression rex_clickbutton(regex_clickbutton);
	const QRegularExpressionMatch match_clickbutton(rex_clickbutton.match(command));
	static const int idx_button = 1;
	if (match_clickbutton.captured(0) == command && !command.isEmpty()) {
		auto *clicked_button = this->findChild<QPushButton *>("_workflow_" + Atomic::getQtKey(
		    match_clickbutton.captured(idx_button)));
		if (clicked_button) {
			if (clicked_button->objectName() == button->objectName()) {
				workflowStatus(tr("A button can not click itself"), status_label);
				return true; //TODO: circular clicks still possible --> infinite loop
			}
			//wait until the buttons' processes have finished before clicking the next one:
			clicked_button_running_ = true; //set to false when process finishes
			clicked_button->animateClick();
			while (clicked_button_running_)
				QApplication::processEvents(); //keep GUI responsive
		} else {
			workflowStatus(tr(R"(Button with ID "%1" not found)").arg(
			    match_clickbutton.captured(idx_button)), status_label);
		}
		return true;
	}
	return false;
}

/**
 * @brief Execute a system command on button click.
 * @param[in] command The command to execute.
 * @return True if the "stop all processes" flag has been set when waiting.
 */
bool WorkflowPanel::actionSystemCommand(const QString &command, QPushButton *button, const QString &ref_path, const QString &appname)
{
	const int idx = button->parent()->property("stack_index").toInt();
	auto *terminal = static_cast<TerminalView *>( //get the button's terminal view
	    getMainWindow()->getControlPanel()->getWorkflowStack()->widget(idx));

	button->setText(tr("Stop process"));
	button->setStyleSheet("QPushButton {background-color: " + colors::getQColor("important").name() + "}");

	os::setSystemPath(appname.toLower()); //set an enhanced PATH to have more chances to find the app
	auto *process = new QProcess(button); //set as parent so button can look up what to cancel
	//set the working directory for the child process: either cwd or something related to the inifile
	process->setWorkingDirectory(ref_path);

	connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
	    [=](int exit_code, QProcess::ExitStatus exit_status)
	    { processFinished(exit_code, exit_status, terminal, button); });
	connect(process, &QProcess::readyReadStandardOutput, this, [=]{ processStandardOutput(terminal); });
	connect(process, &QProcess::readyReadStandardError, this, [=]{ processStandardError(terminal); });
	connect(process, &QProcess::errorOccurred, this, [&](const QProcess::ProcessError& error){ processErrorOccured(error, terminal, button); });

	terminal->log("\033[3mPATH set to: "+QString::fromLocal8Bit(qgetenv("PATH"))+"\033[0m");
	process->start(command); //execute the system command via QProcess

	terminal->log(html::bold("$ " + command)); //show what is being run
	topStatus(tr("A process is running..."), "normal", true);
	getMainWindow()->refreshStatus(); //blocked at this point if not called manually
	getMainWindow()->repaint();
	while (process->state() == QProcess::Starting || process->state() == QProcess::Running)
		QCoreApplication::processEvents();
	return button->property("process_closing").toBool(); //user has requested all processes (of this button) to stop
}

/**
 * @brief Build the path to use as a reference for running the application, for the outputs, etc from the user selection
 * @param[in] button Element to get the value from.
 * @param[in] ini_path Path to the ini file.
 * @return Either ini_path or cwd or something related to ini_path depending on the user's choice
 */
QString WorkflowPanel::setReferencePath(const QPushButton *button, const QString &ini_path) const
{
	if (button->parent()->property("action").toString() == "terminal") {
		const QComboBox *working_directory( button->parent()->findChild<QComboBox *>("_working_directory_") );
		if (working_directory && working_directory->currentText().contains("{inifile}")) {
			QString new_path( working_directory->currentText() );
			new_path.replace("{inifile}", ini_path);
			return new_path;
		} else {
			return QDir::currentPath();
		}
	} else {
		return ini_path;
	}
}

/**
 * @brief Get a workflow panel's value in text form.
 * @param[in] widget Element to get the value from.
 * @return Value that is currently set in the panel.
 */
QString WorkflowPanel::getWidgetValue(QWidget *widget) const
{
	if (auto *wid = qobject_cast<QDateTimeEdit *>(widget))
		return wid->dateTime().toString(Qt::DateFormat::ISODate);
	else if (auto *wid = qobject_cast<QLineEdit *>(widget))
		return wid->text();
	else if (auto *wid = qobject_cast<QCheckBox *>(widget))
		return (wid->checkState() == Qt::Checked? "TRUE" : "FALSE");
	return QString();
}

/**
 * @brief Delegated event listener for when a started system process finishes.
 * @details This function is called by a lambda when a process that was started by a button
 * click exits.
 * @param[in] exit_code Flag telling if the process has finished normally or with errors.
 * @param[in] exit_status Flag telling if the process has finished normally or with errors.
 * @param[in] terminal The TerminalView associated with the process.
 * @param[in] button Button that started the process.
 */
void WorkflowPanel::processFinished(int exit_code, QProcess::ExitStatus exit_status, TerminalView *terminal,
    QPushButton *button)
{
	if (exit_status != QProcess::NormalExit) {
		const QString msg( QString(tr(
		    "The process was terminated unexpectedly (exit code: %1, exit status: %2).")).arg(
		    exit_code).arg(exit_status) );
		terminal->log(html::color(html::bold(msg), "error"));
		topStatus(tr("Process was terminated"), "error", false);
	} else {
		topStatus(tr("Process has finished"), "normal", false);
	}
	//the button starting the command was set to "Stop process" when clicked - switch back:
	button->setText(button->property("caption").toString()); //original caption
	button->setStyleSheet("");
	terminal->log(html::color(html::bold("$ " + QDir::currentPath()), "normal"));
	clicked_button_running_ = false; //for buttons that click other buttons (and wait for them to finish)
	QApplication::alert( this ); //notify the user that the task is finished
}

/**
 * @brief Delegated event listener for when there is output ready from a started system process.
 * @details This function is called by a lambda when new stdout text is available.
 * @param[in] terminal The TerminalView associated with this process.
 */
void WorkflowPanel::processStandardOutput(TerminalView *terminal)
{
	//read line by line for proper buffering:
	 while( static_cast<QProcess *>((QObject::sender()))->canReadLine() ) {
		const QString str_std( static_cast<QProcess *>((QObject::sender()))->readLine() );
		terminal->log(str_std);
	}
}

/**
 * @brief Delegated event listener for when there is error output ready from a started system process.
 * @details This function is called by a lambda when new stderr text is available.
 * @param[in] terminal The TerminalView associated with this process.
 */
void WorkflowPanel::processStandardError(TerminalView *terminal)
{
	const QString str_err(static_cast<QProcess *>(QObject::sender())->readAllStandardError());
	if (!str_err.isEmpty())
		terminal->log(str_err, true); //log as error
}

/**
 * @brief Delegated event listener for when there is an error starting the process.
 * @details This function is called by a lambda when the child process returns an error.
 * @param[in] terminal The TerminalView associated with this process.
 */
void WorkflowPanel::processErrorOccured(const QProcess::ProcessError &error, TerminalView *terminal, QPushButton *button)
{
	QString message;
	switch(error) {
		case QProcess::FailedToStart:
		{
			const QString tmp( tr("Can not start process. Please make sure that the executable is in the PATH environment variable or in any of the following paths "));
			message = tmp + os::getExtraPath("{application name}");
			break;
		}
		case QProcess::Crashed:
			break;
		case QProcess::Timedout:
			message = tr("Timeout when running process...");
			break;
		case QProcess::WriteError:
		case QProcess::ReadError:
			message = tr("Can not read or write to process.");
			break;
		default:
			message = tr("Unknown error when running process.");
	}

	topStatus(tr("Process was terminated"), "error", false);
	if (!message.isEmpty()) {
		terminal->log(html::color(html::bold(message), "error"));
		topLog("[Workflow] " + message, "error"); //also log to main logger
	}

	//the button starting the command was set to "Stop process" when clicked - switch back:
	button->setText(button->property("caption").toString()); //original caption
	button->setStyleSheet("");
	terminal->log(html::color(html::bold("$ " + QDir::currentPath()), "normal"));
	clicked_button_running_ = false; //for buttons that click other buttons (and wait for them to finish)
}

/**
 * @brief Display info concerning a certain workflow panel.
 * @details Cf. notes in buildWorkflowSection() for the QLabel.
 * @param message
 * @param status_label
 */
void WorkflowPanel::workflowStatus(const QString &message, QLabel *status_label)
{
#ifdef DEBUG
	if (!status_label) {
		qDebug() << "A wokflow status label does not exist when it should in commandSubstitutions()";
		return;
	}
#endif //def DEBUG
	status_label->setText(message);
	topLog("[Workflow] " + message, "error"); //also log to main logger
}

/**
 * @brief Event listener for when a custom button (read from XML) is clicked in the workflow panel.
 * @details Buttons can execute and stop system processes, open URLs, and interact with other
 * workflow panels.
 * @param[in] button The clicked button.
 * @param[in] action_list A list of system commands that the button should execute.
 */
void WorkflowPanel::buttonClicked(QPushButton *button, const QStringList &action_list, const QString& appname)
{
	const QString current_ini( getMainWindow()->getIni()->getFilename());
	const QString ini_path = (current_ini.isEmpty())? QString() : QFileInfo( current_ini ).absolutePath();
	const QString ref_path( setReferencePath(button, ini_path) ); //this contains the path where the application runs

	/*
	 * A button can currently perform the following actions:
	 *   - Execute a system command
	 *   - Stop said system command
	 *   - Open an URL
	 *   - Set the path of a PathView element (e. g. to set a path from an INI key)
	 *   - Click another button (e. g. for a "Run all" button)
	 */

	/*
	 * If there is a stylesheet set for the button this means that it is currently running
	 * a process, and should therefore act as a stop button. Stop all processes and reset
	 * the style.
	 */
	auto *status_label = button->parent()->findChild<QLabel *>("_status_label_");
	topStatus(QString(), QString(), false); //clear left-over messages
	if (!button->styleSheet().isEmpty()) {
		//QProcess items are started with the starting button as parent, so we can find
		//all processes that were started by the button:
		QList<QProcess *> process_list( button->findChildren<QProcess *>() );
		for (auto &process : process_list)
			process->close();
		button->text() = button->property("caption").toString();
		button->setStyleSheet("");
		button->setProperty("process_closing", true); //to stop loop of multiple commands
		return;
	}

	/*
	 * Run through the list of commands that were given by the user in the XML. After each command
	 * we allow user input (e. g. Stop button clicks), but we do wait until the command has finished
	 * before starting a new one.
	 */
	status_label->clear();
	for (int ii = 0; ii < action_list.size(); ++ii) {
		//this property is only looked at in the loop below that waits for stop button clicks:
		button->setProperty("process_closing", false);
		const QString command( parseCommand(action_list[ii], button, status_label) ); //cmd ready to execute

		/* open an URL */
		if (actionOpenUrl(command))
			continue;

		/* set a PathView's path */
		if (actionSwitchPath(command, status_label, ref_path))
			continue;

		/* click another button */
		if (actionClickButton(command, button, status_label))
			continue;

		/* execute a system command */
		if (actionSystemCommand(command, button, ref_path, appname))
			break; //user has requested to stop all processes
	}
}

/**
 * @brief Display the content in the main area that belongs to the clicked workflow panel.
 * @param[in] index Index of the section that is being clicked in the workflow tab.
 */
void WorkflowPanel::toolboxClicked(int index)
{
	if (getMainWindow()->getControlPanel() == nullptr)
		return; //stacked widget not built yet
	QStackedWidget *stack( getMainWindow()->getControlPanel()->getWorkflowStack() );
	const QString action( workflow_container_->widget(index)->property("action").toString() );
	//if there is a terminal action associated with this section show the terminal; if not,
	//show the main GUI:
	if (action == "terminal") {
		const int idx = workflow_container_->widget(index)->property("stack_index").toInt();
		stack->setCurrentIndex(idx); //bring this section's main window to the top
	} else {
		stack->setCurrentIndex(0);
	}
}
