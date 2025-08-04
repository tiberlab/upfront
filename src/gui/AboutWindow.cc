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

#include "AboutWindow.h"
#include "src/main/common.h"
#include "src/main/constants.h"
#include "src/main/inishell.h"

#include <QKeySequence>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>

#ifdef DEBUG
	#include <iostream>
#endif

/**
 * @class AboutWindow
 * @brief Constructor for the About window.
 * @param[in] parent The window's parent.
 */
AboutWindow::AboutWindow(QWidget *parent) : QWidget(parent, Qt::Window)
{
	auto *textbox( new QTextBrowser );
	textbox->setReadOnly(true);
	textbox->setTextInteractionFlags(Qt::TextBrowserInteraction);
	textbox->setOpenExternalLinks(true);

	setAboutText(textbox); //fill with text

	auto *main_layout( new QVBoxLayout );
	main_layout->addWidget(textbox);
	this->setLayout(main_layout);

	/* center the window on screen */
	this->setWindowFlags(Qt::Dialog); //prevent double title bar on osX
	this->setFixedSize(Cst::width_help_about, Cst::height_help_about);
	QScreen *screen_object( QGuiApplication::primaryScreen() );
	QSize screen(screen_object->geometry().width(), screen_object->geometry().height());
	this->move(screen.width() / 2 - Cst::width_help_about / 2,
	    screen.height() / 2 - Cst::height_help_about / 2);
}

/**
 * @brief Catch key press events in the About window..
 * @details Close the About window on pressing ESC, and set the usual
 * shortcuts to show our windows.
 * @param event The key press event that is received.
 */
void AboutWindow::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape || keyToSequence(event) == QKeySequence::Close) {
		this->close();
	} else if (keyToSequence(event) == QKeySequence::Print) {
		emit getMainWindow()->viewPreview();
	} else if (event->modifiers() == Qt::CTRL && event->key() == Qt::Key_L) {
		getMainWindow()->getLogger()->show();
		getMainWindow()->getLogger()->raise();
	}
}

/**
 * @brief Fill the About window with HTLM Text.
 * @details This is hardcoded to be able to conveniently access runtime information (e. g. version info).
 * @param[in] textbox The textbox to fill.
 */
void AboutWindow::setAboutText(QTextBrowser *textbox)
{ //hardcoded for easy access to version
	textbox->setHtml("<a href=\"https://models.slf.ch/p/inishell-ng/\"><center><img src=\":/icons/inishell_192.ico\" height=\"92\" width=\"92\"></a></center> \
	    <center><b>INIshell version " + QString(APP_VERSION_STR) + "</b><br> \
	    &copy; WSL-Institut für Schnee-und Lawinenforschung <a href=\"https://www.slf.ch\">SLF</a> 2019-2020<br> \
	    <a href=\"https://models.slf.ch/p/inishell-ng/\">Project page</a> &middot; <a href=\"https://models.slf.ch/p/inishell-ng/issues/\">Bug tracker</a> &middot; <a href=\"https://models.slf.ch/p/inishell-ng/source/tree/master/\">Source code</a><br></center> \
	    Original version: <i>Michael Reisecker, 2019 - 2020</i><br> \
	    Inspired by INIshell v1: <i>Mathias Bavay, Thomas Egger & Daniela Korhammer, 2011</i><br> \
		Built with <a href=\"https://www.qt.io/\">Qt</a>,  \
	    arithmetic evaluations by <a href=\"https://github.com/codeplea/tinyexpr\">tinyexpr</a>, lines numbering from Qt under a <a href=\"https://opensource.org/licenses/BSD-3-Clause\">BSD</a> license.<br> \
	    Icons by <a href=\"https://github.com/elementary/icons\">Elementary</a>, <a href=\"https://github.com/KDE/breeze-icons\">Breeze-icons</a> and <a href=\"https://icons8.com\">Icons8</a>.<br><hr> \
	    <center><i>INIshell is free software: you can redistribute it and/or modify \
	    it under the terms of the \
	    <b><a href=\"http://www.gnu.org/licenses/\">GNU General Public License</a></b> \
	    as published by the Free Software Foundation, \
	    either version 3 of the License, or \
	    (at your option) any later version.<br><br> \
	    INIshell is distributed in the hope that \
	    it will be useful, but \
	    WITHOUT ANY WARRANTY; without even the implied warranty of \
	    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \
	    GNU General Public License for more details.<br><br> \
	    <a href=\"https://www.slf.ch\"><img src=\":/icons/slf.svg\" height=\"92\" width=\"92\"></a></center> \
	");
}
