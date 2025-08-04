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
 * The PreviewWindow shows the current INI file and offers to save it.
 * 2019-11
 */

#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include "src/main/INIParser.h"
#include "src/main/colors.h"
#include "src/gui/PreviewEdit.h"

#include <QAction>
#include <QLineEdit>
#include <QMainWindow>
#include <QToolButton>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QSyntaxHighlighter>
#include <QTabWidget>
#include <QTextDocument>
#include <QPlainTextEdit>
#include <QVector>
#include <QWidget>

class EditorKeyPressFilter : public QObject { //detect key press events in the text editor
	public:
		bool eventFilter(QObject *object, QEvent *event) override;
};

class SyntaxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

	public:
		SyntaxHighlighter(QTextDocument *textdoc = nullptr);

	protected:
		void highlightBlock(const QString &text) override;

	private:
		struct HighlightingRule {
			QRegularExpression pattern;
			QTextCharFormat format;
		};
		QVector<HighlightingRule> rules_;
};

class PreviewWindow : public QMainWindow
{
	Q_OBJECT

	public:
		explicit PreviewWindow(QMainWindow *parent = nullptr);
		~PreviewWindow() override;
		PreviewWindow(const PreviewWindow&) = delete; //these would be for multi-window-previews
		PreviewWindow& operator =(PreviewWindow const&) = delete;
		PreviewWindow(PreviewWindow&&) = delete;
		PreviewWindow& operator=(PreviewWindow&&) = delete;
		void addIniTab(const QString& infile = QString());
		int count() const { return file_tabs_->count(); }

	protected:
		void closeEvent(QCloseEvent* event) override;
		void keyPressEvent(QKeyEvent *event) override;
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dropEvent(QDropEvent *event) override;

	private:
		enum insert_text { //options for the insert menu
			HEADER,
			MISSING,
			MISSING_MANDATORY
		};
		enum transform_whitespaces { //options for transformations
			SINGLE_WS,
			LONGEST_WS
		};
		enum transform_capitalization {
			SECTIONS_UPPER,
			SECTIONS_LOWER,
			KEYS_UPPER,
			KEYS_LOWER,
			VALUES_UPPER,
			VALUES_LOWER,
			UPPER_CASE,
			LOWER_CASE
		};
		enum transform_comments {
			BLOCK_COMMENT,
			BLOCK_UNCOMMENT,
			ALL_CONTENT,
			DUPLICATE,
			MOVE_TO_VALUES,
			MOVE_TO_END,
			TRIM,
			DELETE,
			CONVERT_NUMBERSIGN,
			CONVERT_SEMICOLON
		};
		enum convert_tabs {
			LONG_SPACES_TO_TABS,
			SHORT_SPACES_TO_TABS,
			TABS_TO_LONG_SPACES,
			TABS_TO_SHORT_SPACES
		};

		void closeTab(int index);
		void createMenu();
		void createFindBar();
		void showFindBar();
		void hideFindBar();
		void textChanged(const int &index);
		void loadIniWithGui();
		void writeIniToFile(const QString &file_name);
		int warnOnUnsavedIni();
		void previewStatus(const QString &text);
		PreviewEdit * getCurrentEditor();
		QString getCurrentFilename() const;
		void setTextWithHistory(QPlainTextEdit *editor, const QString& text);
		void insertText(const insert_text &mode);
		void transformWhitespaces(const transform_whitespaces &mode);
		void transformCapitalization(const transform_capitalization &mode);
		bool transformComments(const transform_comments &mode);
		void convertTabs(const convert_tabs &mode);
		int getCurrentLineNumber();
		int getNrOfSelectedLines();
		void getSelectionMargins(int &first_line, int &last_line);
		void pasteToNewline();
		QString trimComment(const QString &comment) const;
		inline QString carryIfContent(const QString &text)
		{
			return text.isEmpty()? QString() : text + "\n";
		}
		inline QString convertPrefix(const QString &text, const bool numbers_sign)
		{
			QStringList lines( text.split("\n") );
			for (auto &lin : lines) {
				if (!lin.isEmpty()) //catch newline at end
					lin.replace(0, 1, numbers_sign? "#" : ";");
			}
			return lines.join("\n");

		}

		INIParser preview_ini_; //our local INIParser to do transformations on
		EditorKeyPressFilter *editor_key_filter_ = nullptr;
		QTabWidget *file_tabs_ = nullptr;
		SyntaxHighlighter *highlighter_ = nullptr;
		QLineEdit *find_text_ = nullptr;
		QToolButton *close_find_bar_ = nullptr;
		QAction *file_save_and_load_ = nullptr;
		QAction *file_load_ = nullptr;
		QAction *edit_insert_missing_ = nullptr;
		QAction *edit_insert_missing_mandatory_ = nullptr;
		QAction *transform_reset_full_ = nullptr;
		static const int paragraph_separator = 0x2029; //UNICODE paragraph separator
		static const int long_spaces_for_tabs = 8; //8 spaces ~ 1 tab
		static const int short_spaces_for_tabs = 4; //4 spaces ~ 1 tab
		unsigned int unsaved_ini_counter = 1;
		bool has_sorted_alphabetically_ = false;

	private slots:
		void openFile();
		void saveFile();
		void saveFileAs();
		void saveFileAndLoadIntoGui();
		void loadIntoGui();
		void quickBackup();
		void onFindTextChanged(const QString &text);
		void onInsertMenuClick(const QString &action);
		void onTransformMenuClick(const QString &action);
		void onConvertMenuClick(const QString &action);
		void onShowWhitespacesMenuClick(const bool &show_ws);
};

#endif //PREVIEWWINDOW_H
