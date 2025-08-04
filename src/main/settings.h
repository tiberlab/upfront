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
 * XML interfact to manage INIshell's settings for the static part of the GUI.
 * The Settings page itself is handled by MainPanel.cc
 * 2019-10
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QButtonGroup>
#include <QListWidget>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtXml>

#ifdef DEBUG
	#include <iostream>
#endif

extern QDomDocument global_xml_settings; //the settings are always in scope

struct command_line_args {
	public:
		QString startup_ini_file = "";
		QString settings_file = "";
		QString out_ini_file = "";
		QString program_style = "";
};

void groupButtons(QButtonGroup &group, QWidget *parent);
void checkSettings();
void saveSettings();

QString getSettingsFileName();
QString getSetting(const QString &setting_name, const QString &attribute = QString());
QStringList getListSetting(const QString &parent_setting, const QString &node_name = QString());
void setListSetting(const QString &parent_setting, const QString &node_name, const QStringList &item_list);
void setSetting(const QString &setting_name, const QString &attribute = QString(),
    const QString &value = QString());
QStringList getSimpleSettingsNames();

#endif //SETTINGS_H
