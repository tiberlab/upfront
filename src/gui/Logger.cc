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

#include "Logger.h"
#include "src/main/colors.h"
#include "src/main/common.h"
#include "src/main/Error.h"
#include "src/main/inishell.h"
#include "src/main/settings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QKeySequence>
#include <QListWidgetItem>
#include <QPushButton>
#include <QTextStream>
#include <QTime>

/**
 * @class Logger
 * @brief Default constructor for the Logger.
 * @details This class is constructed as once and stays alive as a logging window.
 * @param[in] parent The parent window (should be main window).
 */
Logger::Logger(QWidget *parent) : QWidget(parent, Qt::Window) //dedicated window
{
	/* log list, close and save buttons */
	loglist_ = new QListWidget(this);
	auto *close_button = new QPushButton(getIcon("application-exit"), tr("&Close"));
	close_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	connect(close_button, &QPushButton::clicked, this, &Logger::closeLogger);
	auto *save_button = new QPushButton(getIcon("document-save-as"), tr("&Save as..."));
	save_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	connect(save_button, &QPushButton::clicked, this, &Logger::saveLog);
	auto *clear_button = new QPushButton(getIcon("edit-clear-all"), tr("C&lear"));
	connect(clear_button, &QPushButton::clicked, this, &Logger::clearLog);
	clear_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

	/* layout of the buttons */
	auto *button_layout = new QHBoxLayout; //two buttons on the left
	auto *left_frame = new QFrame;
	auto *left_layout = new QHBoxLayout;
	left_layout->setContentsMargins(0, 0, 0, 0);
	left_layout->addWidget(close_button);
	left_layout->addWidget(clear_button);
	left_frame->setLayout(left_layout);
	button_layout->addWidget(left_frame, 0, Qt::AlignLeft);
	auto *right_frame = new QFrame; //one button on the right
	auto *right_layout = new QHBoxLayout;
	right_layout->setContentsMargins(0, 0, 0, 0);
	right_layout->addWidget(save_button);
	right_frame->setLayout(right_layout);
	button_layout->addWidget(right_frame, 0, Qt::AlignRight);

	/* main layout */
	auto *log_layout = new QVBoxLayout;
	log_layout->addWidget(loglist_);
	log_layout->addLayout(button_layout);
	this->setLayout(log_layout);

	this->setWindowTitle(tr("Log Messages") + " ~ " + QCoreApplication::applicationName());
	this->setWindowFlags(Qt::Dialog); //prevent double title bar on osX
}

/**
 * @brief Add a text message to the Logger window.
 * @param[in] message The log message.
 * @param[in] color Color of the log message.
 * @param[in] no_timestamp Set true to disable the timestamp for this message.
 */
void Logger::log(const QString &message, const QString &color, const bool &no_timestamp)
{
	const QString timestamp( QTime::currentTime().toString("[hh:mm:ss] ") );
	auto *msg = new QListWidgetItem((no_timestamp? QString() : timestamp) + message, loglist_);
	msg->setForeground(colors::getQColor(color));
	loglist_->setCurrentRow(loglist_->count() - 1); //move cursor to last msg
	loglist_->setCurrentRow(-1); //... but don't select
}

/**
 * @brief Log some system and Qt version info.
 */
void Logger::logSystemInfo()
{
	log(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ") + QCoreApplication::applicationName() +
	    " " + QCoreApplication::applicationVersion(), "normal", true);
	log(tr("Running on ") + QSysInfo::prettyProductName() + ", " + QSysInfo::kernelVersion() + ", "
	    + QSysInfo::buildAbi() + tr("; built with Qt ") + QT_VERSION_STR);
}

/**
 * @brief Event listener for the ESC key.
 * @details Close the logger on pressing ESC.
 * @param event The key press event that is received.
 */
void Logger::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape || keyToSequence(event) == QKeySequence::Close)
		this->close();
	else if (keyToSequence(event) == QKeySequence::Print)
		emit getMainWindow()->viewPreview();
}

/**
 * @brief Save button: save the log to a file.
 */
void Logger::saveLog()
{
	QString start_path(getSetting("auto::history::last_log_write", "path"));
	if (start_path.isEmpty())
		start_path = QDir::currentPath();
	const QString date = QDateTime::currentDateTime().toString(Qt::DateFormat::ISODate).replace(":", "-");
	const QString file_name = QFileDialog::getSaveFileName(this, tr("Save Log"), start_path + "/inishell_log_" + date + ".html",
	    tr("HTML files") + " (*.html *.htm);;" + tr("Text Files") + " (*.log *.txt *.dat);;" + tr("All Files") + " (*)", nullptr,
	    QFileDialog::DontUseNativeDialog); //native is all but smooth on GNOME
	//QTBUG: https://bugreports.qt.io/browse/QTBUG-67866?focusedCommentId=408617&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-408617
	if (file_name.isNull())
		return;

	/* open the file to save log to */
	QFile outfile(file_name);
	if (!outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		Error(tr("Could not open file for writing"), QString(),
		    file_name + ":\n" + outfile.errorString());
		return;
	}
	QTextStream out_ss(&outfile);

	QFileInfo file_info(file_name);
	bool html = false;
	if (file_info.completeSuffix().toLower() == "html" || file_info.completeSuffix().toLower() == "htm")
		html = true;

	/* save as html or plain text */
	for (int ii = 0; ii < loglist_->count(); ++ii) {
		QString item_text = loglist_->item(ii)->text();
		if (html) { //use the Logger's entries' colors
			const QString color = loglist_->item(ii)->foreground().color().name();
			item_text = html::color(item_text, color);
		}
		out_ss << item_text << (html? "<br>" : "\n");
	}
	outfile.close();
	setSetting("auto::history::last_log_write", "path", file_info.absoluteDir().path());
}

/**
 * @brief Close button: close the window.
 * @details The window is closed but stays in scope.
 */
void Logger::closeLogger()
{
	this->close();
}

/**
 * @brief Clear button: clear the log.
 */
void Logger::clearLog()
{
	loglist_->clear();
	topStatus(QString());
}
