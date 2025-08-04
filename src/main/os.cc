/*****************************************************************************/
/*  Copyright 2020 WSL Institute for Snow and Avalanche Research  SLF-DAVOS  */
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

#include "os.h"

#include <QCoreApplication>
#include <QDir>
#include <QKeySequence>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QtGlobal>

namespace os {

/**
 * @brief Choose a number of locations to look for applications at.
 * @param[out] locations List of locations which will have the system specific ones appended.
 */
void getSystemLocations(QStringList &locations)
{
#if defined Q_OS_WIN
	locations << "../.."; //this is useful for some out of tree builds
	locations << QCoreApplication::applicationDirPath(); //directory that contains the application executable
	locations << QCoreApplication::applicationDirPath() + "/../share";
	locations << QCoreApplication::applicationDirPath() + "/..";
	locations << QStandardPaths::standardLocations(QStandardPaths::DesktopLocation);
	locations << QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
	locations << QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
	locations << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation); //location where persistent application data can be stored
#endif
#if defined Q_OS_MAC
	locations << "./../../../.."; //this is useful for some out of tree builds: we must get out of the bundle
	locations << QCoreApplication::applicationDirPath();
	locations << QCoreApplication::applicationDirPath() + "/../share";
	locations << QCoreApplication::applicationDirPath() + "/..";
	locations << QCoreApplication::applicationDirPath() + "/../../../.."; //we must get out of the bundle
	locations << QStandardPaths::standardLocations(QStandardPaths::DesktopLocation);
	locations << QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
	locations << QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
	locations << QDir::homePath() + "/usr/share";
#endif
#if !defined Q_OS_WIN && !defined Q_OS_MAC
	locations << QCoreApplication::applicationDirPath(); //directory that contains the application executable
	locations << QCoreApplication::applicationDirPath() + "/../share";
	locations << QCoreApplication::applicationDirPath() + "/..";
	locations << QStandardPaths::standardLocations(QStandardPaths::DesktopLocation);
	locations << QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation); //$HOME/Documents
	locations << QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
	locations << QStandardPaths::standardLocations(QStandardPaths::DataLocation);
	locations << QDir::homePath() + "/usr/share";
#endif
}

/**
 * @brief Check if we're running on KDE Desktop environment.
 * @details On a well-tuned KDE, things should be the smoothest.
 * @return True if on KDE.
 */
bool isKDE() {
	const QString DE( QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP")) );
	return (DE == "KDE");
}

/**
 * @brief Looking at the default palette, return true if it is a dark theme
 * @return true if the current color theme is dark
 */
bool isDarkTheme()
{
#ifdef Q_OS_WIN
	QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
	return (settings.value("AppsUseLightTheme")==0);
#else
	const int bg_lightness = QPalette().color(QPalette::Window).lightness();
	return (bg_lightness < 128);
#endif
}

/**
 * @brief Extended search paths for executables, to preprend or append to a PATH variable.
 * @param[in] appname application's name.
 */
QString getExtraPath(const QString& appname)
{
	const QString own_path( QCoreApplication::applicationDirPath() ); //so exe copied next to inishell are found, this is usefull for packaging
	const QString home( QDir::homePath() );
	const QString desktop( QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0) ); //DesktopLocation always returns 1 element

#if defined Q_OS_WIN
	QString extra_path( ";" + desktop+"\\"+appname+"\\bin;" + home+"\\src\\"+appname+"\\bin;" + "D:\\src\\"+appname+"\\bin;" + "C:\\Program Files\\"+appname+"\\bin;" + "C:\\Program Files (x86)\\"+appname+"\\bin;" + own_path);

	const QString reg_key("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appname + "\\UninstallString");
	QSettings settings;
	const QString uninstallExe( settings.value(reg_key).toString() );
	if (!uninstallExe.isEmpty()) {
		const QString installPath( QFileInfo( uninstallExe ).absolutePath() );
		if (!installPath.isEmpty())
			extra_path.append( ";"+installPath );
	}

	return extra_path;
#endif
#if defined Q_OS_MAC
	QString Appname( appname );
	Appname[0] = Appname[0].toUpper();
	const QString extra_path( ":" +home+"/bin:" + home+"/usr/bin:" + home+"/src/"+appname+"/bin:" + desktop+"/"+appname+"/bin:" + "/Applications/"+appname+".app/Contents/bin:" + "/Applications/"+Appname+".app/Contents/bin:" + "/Applications/"+appname+"/bin:" + "/Applications/"+Appname+"/bin:" + own_path);
	return extra_path;
#endif
#if !defined Q_OS_WIN && !defined Q_OS_MAC
	const QString extra_path( ":" + home+"/bin:" + home+"/usr/bin:" + home+"/src/"+appname+"/bin:" + desktop+"/"+appname+"/bin:" + "/opt/"+appname+"/bin:" + own_path );
	return extra_path;
#endif
}

/**
 * @brief Extend path where to search for executables, ie a proper initialization for a PATH variable.
 * @param[in] appname application's name.
 */
void setSystemPath(const QString& appname)
{
	static const QString root_path( QString::fromLocal8Bit(qgetenv("PATH")) ); //original PATH content with platform specific additions
	QString env_path( root_path + getExtraPath(appname) );
	qputenv("PATH", env_path.toLocal8Bit());
}

/**
 * @brief Return native help keyboard shortcut as string to use in guidance texts.
 * @return Help key sequence as string.
 */
QString getHelpSequence() {
	QKeySequence help_sequence(QKeySequence::HelpContents);
	return help_sequence.toString(QKeySequence::NativeText); //display as is on the machine
}

/**
 * @brief Return the user's logname of the computer running the current process.
 * @return current username or empty string
 */
QString getLogName()
{
	char *tmp;

	if ((tmp=getenv("USERNAME"))==NULL) { //Windows & Unix
		if ((tmp=getenv("LOGNAME"))==NULL) { //Unix
			tmp=getenv("USER"); //Windows & Unix
		}
	}

	if (tmp==NULL) return QString();
	return QString(tmp);
}

} //namespace os
