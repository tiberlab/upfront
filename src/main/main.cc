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
 * INIshell - A dynamic graphical interface to generate INI files
 * INIshell v2: Michael Reisecker, 2019
 * Inspired by INIshell v1: Mathias Bavay, Thomas Egger & Daniela Korhammer, 2011
 *
 * This is the main program starting the event loop.
 * 2019-10
 */

#include "colors.h"
#include "common.h"
#include "Error.h"
#include "XMLReader.h"
#include "src/gui/MainWindow.h"
#include "src/main/settings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QStandardPaths>
#include <QStringList>
#include <QStyleFactory>
#include <QTranslator>

#include <iostream>

/**
 * @brief Set meta data for the application.
 * @details This is used in different places INIshell writes and installs to depending on the OS.
 */
void setAppMetadata()
{
	QApplication::setApplicationName("INIshell");
	QApplication::setOrganizationName("SLF");
	QApplication::setOrganizationDomain("slf.ch");
	QApplication::setApplicationVersion(APP_VERSION_STR);
	QApplication::setWindowIcon(QIcon(":/icons/inishell_192.ico"));
}

/**
 * @brief Prepare the command line parser options.
 * @param[in] parser Instance of the command line parser.
 * @param[in] cmd_args Command line args given to the programs.
 */
void prepareCommandline(QCommandLineParser &parser, command_line_args &cmd_args)
{
	QList<QCommandLineOption> cmd_options;
	cmd_options << QCommandLineOption({"e", "exit"}, "Exit after command line operations (surpass GUI)");
	cmd_options << QCommandLineOption({"i", "inifile"}, "INI file to import on startup\nUse syntax SECTION::KEY=\"value\" as additional arguments to modifiy INI keys", "inifile");
	cmd_options << QCommandLineOption({"s", "settingsfile"}, "INIshell settings file", "settingsfile");
	cmd_options << QCommandLineOption({"o", "outinifile"}, "INI file to write out", "outinifile");
	cmd_options << QCommandLineOption("dump_resources", "Dump internal resource files to current directory");
	cmd_options << QCommandLineOption("dump_help", "Dump user's guide and developer's help to current directory");
	cmd_options << QCommandLineOption("print_search_dirs", "Print list of directories INIshell searches");
	cmd_options << QCommandLineOption("print_settings_location", "Print location of the settings file");
	cmd_options << QCommandLineOption({"c", "clear"}, "Clear settings file");
	cmd_options << QCommandLineOption("print_styles", "Print available Qt styles");
	cmd_options << QCommandLineOption("set_style", "Set the program style", "style");
	cmd_options << QCommandLineOption("info", "Display program info");

	parser.addOptions(cmd_options);
	parser.addHelpOption();
	parser.addVersionOption();

	parser.process(QCoreApplication::arguments());
	cmd_args.startup_ini_file = parser.value("inifile");
	cmd_args.settings_file = parser.value("settingsfile");
	cmd_args.out_ini_file = parser.value("outinifile");
	cmd_args.program_style = parser.value("set_style");
}

/**
 * @brief Work through command line arguments.
 * @details This function processes command line options on program start.
 * @return True if "exit" was parsed and the user wants to immediately quit after
 * the command line tools.
 */
bool workCommandlineArguments(QCommandLineParser *parser)
{
	if (parser->isSet("clear")) {
		const QString xml_settings_filename(getSettingsFileName());
		std::cout << "Deleting " << xml_settings_filename.toStdString() << "..." << std::endl;
		QFile sfile( getSettingsFileName() );
		sfile.remove();
		if (sfile.error())
			std::cerr << "[E] Can't delete settings file: " << sfile.errorString().toStdString() << std::endl;
	}
	if (parser->isSet("dump_resources")) { //note that resource files are write-protected when copying out
		std::cout << "Dumping config_schema.xsd..." << std::endl;
		QFile::copy(":config_schema.xsd", QDir::currentPath() + "/config_schema.xsd");
		std::cout << "Dumping inishell_settings_minimal.xml..." << std::endl;
		QFile::copy(":inishell_settings_minimal.xml", QDir::currentPath() + "/inishell_settings_minimal.xml");
	}
	if (parser->isSet("dump_help")) {
		std::cout << "Dumping help.xml and help_dev.xml..." << std::endl;
		QFile::copy(":doc/help.xml", QDir::currentPath() + "/help.xml");
		QFile::copy(":doc/help_dev.xml", QDir::currentPath() + "/help_dev.xml");
	}
	if (parser->isSet("print_search_dirs")) {
		const QStringList search_dirs( getSearchDirs(false) ); //without user dirs, settings not ready yet
		std::cout << "Searching the following directories:" << std::endl;
		for (auto &dir: search_dirs)
			std::cout << dir.toStdString() << std::endl;
	}
	if (parser->isSet("print_settings_location")) {
		std::cout << "Location of settings file: " << getSettingsFileName().toStdString() << std::endl;
	}
	if (parser->isSet("print_styles")) {
		std::cout << "The following styles are available:" << std::endl;
		for (auto &style : QStyleFactory::keys())
			std::cout << style.toStdString() << std::endl;
	}
	if (parser->isSet("info")) {
		std::cout << QApplication::applicationName().toStdString() << " " <<
		    QApplication::applicationVersion().toStdString() << std::endl;
		std::cout << "(c) 2019 " << QApplication::organizationName().toStdString() << ", " <<
		    QApplication::organizationDomain().toStdString() << std::endl;
		std::cout << "INIshell is a dynamic graphical user interface to manipulate INI files." << std::endl;
		std::cout << "Visit https://models.slf.ch/p/inishell-ng/ for more information." << std::endl;
		std::cout << "License: GNU General Public License" << std::endl;
		std::cout << "Run ./inishell --help to view all command line options." << std::endl;
		return true; //don't enter GUI
	}

	return (parser->isSet("exit"));
}

