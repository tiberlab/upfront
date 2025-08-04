/****************************************************************************
** This file is based on the line numbering example in Qt's documentation at
** https://doc.qt.io/qt-5/qtwidgets-widgets-PreviewEdit-example.html.
** It is lincensed under a BSD license (see original text below).
** Changes by M. Bavay, 2020-03-20 for WSL/SLF
**/

/**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** BSD License Usage
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
*/

#include "PreviewEdit.h"
#include "src/main/colors.h"

#include <QMainWindow>
#include <QtWidgets>

PreviewEdit::PreviewEdit(const bool& monospace) : QPlainTextEdit()
{
	if (monospace) setMonospacedFont();
	setToolTip("Some of the supported shortcuts:\nCtrl+K \t\t\t Delete to the end of the line\nCtrl+Z / Y \t\t Undo / Redo\nCtrl+Tab / Shift+Tab \t Move to next / previous tab");
	sidePanel = new PreviewSidePanel(this);
	sidePanel->setStyleSheet("QWidget {background-color: " + colors::getQColor("syntax_background").name() + "; color: " +
	    colors::getQColor("sl_base01").name() + "; font-style: italic; font-size: 9pt}");

	connect(this, &PreviewEdit::blockCountChanged, this, &PreviewEdit::updateSidePanelWidth);
	connect(this, &PreviewEdit::updateRequest, this, &PreviewEdit::updateSidePanel);

	updateSidePanelWidth();
}

/**
 * @brief Try to set a monospaced font for files that are space indented.
 */
void PreviewEdit::setMonospacedFont()
{
	QFont mono_font( QFontDatabase::systemFont(QFontDatabase::FixedFont) ); //system recommendation
	mono_font.setPointSize( this->font().pointSize() );
	this->setFont(mono_font);
}

int PreviewEdit::getSidePanelWidth()
{
	//since log10 returns 0 for numbers <10, add 1. For a nicer look, we add 0.5 char's width
	const double digits = floor(log10(qMax(1, blockCount()))) + 1.5;
	const int width = static_cast<int>(fontMetrics().boundingRect(QLatin1Char('0')).width() * digits);
	return width;
}

void PreviewEdit::updateSidePanelWidth()
{
	//we add a small margin between the line number and the line itself
	const int margin_width = fontMetrics().boundingRect(QLatin1Char('0')).width() / 2;
	setViewportMargins(getSidePanelWidth()+margin_width, 0, 0, 0);
}

void PreviewEdit::updateSidePanel(const QRect &rect, int dy)
{
	if (dy)
		sidePanel->scroll(0, dy);
	else
		sidePanel->update(0, rect.y(), sidePanel->width(), rect.height());

	if (rect.contains(viewport()->rect()))
		updateSidePanelWidth();
}

void PreviewEdit::resizeEvent(QResizeEvent *event)
{
	QPlainTextEdit::resizeEvent(event);
	const QRect cr( contentsRect() );
	sidePanel->setGeometry(QRect(cr.left(), cr.top(), getSidePanelWidth(), cr.height()));
}

/**
 * @brief Show info when users drag files over the editor.
 * @details We let the text editor handle the drag and drop behaviour. However, we give notice that
 * dropping files in an area outside of the text editor itself will open the files.
 * @param[in] event The drag move event.
 */
void PreviewEdit::dragMoveEvent(QDragMoveEvent *event)
{
	const QMainWindow* preview_window(qobject_cast<QMainWindow*>(this->parent()->parent()->parent())); //QStackedWidget->QTabWidget->PreviewWindow
	if (preview_window) {
		if (event->mimeData()->hasUrls()) {
			preview_window->statusBar()->showMessage(tr("Drop files over the menu or tab titles to open."));
			preview_window->statusBar()->show(); //might be hidden from closing search bar
		}
	}

	event->acceptProposedAction(); //progress as normal
	
	//Note: overriding QDragENTERevent instead causes "QDragManager::drag in possibly invalid state" for no apparent reason,
	//but this here should be good.
}

void PreviewEdit::repaintSidePanel(QPaintEvent *event)
{
	QPainter painter(sidePanel);
	QTextBlock block( firstVisibleBlock() );
	int lineNumber = block.blockNumber();
	int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
	int bottom = top + static_cast<int>(blockBoundingRect(block).height());

	while (block.isValid() && top <= event->rect().bottom()) {
		if (block.isVisible() && bottom >= event->rect().top()) {
			const QString number( QString::number(lineNumber + 1) );
			painter.drawText(0, top, sidePanel->width(), fontMetrics().height(), Qt::AlignRight | Qt::AlignVCenter, number);
		}

		block = block.next();
		top = bottom;
		bottom = top + static_cast<int>(blockBoundingRect(block).height());
		++lineNumber;
	}
}
