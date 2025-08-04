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

#include "TerminalView.h"
#include "src/main/inishell.h"

#include <QAction>
#include <QDir>
#include <QMenu>
#include <QVBoxLayout>

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @brief Default constructor for a TerminalView.
 * @details This constructor creates a text box for terminal output.
 * @param[in] parent The TerminalView's parent window.
 */
TerminalView::TerminalView(QWidget *parent) : QWidget(parent)
{
	/* text box for terminal output */
	console_ = new QTextEdit;
	console_->setReadOnly(true);
	//else, a right-click on our panel doesn't trigger the event:
	console_->setContextMenuPolicy(Qt::NoContextMenu);
	this->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, &TerminalView::onContextMenuRequest);
	console_->setToolTip(tr("Workflow console output"));
	log(html::color(html::bold("$ " + QDir::currentPath()), "normal"));

	/* main layout */
	auto *main_layout = new QVBoxLayout;
	main_layout->addWidget(console_);
	this->setLayout(main_layout);
}

/**
 * @brief Log text to the terminal text box.
 * @param[in] text The text to log.
 * @param[in] is_std_err True if the text comes from stderr, false if from stdout.
 */
void TerminalView::log(const QString &text, const bool &is_std_err)
{
	if (!text.contains("\033[")) { //no extra styling given: use error color
		console_->moveCursor(QTextCursor::End);
		if (is_std_err)
			console_->insertHtml(html::color(html::bold(text + "<br>"), "error"));
		else //output stems from stdout, i. e. all is well
			console_->insertHtml(text + "<br>");
	} else { //text styling requested from the output stream
		QString tmp( text ); //support terminal color-style
		tmp.replace("\033[01;30m", "<span style=\"color:#93a1a1;\">"); //solarized base1
		tmp.replace("\033[31;1m", "<span style=\"color:#dc322f; font-weight: bold;\">"); //solarized red
		tmp.replace("\033[4m", "<span style=\"text-decoration: underline;\">");
		tmp.replace("\033[3m", "<span style=\"color:#93a1a1; font-style: italic;\">"); //solarized base1
		//tmp.replace("\033[23m", "<span style=\"font-style: normal;\">");
		tmp.replace("\033[23m", "</span>");
		tmp.replace("\033[0m", "</span>");
		tmp.replace("\n", "<br>");
		//to prevent inserting new lines before all appends, it is necessary␊
		//to move the cursor to the end and insert the text:
		console_->moveCursor(QTextCursor::End);
		console_->insertHtml(tmp + "<br>");
	}
	console_->ensureCursorVisible();
}

/**
 * @brief Show an extended context menu for the TerminalView.
 * @param[in] pos Mouse click position.
 */
void TerminalView::onContextMenuRequest(const QPoint &pos)
{
	QMenu *std_menu( console_->createStandardContextMenu(pos) );
	std_menu->addAction(tr("Clear"));
	QAction *selected( std_menu->exec(QCursor::pos()) );
	if (selected) {
		if (selected->text() == tr("Clear")) {
			console_->setText(QString());
			topStatus(QString());
		}
	}
}