/**
 * @brief Load settings from INIshell's XML settings file.
 * @details This has nothing to do with the XMLs that are parsed to build the GUI,
 * stored here are INIshell's own static GUI settings (like the language etc.).
 * @param cmd_args
 * @return
 */
void loadSettings(QString &settings_file, QStringList &errors)
{
	if (!QFileInfo( settings_file ).exists()) { //quietly create file after first program start
		global_xml_settings = QDomDocument( );
		return;
	}

	XMLReader xml_settings_reader;
	QString xml_error = QString();
	xml_settings_reader.read(settings_file, xml_error, true);
	if (!xml_error.isNull()) {
		errors.push_back(QApplication::tr("Could not read settings file. Unable to load \"") +
		    QDir::toNativeSeparators(settings_file) + "\"\n" + xml_error +
		    QApplication::tr("If possible, the settings file will be recreated for the next program start (check INIshell's write access to the directory).\nIf not, INIshell will function normally but will not be able to save any settings."));
	}
	global_xml_settings = xml_settings_reader.getXml();
}

/**
 * @brief Set global stylesheets for panels/widgets.
 * @details Global styling is done here, including the styles of properties that may or may not
 * be set, such as how a mandatory field that is not filled in should look. Further styling is
 * done locally.
 * @param[in] app The main app instance.
 */
void setAppStylesheet(QApplication &app, const command_line_args &cmd_args)
{
	/*
	 * Unfortunately, it is technically not possible in Qt to style a widget in any way while
	 * keeping the native OS style, at least it is not guaranteed. Same for Palettes, which
	 * may or may not be respected. Hence, it is impossible to color for example a button and
	 * keep the macOS style, which also leads to surprising differences in the sizes.
	 * See the following links:
	 *   https://doc.qt.io/qt-5/stylesheet.html (last paragraph)
	 *   https://stackoverflow.com/questions/28839907/how-to-override-just-one-propertyvalue-pair-in-qt-stylesheet
	 * Furthermore, there are bugs in many of the styles that concern our look, for example
	 * a frame's caption may be striked through by the border, and a frame coloring may extend
	 * outside the frame.
	 * For these reasons we try to set a fixed style that is widely available. "Fusion" should be
	 * built by default; make sure on deployment that the plugin is included. The way I see it,
	 * the only alternative is to style each and every widget we use manually.
	 */

	if (!cmd_args.program_style.isEmpty())
		QApplication::setStyle(cmd_args.program_style);
	else
#if defined Q_OS_WIN
		QApplication::setStyle("WindowsVista");
#else
		if (QStyleFactory::keys().contains("Fusion"))
			QApplication::setStyle("Fusion");
#endif
	/*
	 * Set the global stylesheet:
	 * Here we define some properties that can be set for all panels (without casting to their type),
	 * and the style that will be applied if the property is set (e. g. default values, faulty
	 * expressions, ...).
	 * We also try to avoid gaps and borders of different colors, hence we set backgrounds for some
	 * of our design elements trying to take into account current OS color scheme settings.
	 */
	app.setStyleSheet(" \
	    * [mandatory=\"true\"] {background-color: " + colors::getQColor("mandatory").name() + "; color: " + colors::getQColor("normal").name() + "} \
	    * [shows_default=\"true\"] {font-style: italic; color: " + colors::getQColor("default_values").name() + "} \
	    * [faulty=\"true\"] {color: " + colors::getQColor("faulty_values").name() + "} \
	    * [valid=\"true\"] {color: " + colors::getQColor("valid_values").name() + "} \
	    QTabWidget {padding: 0px; font-weight: bold; background-color: " + colors::getQColor("app_bg").name() + "} \
	    QTabWidget:pane {background-color: " + colors::getQColor("app_bg").name() + "} \
	    QScrollArea {background-color: " + colors::getQColor("app_bg").name() + "} \
	    QScrollBar:horizontal {height: 15px;} \
	    Group {background-color: " + colors::getQColor("app_bg").name() + "} \
	");
}

