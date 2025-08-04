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

#include "common.h"
#include "colors.h"
#include "src/main/os.h"
#include "src/main/settings.h"

#include <QDir>
#include <QFileInfo>

namespace html {

/**
 * @brief Add HTML embolden tags to text.
 * @param[in] text Text to add embolden tags to.
 * @return Text enclosed by embolden tags.
 */
QString bold(const QString &text)
{
	return "<b>" + text + "</b>";
}

/**
 * @brief Add HTML color tags to text.
 * @param[in] text Text to add color tags to.
 * @return Text enclosed by color tags.
 */
QString color(const QString &text, const QString &color)
{
	const QString strRet( "<font color=\"" + colors::getQColor(color).name() + "\">" + text + "</font>");
	return strRet;
}

} //namespace html

/**
 * @brief Retrieve the message of an event that wants to communicate.
 * @details This class is used for example by the XML schema validation.
 * @param[in] type Type of the message (unused).
 * @param[in] description A text description of what has happened.
 * @param[in] identifier Identifier for the message (unused).
 * @param[in] location The location something has happened at (e. g. line number for text errors).
 */
void MessageHandler::handleMessage(QtMsgType type, const QString &description, const QUrl &identifier,
    const QSourceLocation &location)
{
	Q_UNUSED(type) //turn off compiler warnings
	Q_UNUSED(identifier)

	description_ = description;
	location_ = location;
}

/**
 * @brief Fill a list with directories to search for XMLs.
 * @details This function queries a couple of default folders on various systems, as well as
 * ones that can be set by the user. Duplicates (e. g. the same folder given by a relative
 * and an absolute path) are ignored.
 * @return A list of directories to search for XMLs.
 */
QStringList getSearchDirs(const bool &include_user_set, const bool &include_nonexistent_folders)
{

	/* hardcoded directories, the system specific paths are partly handled by Qt */
	QStringList locations;
	locations << "."; //the application's current directory
	os::getSystemLocations(locations); //update list with OS specific search paths

	//TODO: recursive search
	QStringList dirs;
	for (auto &tmp_dir : locations) {
		dirs << tmp_dir + "/inishell-apps";
		dirs << tmp_dir + "/simulations";
	}

	if (include_user_set) {
		/* user set paths */
		const QStringList user_xml_paths( getListSetting("user::xmlpaths", "path") );
		dirs.append(user_xml_paths);
	}

	//now, check that we don't have the same folder multiple times (e. g. via relative and absolute paths):
	QStringList filtered_dirs;
	for (auto &dir : dirs) {
		const QFileInfo dinfo( dir );
		if (!include_nonexistent_folders && !dinfo.exists())
			continue;
		bool found = false;
		for (const auto &filtered_dir : filtered_dirs) {
			if (dinfo.absoluteFilePath() == QFileInfo( filtered_dir ).absoluteFilePath()) {
				found = true;
				break;
			}
		}
		if (!found)
			filtered_dirs << dir;
	}

	return filtered_dirs;
}

/**
 * @brief Convert a key press event to a key sequence.
 * @details This way key sequences can also be used in key press event listeners.
 * @param[in] event Input key press event.
 * @return The key event converted to a QKeySequence.
 */
QKeySequence keyToSequence(QKeyEvent *event)
{ //this is a bit of a HACK: get key sequences in event listeners
	QString modifier;
	if (event->modifiers() & Qt::ShiftModifier)
		modifier += "Shift+";
	if (event->modifiers() & Qt::ControlModifier)
		modifier += "Ctrl+";
	if (event->modifiers() & Qt::AltModifier)
		modifier += "Alt+";
	if (event->modifiers() & Qt::MetaModifier)
		modifier += "Meta+";

	const QString key_string( QKeySequence(event->key()).toString() );
	return QKeySequence( modifier + key_string );
} //https://forum.qt.io/topic/73408/qt-reading-key-sequences-from-key-event
