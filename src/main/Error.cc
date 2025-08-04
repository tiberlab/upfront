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

#include "Error.h"
#include "colors.h"
#include "inishell.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QTextStream>

/**
 * @class Error
 * @brief Default constructor for an Error message.
 * @details This constructor shows only a single paragraph as error text.
 * @param[in] message The error message to display.
 */
Error::Error(const QString &message)
{
	messageBox(message);
}

/**
 * @brief Error constructor with additional info text.
 * @details This constructor shows the main message, and and additional paragraph below.
 * @param[in] message The error message to display.
 * @param[in] infotext Additional error information.
 */
Error::Error(const QString &message, const QString &infotext)
{
	messageBox(message, infotext);
}

/**
 * @brief Error constructor with additional info text and an additional detailed description.
 * @details This constructor shows the main message, an additional paragraph below, and
 * a detailed description that can be shown at the click of a button.
 * @param[in] message The error message to display.
 * @param[in] infotext Additional error information.
 * @param[in] details Additional detailed message.
 */
Error::Error(const QString &message, const QString &infotext, const QString &details)
{
	messageBox(message, infotext, details);
}

/**
 * @brief Error constructor with additional info text, an additional detailed description,
 * and an icon specification.
 * @details This constructor shows the main message, an additional paragraph below, and
 * a detailed description that can be shown at the click of a button. It also sets an icon
 * according to the urgency of the message.
 * @param[in] message The error message to display.
 * @param[in] infotext Additional error information.
 * @param[in] details Additional detailed message.
 */
Error::Error(const QString &message, const QString &infotext, const QString &details,
    const error::urgency &level, const bool &no_log)
{
	messageBox(message, infotext, details, level, no_log);
	if (level == error::fatal) { //currently not used
		QString msg;
		QTextStream ss(&msg);
		ss << QMessageBox::tr("Aborted after fatal error:") << endl;
		ss << message << endl;
		ss << infotext << endl << details;
		throw std::runtime_error(msg.toStdString());
		//TODO: autosave log if this level is ever actually used
	}
}

/**
 * @brief Display a simple info text.
 * @param[in] message The info message to display.
 */
Info::Info(const QString &message)
{
	messageBox(message, QString(), QString(), error::info);
}

/**
 * @brief Wrapper to display a QMessageBox that is styled to our needs.
 * @param[in] message Main message to display(title of the error).
 * @param[in] infotext Additional info text.
 * @param[in] details Collapsible detailed description.
 * @param[in] level Urgency level of the error / warning.
 * @param[in] no_log If true, the error / warning is not automatically logged.
 * @return The button the user has clicked.
 */
int messageBox(const QString &message, const QString &infotext, const QString &details,
    const error::urgency &level, const bool &no_log)
{
	QMessageBox msgBox;
	msgBox.setText("<b>" + message + "</b>");
	msgBox.setInformativeText(infotext);
	if (!details.isEmpty())
		msgBox.setDetailedText(details);

	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Close);
	msgBox.setDefaultButton(QMessageBox::Ok);
	QString color("msg");
	QString title;
	switch (level) {
	case error::info:
		title = QMessageBox::tr("Info");
		msgBox.setIcon(QMessageBox::Information);
		color = "info";
		break;
	case error::warning:
		title = QMessageBox::tr("Warning");
		msgBox.setIcon(QMessageBox::Warning);
		color = "warning";
		break;
	case error::error:
		title = QMessageBox::tr("Error");
		msgBox.setIcon(QMessageBox::Critical);
		color = "error";
		break;
	case error::critical:
		title = QMessageBox::tr("Critical Error");
		msgBox.setIcon(QMessageBox::Critical);
		color = "error";
		break;
	case error::fatal:
		title = QMessageBox::tr("Fatal Error");
		msgBox.setIcon(QMessageBox::Critical);
		color = "error";
	}
	if (!no_log) //for if we want to log something different than is displayed in the error dialog
		topLog(title + ": " + message + (infotext.isEmpty()? "" : " ~ " + infotext) +
		    (details.isEmpty()? "" : " ~ " + details), color);
	msgBox.setWindowTitle(title + " ~ " + QCoreApplication::applicationName());
	return msgBox.exec();
}