/**
 * @brief Perform INI operations in command line mode.
 * @details INIshell will still start the GUI for most command line operations, unless explicitly
 * asked to quit via -e.
 * @param[in] parser Command line parser object.
 * @param[in] cmd_args Container for the command line arguments.
 * @param[in] errors Error messages to add on to if necessary.
 */
void perform_cmd_ini_operations(const QCommandLineParser &parser, const command_line_args &cmd_args, QStringList &errors)
{
	const QString in_inifile( cmd_args.startup_ini_file );
	const QString out_inifile( cmd_args.out_ini_file );
	if (!out_inifile.isEmpty() && in_inifile.isEmpty()) {
		const QString err_msg(
		    QApplication::tr(R"(To output a file with "-o" you need to specify the input file with "-i")"));
		errors.push_back(err_msg);
		std::cerr << "[E] " << err_msg.toStdString() << std::endl;
	} else if (!in_inifile.isEmpty()) {
		if (out_inifile.isEmpty()) {
			const QString err_msg(QApplication::tr(
			    R"(To input a file with "-i" you need to specify the output file with "-o")"));
			errors.push_back(err_msg);
			std::cerr << "[E] " << err_msg.toStdString() << std::endl;
		} else {
			INIParser cmd_ini;
			cmd_ini.parseFile(in_inifile);

			/* modify INI keys */
			for (auto &pos : parser.positionalArguments()) {
				const QStringList mod_ini_list(pos.split("="));
				if (mod_ini_list.size() == 2) {
					const QStringList param_list(mod_ini_list.at(0).trimmed().split(
					    Cst::sep, QString::SkipEmptyParts));
					if (param_list.size() == 2) //silently skip wrong formats
						cmd_ini.set(param_list.at(0), param_list.at(1),
						    mod_ini_list.at(1).trimmed());
				}
			}

			QFile ini_output(out_inifile);
			if (ini_output.open(QIODevice::WriteOnly)) {
				QTextStream iniss(&ini_output);
				cmd_ini.outputIni(iniss);
			} else {
				const QString err_msg(QApplication::tr(R"(Unable to open output INI file "%1": %2)").arg(
				    QDir::toNativeSeparators(out_inifile), ini_output.errorString()));
				errors.push_back(err_msg);
				std::cerr << "[E] " << err_msg.toStdString() << std::endl;
			}
		} //endif out_inifile.isEmpty()
	} //endif in/outfile.isEmpty()
}

/**
 * @brief Entry point of the main program.
 * @details This function starts the main event loop.
 * @param[in] argc Command line arguments count.
 * @param[in] argv Command line arguments.
 * @return Exit code.
 */
int main(int argc, char *argv[])
{
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QStringList errors; //errors that happen before a logger is available
	QApplication app(argc, argv);
	setAppMetadata();

	/* parse the command line */
	QCommandLineParser parser;
	command_line_args cmd_args;
	prepareCommandline(parser, cmd_args); //will also be used in MainWindow() where more info is available
	const bool exit_early = workCommandlineArguments(&parser);

	/* load and apply settings for the static part of the GUI */
	QString xml_settings_filename(getSettingsFileName()); //e. g. ".config/SLF/INIshell/inishell_settings.xml" on GNU/Linux
	if (!cmd_args.settings_file.isEmpty()) //file given in command line
		xml_settings_filename = cmd_args.settings_file;
	loadSettings(xml_settings_filename, errors); //load global settings file
	checkSettings(); //make sure valid settings can be read throughout this program run

	QFont global_font(QApplication::font());
	global_font.setPointSize(getSetting("user::appearance::fontsize", "value").toInt());
	QApplication::setFont(global_font);

	/* command line INI file manipulation */
	perform_cmd_ini_operations(parser, cmd_args, errors);
	if (exit_early) //exit after command line tools if user gives "--exit"
		return 0;

	/* GUI mode when reaching this */
	const QString language( getSetting("user::appearance::language", "value") );
	QTranslator translator; //can't go out of scope
	if (!language.isEmpty() && language != "en") { //texts that are not found will remain in English
		const QString language_file(":/languages/inishell_" + language);
		if (translator.load(language_file)) {
			QApplication::installTranslator(&translator);
		} else { //should not happen since it's a resource and the build process would complain
			Error("Language file not found", "File \"" + language_file +
			    "\" is not a valid language file.");
			errors.push_back("Language file not found ~ File \"" + language_file +
			    "\" is not a valid language file.");
		}
	} //endif language

	setAppStylesheet(app, cmd_args);
	//open MainWindow with the XML settings and their path, and errors that occurred so far:
	MainWindow main_window(xml_settings_filename, errors);
	main_window.show(); //start INIshell's GUI
	errors.clear(); //save a bit of RAM - the messages have been processed
	return QApplication::exec();
}
