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

#include <QAction>
#include <QString>
#include <QTextDocument>
#include <QPlainTextEdit>
#include <QWidget>

class PreviewEdit : public QPlainTextEdit
{
	Q_OBJECT

	public:
		PreviewEdit(const bool& monospace);

		void repaintSidePanel(QPaintEvent *event);
		int getSidePanelWidth();

	protected:
		void setMonospacedFont();
		void resizeEvent(QResizeEvent *event) override;
		void dragMoveEvent(QDragMoveEvent *event) override;

	private slots:
		void updateSidePanelWidth();
		void updateSidePanel(const QRect &, int);

	private:
		QWidget *sidePanel;
};


class PreviewSidePanel : public QWidget
{
	Q_OBJECT

	public:
		PreviewSidePanel(PreviewEdit *editor) : QWidget(editor), textEdit(editor) {}

		QSize sizeHint() const
		{
			return QSize(textEdit->getSidePanelWidth(), 0);
		}

	protected:
		void paintEvent(QPaintEvent *event)
		{
			textEdit->repaintSidePanel(event);
		}

	private:
		PreviewEdit *textEdit;
};
