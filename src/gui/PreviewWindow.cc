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

#include "PreviewWindow.h"
#include "src/gui_elements/Atomic.h"
#include "src/main/colors.h"
#include "src/main/common.h"
#include "src/main/os.h"
#include "src/main/dimensions.h"
#include "src/main/inishell.h"
#include "src/main/settings.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>
#include <QHostInfo>
#include <QKeySequence>

#include <vector>

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class KeyPressFilter
 * @brief Key press event listener for the preview text editors.
 * @param[in] object Object the event stems from (the Text Editor).
 * @param[in] event The type of event.
 * @return True if the event was accepted.
 */
bool EditorKeyPressFilter::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		auto *key_event = static_cast<QKeyEvent *>(event);
		if (keyToSequence(key_event) == QKeySequence::Cut) {
			//cut whole line on empty selection:
			auto *current_editor( static_cast<QPlainTextEdit *>(object) );
			current_editor->moveCursor(QTextCursor::StartOfLine);
			current_editor->moveCursor(QTextCursor::Down, QTextCursor::KeepAnchor);
			current_editor->cut();
			//our convention: cut the line feed, but don't paste it back
			//(convenient shortcut combination to move keys around)
			QClipboard *clipboard = QGuiApplication::clipboard();
			QString clip_text( clipboard->text() );
			clip_text.chop(1); //remove trailing newline
			clipboard->setText(clip_text);
			event->accept();
			return true;
		}
	}
	return QObject::eventFilter(object, event); //pass to actual event of the object
}

/**
 * @class SyntaxHighlighter
 * @brief Default constructor for a syntax highlighter used in the INI preview.
 * @param[in] textdoc Text document to handle syntax highlighting for.
 */
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *textdoc) : QSyntaxHighlighter(textdoc)
{
	HighlightingRule rule;

	/*
	 * The SyntaxHighlighter parses a document for INI file syntax and checks the sections
	 * and keys against the curently loaded XML. Sections and keys that are not present
	 * in the XML are highlighted differently.
	 */

	/* unknown sections */
	QTextCharFormat format_section;
	format_section.setForeground(colors::getQColor("syntax_unknown_section"));
	format_section.setFontWeight(QFont::Bold);
	rule.pattern = QRegularExpression(R"(.*\)" + Cst::section_open + R"(.*\)" + Cst::section_close + R"(.*)");
	rule.format = format_section;
	rules_.append(rule);

	/* unknown keys */
	QTextCharFormat format_unknown_key;
	format_unknown_key.setForeground(colors::getQColor("syntax_unknown_key"));
	rule.pattern = QRegularExpression(R"(^\s*[\w|:|*]+(?=\s*=))"); //TODO: only :: but not :
	//TODO: collect and unify all regex handling (e. g. use the same here as in INIParser)
	rule.format = format_unknown_key;
	rules_.append(rule);

	/* INI values */
	QTextCharFormat format_value;
	format_value.setForeground(colors::getQColor("syntax_value"));
	rule.pattern = QRegularExpression(R"((?<=\=).*)");
	rule.format = format_value;
	rules_.append(rule);


	/* populate highlighter with known sections and keys */
	QTextCharFormat format_known_key;
	format_known_key.setForeground(colors::getQColor("syntax_known_key"));
	QTextCharFormat format_known_section;
	format_known_section.setForeground(colors::getQColor("syntax_known_section"));
	format_known_section.setFontWeight(QFont::Bold);

	const QList<Atomic *> panel_list( getMainWindow()->findChildren<Atomic *>() );
	for (auto &panel : panel_list) {
		if (panel->property("no_ini").toBool()) //e. g. Groups / frames
			continue;
		QString section, key;
		(void) panel->getIniValue(section, key);
		key.replace("*", "\\*");
		rule.pattern = QRegularExpression("\\" + Cst::section_open + section + "\\" +
		    Cst::section_close, QRegularExpression::CaseInsensitiveOption); //TODO: escape only if needed for the set char
		rule.format = format_known_section;
		rules_.append(rule);
		rule.pattern = QRegularExpression(R"(^\s*)" + key + R"((=|\s))", QRegularExpression::CaseInsensitiveOption);
		rule.format = format_known_key;
		rules_.append(rule);
	}

	/* comments */
	QTextCharFormat format_block_comment;
	format_block_comment.setForeground(colors::getQColor("syntax_comment"));
	rule.pattern = QRegularExpression(R"(^\s*[#;].*)");
	rule.format = format_block_comment;
	rules_.append(rule);
	rule.pattern = QRegularExpression(R"(([#;](?!$).*))");
	rules_.append(rule);

	/* coordinates */
	QTextCharFormat format_coordinate;
	format_coordinate.setForeground(colors::getQColor("coordinate"));
 	rule.pattern = QRegularExpression(R"((latlon|xy)\s*\(([-\d\.]+)(?:,)\s*([-\d\.]+)((?:,)\s*([-\d\.]+))?\))");
	rule.format = format_coordinate;
	rules_.append(rule);
	
	/* equals sign */
	QTextCharFormat format_equal;
	format_equal.setForeground(Qt::black);
	rule.pattern = QRegularExpression(R"(=)");
	rule.format = format_equal;
	rules_.append(rule);
}

/**
 * @brief Send a chunk of text through the syntax highlighter.
 * @param[in] text The text to syntax-highlight.
 */
void SyntaxHighlighter::highlightBlock(const QString &text)
{
	for (const HighlightingRule &rule : qAsConst(rules_)) {
		QRegularExpressionMatchIterator mit( rule.pattern.globalMatch(text) );
		while (mit.hasNext()) { //run trough regex matches and set the stored formats
			QRegularExpressionMatch match = mit.next();
			setFormat(match.capturedStart(), match.capturedLength(), rule.format);
		}
	}
}

/**
 * @class PreviewWindow
 * @brief Default constructor for the PreviewWindow.
 * @details This constructor creates a tab bar to show multiple INI file versions in.
 * @param [in] parent The PreviewWindow's parent window (the main window).
 */
PreviewWindow::PreviewWindow(QMainWindow *parent) : QMainWindow(parent),
    preview_ini_(getMainWindow()->getLogger())
{
	editor_key_filter_ = new EditorKeyPressFilter;
	this->setUnifiedTitleAndToolBarOnMac(true);
	file_tabs_ = new QTabWidget;
	connect(file_tabs_, &QTabWidget::tabCloseRequested, this, &PreviewWindow::closeTab);
	
	file_tabs_->setTabsClosable(true);
	this->setCentralWidget(file_tabs_);
	createMenu();
	setAcceptDrops(true);

	//size this window (from "outside" via a timer to have it work on macOS; works without for the logger?):
	QTimer::singleShot(1, [=]{ dim::setDimensions(this, dim::PREVIEW); });
	this->setWindowTitle(tr("Preview") + " ~ " + QCoreApplication::applicationName());
	createFindBar(); //do last to keep status bar hidden
	statusBar()->hide();
}

/**
 * @brief Destructor with minimal cleanup.
 */
PreviewWindow::~PreviewWindow()
{
	delete editor_key_filter_;
}

/**
 * @brief Display the current INI file in a new tab.
 * @param[in] infile Optional file name to load instead of the GUI values.
 */
void PreviewWindow::addIniTab(const QString& infile)
{
	const bool fromGUI = infile.isNull();
	/* get currently set INI values */
	QString ini_contents;
	QTextStream ss(&ini_contents);
	if (fromGUI) //load INI from GUI
		loadIniWithGui(); //extend original file's INIParser with GUI values
	else
		preview_ini_.parseFile(infile); //load INI from file system

	/* text box for the current INI */
	const bool setMonospace = (getSetting("user::preview::mono_font", "value") == "TRUE");
	auto *preview_editor( new PreviewEdit(setMonospace) );
	preview_editor->installEventFilter(editor_key_filter_);
	preview_editor->setStyleSheet("QPlainTextEdit {background-color: " + colors::getQColor("syntax_background").name() + "; color: " + colors::getQColor("syntax_invalid").name() + "}");

	highlighter_ = new SyntaxHighlighter(preview_editor->document());

	/* display the current GUI's contents */
	preview_ini_.outputIni(ss);
	if (ini_contents.isEmpty()) {
		ini_contents = tr("#Empty INI file\n");
		previewStatus(tr("Open an application and load an INI file to view contents"));
	} else {
		previewStatus(QString());
		this->statusBar()->hide();
	}
	preview_editor->setPlainText(ini_contents); //without undo history --> can't undo to empty
	if (!fromGUI) preview_editor->setReadOnly(true); //INI read directly from file are RO

	const QString loaded_file = fromGUI? getMainWindow()->getIni()->getFilename() : infile;
	const QFileInfo file_info(loaded_file);
	QString file_name(file_info.fileName());
	QString file_path;
	if (file_info.exists()) //avoid warning
		file_path = file_info.absolutePath();
	if (file_name.isEmpty()) { //pick file name if no INI is opened yet
		file_name = "unsaved("+QString::number(unsaved_ini_counter)+")";
		unsaved_ini_counter++;
	} else if (fromGUI) { //only for ini files coming from the GUI
		INIParser gui_ini = getMainWindow()->getIniCopy();
		(void) getMainWindow()->getControlPanel()->setIniValuesFromGui(&gui_ini);
		if (getMainWindow()->getIniCopy() != gui_ini)
			file_name += " *"; //asterisk for "not saved yet", unless it's only the info text
	}
	if (file_path.isEmpty())
		file_path = QDir::currentPath();
	file_tabs_->addTab(preview_editor, file_name);
	file_tabs_->setTabToolTip(file_tabs_->count() - 1, file_path);
	file_tabs_->setCurrentIndex(file_tabs_->count() - 1); //switch to new tab
	connect(preview_editor, &QPlainTextEdit::textChanged, this, [=]{ textChanged(file_tabs_->count() - 1); });

	onShowWhitespacesMenuClick((getSetting("user::preview::show_ws", "value") == "TRUE"));

} //TODO: tab completion for INI keys

/**
 * @brief Event listener for when the window is being closed.
 * @details This function allows the user to cancel the closing if there are unsaved
 * changes in the opened INI files.
 * @param[in] event The close event.
 */
void PreviewWindow::closeEvent(QCloseEvent *event)
{
	bool has_unsaved_changes = false; //run through tabs and check for asterisks
	for (int ii = 0; ii < file_tabs_->count(); ++ii) {
		if (file_tabs_->tabText(ii).endsWith("*")) {
			has_unsaved_changes = true;
			break;
		}
	}
	if (has_unsaved_changes &&
	    getSetting("user::inireader::warn_unsaved_ini", "value") == "TRUE") { //at least one tab has unsaved changes
		const int cancel = warnOnUnsavedIni();
		if (cancel == QMessageBox::Cancel) {
			event->ignore();
			return;
		}
	}
	event->accept();
}

/**
 * @brief Event listener for key presses.
 * @details Close the Preview, add tab, or show the logger.
 * @param event The key press event that is received.
 */
void PreviewWindow::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape) {
		hideFindBar();
	} else if (keyToSequence(event) == QKeySequence::Print) {
		addIniTab();
	} else if (event->modifiers() == Qt::CTRL && event->key() == Qt::Key_L) {
		getMainWindow()->getLogger()->show();
		getMainWindow()->getLogger()->raise();
	}
}

/**
 * @brief Event listener for drag events.
 * @details This function reacts if a user starts dragging content into the Preview Editor
 * and accepts the dragging.
 * @param[in] event The drag event.
 */
void PreviewWindow::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

/**
 * @brief Event listener for drop events.
 * @details This is an event listener for the parent preview window, not the text editor itself.
 * As such, the text editor will sill handle its drag & drop functionality accordingly. In order
 * for users to be able to drop files, they need to drop it ouside of the text area. When dragging
 * into the text area however, an info status about this is displayed.
 * @param[in] event The drop event.
 */
void PreviewWindow::dropEvent(QDropEvent *event)
{
	for (const QUrl &url : event->mimeData()->urls()) //run through all files
		addIniTab(url.toLocalFile());
}

/**
 * @brief Close an INI file's tab.
 * @param[in] index Index of the tab that is being closed.
 */
void PreviewWindow::closeTab(int index)
{
	//Check for unsaved changes. Note that changes that cancel out leaving the INI file
	//unaltered will still trigger the warning (unlike the GUI).
	if (file_tabs_->tabText(index).endsWith("*") &&
	    getSetting("user::inireader::warn_unsaved_ini", "value") == "TRUE") { //not saved yet
		const int cancel = warnOnUnsavedIni();
		if (cancel == QMessageBox::Cancel)
			return;
	}

	file_tabs_->removeTab(index);
	if (file_tabs_->count() == 0)
		this->close();
}

/**
 * @brief Create the PreviewWindow's menu.
 */
void PreviewWindow::createMenu()
{
	/* FILE menu */
	QMenu *menu_file = this->menuBar()->addMenu(tr("&File"));
	auto *file_open = new QAction(getIcon("document-open"), tr("&Open..."), menu_file);
	file_open->setShortcut( QKeySequence::Open );
	menu_file->addAction(file_open);
	connect(file_open, &QAction::triggered, this, &PreviewWindow::openFile);
	menu_file->addSeparator();
	auto *file_save = new QAction(getIcon("document-save"), tr("&Save"), menu_file);
	file_save->setShortcut( QKeySequence::Save );
	menu_file->addAction(file_save);
	connect(file_save, &QAction::triggered, this, &PreviewWindow::saveFile);
	auto *file_save_as = new QAction(getIcon("document-save-as"), tr("Save &as..."), menu_file);
	file_save_as->setShortcut( QKeySequence::SaveAs );
	menu_file->addAction(file_save_as);
	connect(file_save_as, &QAction::triggered, this, &PreviewWindow::saveFileAs);
	menu_file->addSeparator();
	file_save_and_load_ = new QAction(tr("Save and load into GUI"), menu_file);
	menu_file->addAction(file_save_and_load_);
	connect(file_save_and_load_, &QAction::triggered, this, &PreviewWindow::saveFileAndLoadIntoGui);
	file_load_ = new QAction(tr("Load into GUI"), menu_file);
	menu_file->addAction(file_load_);
	connect(file_load_, &QAction::triggered, this, &PreviewWindow::loadIntoGui);
	menu_file->addSeparator();
	auto *file_backup = new QAction(tr("Quicksave backup"), menu_file);
	menu_file->addAction(file_backup);
	file_backup->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_B);
	connect(file_backup, &QAction::triggered, this, &PreviewWindow::quickBackup);

	/* EDIT menu */
	QMenu *menu_edit = this->menuBar()->addMenu(tr("&Edit"));
	auto *edit_undo = new QAction(getIcon("edit-undo"), tr("Undo"), menu_edit);
	menu_edit->addAction(edit_undo);
	edit_undo->setShortcut(QKeySequence::Undo);
	connect(edit_undo, &QAction::triggered, this, [=]{ getCurrentEditor()->undo(); });
	auto *edit_redo = new QAction(getIcon("edit-redo"), tr("Redo"), menu_edit);
	menu_edit->addAction(edit_redo);
	edit_redo->setShortcut(QKeySequence::Redo);
	connect(edit_redo, &QAction::triggered, this, [=]{ getCurrentEditor()->redo(); });
	menu_edit->addSeparator();
	auto *edit_cut = new QAction(getIcon("edit-cut"), tr("Cut"), menu_edit);
	menu_edit->addAction(edit_cut);
	edit_cut->setShortcut(QKeySequence::Cut);
	connect(edit_cut, &QAction::triggered, this, [=]{ getCurrentEditor()->cut(); });
	auto *edit_copy = new QAction(getIcon("edit-copy"), tr("Copy"), menu_edit);
	menu_edit->addAction(edit_copy);
	edit_copy->setShortcut(QKeySequence::Copy);
	connect(edit_copy, &QAction::triggered, this, [=]{ getCurrentEditor()->copy(); });
	auto *edit_paste = new QAction(getIcon("edit-paste"), tr("Paste"), menu_edit);
	menu_edit->addAction(edit_paste);
	edit_paste->setShortcut(QKeySequence::Paste);
	connect(edit_paste, &QAction::triggered, this, [=]{ getCurrentEditor()->paste(); });
	auto *edit_paste_newline = new QAction(tr("Paste to new line"), menu_edit);
	menu_edit->addAction(edit_paste_newline);
	edit_paste_newline->setShortcut("Alt+" +
	    QKeySequence(QKeySequence::Paste).toString(QKeySequence::NativeText));
	connect(edit_paste_newline, &QAction::triggered, this, [=]{ pasteToNewline(); });
	auto *edit_select_all = new QAction(getIcon("edit-select-all"), tr("Select all"), menu_edit);
	menu_edit->addAction(edit_select_all);
	edit_select_all->setShortcut(QKeySequence::SelectAll);
	connect(edit_select_all, &QAction::triggered, this, [=]{ getCurrentEditor()->selectAll(); });
	menu_edit->addSeparator();
	auto *edit_find = new QAction(getIcon("edit-find"), tr("&Find text..."), menu_edit);
	menu_edit->addAction(edit_find);
	edit_find->setShortcut(QKeySequence::Find);
	connect(edit_find, &QAction::triggered, this, &PreviewWindow::showFindBar);

	/* INSERT menu */
	QMenu *menu_insert = this->menuBar()->addMenu(tr("&Insert"));
	auto *edit_insert_header = new QAction(tr("Comment header"), menu_insert);
	menu_insert->addAction(edit_insert_header);
	connect(edit_insert_header, &QAction::triggered, this,
	    [=]{ onInsertMenuClick("insert_header"); });
	menu_insert->addSeparator();
	edit_insert_missing_ = new QAction(tr("Missing keys for GUI"), menu_insert);
	menu_insert->addAction(edit_insert_missing_);
	connect(edit_insert_missing_, &QAction::triggered, this,
	    [=]{ onInsertMenuClick("insert_missing"); });
	edit_insert_missing_mandatory_ = new QAction(tr("Mandatory keys for GUI"),
	     menu_insert);
	menu_insert->addAction(edit_insert_missing_mandatory_);
	connect(edit_insert_missing_mandatory_, &QAction::triggered, this,
	    [=]{ onInsertMenuClick("insert_missing_mandatory"); });

	/* TRANSFORM menu */
	QMenu *menu_transform = this->menuBar()->addMenu(tr("&Transform"));
	/* Whitespaces menu */
	auto *transform_whitespaces = new QMenu(tr("Whitespaces"), menu_transform);
	transform_whitespaces->setIcon( getIcon("markasblank") );
	menu_transform->addMenu(transform_whitespaces);
	auto *transform_transform_singlews = new QAction(getIcon("unmarkasblank"), tr("To single spaces"), transform_whitespaces);
	connect(transform_transform_singlews, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_whitespace_singlews"); } );
	transform_whitespaces->addAction(transform_transform_singlews);
	auto *transform_transform_longestws = new QAction(tr("Adapt to longest keys"),
	    transform_whitespaces);
	connect(transform_transform_longestws, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_whitespace_longestws"); } );
	transform_whitespaces->addAction(transform_transform_longestws);
	/* Sort */
	auto *transform_sort = new QMenu(tr("Sort"), menu_transform);
	transform_sort->setIcon( getIcon("view-sort") );
	menu_transform->addMenu(transform_sort);
	auto *transform_sort_alphabetically = new QAction(tr("Alphabetically"), transform_sort);
	connect(transform_sort_alphabetically, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_sort_alphabetically"); } );
	transform_sort->addAction(transform_sort_alphabetically);
	auto *transform_sort_order = new QAction(tr("In order of INI file"), transform_sort);
	connect(transform_sort_order, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_sort_order"); } );
	transform_sort->addAction(transform_sort_order);
	/* Capitalization menu */
	auto *transform_capitalization = new QMenu(tr("Capitalization"), menu_transform);
	menu_transform->addMenu(transform_capitalization);
	auto *transform_capitalization_sections_upper = new QAction(
	    tr("Sections to upper case"), transform_capitalization);
	connect(transform_capitalization_sections_upper, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_capitalization_sections_upper"); } );
	transform_capitalization->addAction(transform_capitalization_sections_upper);
	auto *transform_capitalization_sections_lower = new QAction(
	    tr("Sections to lower case"), transform_capitalization);
	connect(transform_capitalization_sections_lower, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_capitalization_sections_lower"); } );
	transform_capitalization->addAction(transform_capitalization_sections_lower);
	auto *transform_capitalization_keys_upper = new QAction(
	    tr("Keys to upper case"), transform_capitalization);
	connect(transform_capitalization_keys_upper, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_capitalization_keys_upper"); } );
	transform_capitalization->addAction(transform_capitalization_keys_upper);
	auto *transform_capitalization_keys_lower = new QAction(
	    tr("Keys to lower case"), transform_capitalization);
	connect(transform_capitalization_keys_lower, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_capitalization_keys_lower"); } );
	transform_capitalization->addAction(transform_capitalization_keys_lower);
	auto *transform_capitalization_values_upper = new QAction(
	    tr("Values to upper case"), transform_capitalization);
	connect(transform_capitalization_values_upper, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_capitalization_values_upper"); } );
	transform_capitalization->addAction(transform_capitalization_values_upper);
	auto *transform_capitalization_values_lower = new QAction(
	    tr("Values to lower case"), transform_capitalization);
	connect(transform_capitalization_values_lower, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_capitalization_values_lower"); } );
	transform_capitalization->addAction(transform_capitalization_values_lower);
	transform_capitalization->addSeparator();
	auto *transform_capitalization_upper = new QAction(getIcon("format-text-uppercase"),
	    tr("All to upper case"), transform_capitalization);
	connect(transform_capitalization_upper, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_capitalization_upper"); } );
	transform_capitalization->addAction(transform_capitalization_upper);
	auto *transform_capitalization_lower = new QAction(getIcon("format-text-lowercase"),
	    tr("All to lower case"), transform_capitalization);
	connect(transform_capitalization_lower, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_capitalization_lower"); } );
	transform_capitalization->addAction(transform_capitalization_lower);
	/* Comments menu */
	auto *transform_comments = new QMenu(tr("Comments"), menu_transform);
	transform_comments->setIcon( getIcon("code-context") );
	menu_transform->addMenu(transform_comments);
	auto *transform_comment_block = new QAction(tr("Comment selection"));
	transform_comments->addAction(transform_comment_block);
	transform_comment_block->setShortcut(Qt::CTRL + Qt::Key_NumberSign);
	connect(transform_comment_block, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comment_block"); });
	auto *transform_uncomment_block = new QAction(tr("Uncomment selection"));
	transform_comments->addAction(transform_uncomment_block);
	transform_uncomment_block->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_NumberSign);
	connect(transform_uncomment_block, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_uncomment_block"); });
	transform_comments->addSeparator();
	auto *transform_comments_content = new QAction(tr("Comment all content"),
	    transform_comments);
	connect(transform_comments_content, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comments_content"); } );
	transform_comments->addAction(transform_comments_content);
	auto *transform_comments_duplicate = new QAction(tr("Duplicate all to comment"),
	    transform_comments);
	transform_comments_duplicate->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_B);
	connect(transform_comments_duplicate, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comments_duplicate"); } );
	transform_comments->addAction(transform_comments_duplicate);
	auto *transform_comments_move_values = new QAction(tr("Move next to values"),
	    transform_comments);
	connect(transform_comments_move_values, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comments_move_value"); } );
	transform_comments->addAction(transform_comments_move_values);
	auto *transform_comments_move_end = new QAction(tr("Collect at bottom"),
	    transform_comments);
	connect(transform_comments_move_end, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comments_move_end"); } );
	transform_comments->addAction(transform_comments_move_end);
	auto *transform_comments_trim = new QAction(getIcon("edit-clear-all"), tr("Trim"), transform_comments);
	connect(transform_comments_trim, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comments_trim"); } );
	transform_comments->addAction(transform_comments_trim);
	auto *transform_comments_delete = new QAction(tr("Delete all"), transform_comments);
	connect(transform_comments_delete, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comments_delete"); } );
	transform_comments->addAction(transform_comments_delete);
	transform_comments->addSeparator();
	auto *transform_comments_numbersign = new QAction(tr("Switch to #"), transform_comments);
	connect(transform_comments_numbersign, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comments_numbersign"); } );
	transform_comments->addAction(transform_comments_numbersign);
	auto *transform_comments_semicolon = new QAction(tr("Switch to ;"), transform_comments);
	connect(transform_comments_semicolon, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_comments_semicolon"); } );
	transform_comments->addAction(transform_comments_semicolon);
	/* Reset menu */
	auto *transform_reset = new QMenu(tr("Reset"), menu_transform);
	transform_reset->setIcon( getIcon("view-refresh") );
	menu_transform->addMenu(transform_reset);
	auto *transform_reset_original = new QAction(tr("To original INI on file system"),
	    transform_reset);
	connect(transform_reset_original, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_reset_original"); } );
	transform_reset->addAction(transform_reset_original);
	transform_reset_full_ = new QAction(tr("To full INI with GUI keys"), transform_reset);
	connect(transform_reset_full_, &QAction::triggered, this,
	    [=]{ onTransformMenuClick("transform_reset_full"); } );
	transform_reset->addAction(transform_reset_full_);

	/* CONVERT menu */
	QMenu *menu_convert = this->menuBar()->addMenu(tr("&Convert"));
	QMenu *menu_convert_tabs = new QMenu("&Tabs", menu_convert);
	menu_convert->addMenu(menu_convert_tabs);
	auto *convert_long_spaces_to_tabs = new QAction(tr("8 spaces to tabs"), menu_convert_tabs);
	connect (convert_long_spaces_to_tabs, &QAction::triggered, this,
	    [=]{ onConvertMenuClick("convert_long_spaces_to_tabs"); } );
	menu_convert_tabs->addAction(convert_long_spaces_to_tabs);
	auto *convert_short_spaces_to_tabs = new QAction(tr("4 spaces to tabs"), menu_convert_tabs);
	connect (convert_short_spaces_to_tabs, &QAction::triggered, this,
	    [=]{ onConvertMenuClick("convert_short_spaces_to_tabs"); } );
	menu_convert_tabs->addAction(convert_short_spaces_to_tabs);
	auto *convert_tabs_to_long_spaces = new QAction(tr("Tabs to 8 spaces"), menu_convert_tabs);
	connect (convert_tabs_to_long_spaces, &QAction::triggered, this,
	    [=]{ onConvertMenuClick("convert_tabs_to_long_spaces"); } );
	menu_convert_tabs->addAction(convert_tabs_to_long_spaces);
	auto *convert_tabs_to_short_spaces = new QAction(tr("Tabs to 4 spaces"), menu_convert_tabs);
	connect (convert_tabs_to_short_spaces, &QAction::triggered, this,
	    [=]{ onConvertMenuClick("convert_tabs_to_short_spaces"); } );
	menu_convert_tabs->addAction(convert_tabs_to_short_spaces);

	/* VIEW menu */
	QMenu *menu_view = this->menuBar()->addMenu(tr("&View"));
	auto *view_hidden_chars = new QAction(tr("Show &whitespaces"), menu_view);
	view_hidden_chars->setCheckable(true);
	if (getSetting("user::preview::show_ws", "value") == "TRUE")
		view_hidden_chars->setChecked(true);
	connect (view_hidden_chars, &QAction::triggered, this,
	    [=]{ onShowWhitespacesMenuClick(view_hidden_chars->isChecked()); } );
	menu_view->addAction(view_hidden_chars);
	menu_view->addSeparator();
	auto *view_new_tab = new QAction(getIcon("tab-new"), tr("&New tab"), menu_view);
	view_new_tab->setShortcut(QKeySequence::AddTab);
	connect(view_new_tab, &QAction::triggered, this, [=]{ addIniTab(); });
	menu_view->addAction(view_new_tab);
	auto *view_close_tab = new QAction(getIcon("tab-close"), tr("&Close tab"), menu_view);
	view_close_tab->setShortcut(QKeySequence::Close);
	connect(view_close_tab, &QAction::triggered, this, [=]{ closeTab(file_tabs_->currentIndex()); });
	menu_view->addAction(view_close_tab);

	/* HELP menu */
#if !defined Q_OS_MAC
	auto *menu_help_main = new QMenuBar(this->menuBar());
	QMenu *menu_help = menu_help_main->addMenu(tr("&?"));
	menu_help_main->addMenu(menu_help);
	auto *help = new QAction(getIcon("help-contents"), tr("&Help"), menu_help);
	this->menuBar()->setCornerWidget(menu_help_main); //push help menu to the right
	menu_help->addAction(help);

#else
	auto *menu_help_main = this->menuBar()->addMenu("&?");
	auto *help = new QAction(getIcon("help-contents"), tr("&Help"), menu_help_main);
	menu_help_main->addAction(help);
#endif
	help->setMenuRole(QAction::ApplicationSpecificRole);
	connect(help, &QAction::triggered, this,
	    [=]{ getMainWindow()->loadHelp("UI of INIshell", "help-preview"); });
}

/**
 * @brief Prepare a panel in the status bar that allows users to search for text in the preview.
 */
void PreviewWindow::createFindBar()
{
	find_text_ = new QLineEdit(this);
	connect(find_text_, &QLineEdit::textChanged, this, &PreviewWindow::onFindTextChanged);
	close_find_bar_ = new QToolButton(this);
	close_find_bar_->setIcon(getIcon("window-close"));
	close_find_bar_->setAutoRaise(true);
	connect(close_find_bar_, &QToolButton::clicked, this, [&]{ hideFindBar(); });
	this->statusBar()->addWidget(find_text_, 1);
	this->statusBar()->addWidget(close_find_bar_);
	hideFindBar();
}

/**
 * @brief Show the find bar.
 */
void PreviewWindow::showFindBar()
{
	previewStatus(QString( ));
	statusBar()->show();
	find_text_->show();
	close_find_bar_->show();
	find_text_->setFocus();
	find_text_->selectAll();
}

/**
 * @brief Hide the find bar.
 */
void PreviewWindow::hideFindBar()

{
	find_text_->hide();
	close_find_bar_->hide();
	statusBar()->hide();
}

/**
 * @brief Delegated event listener for when the preview text changes.
 * @details A lambda calls this function with the text box index to add an asterisk
 * to the title ("not saved").
 * @param[in] index Index of the tab the text changed in.
 */
void PreviewWindow::textChanged(const int &index)
{
	const QString file_name(file_tabs_->tabText(index));
	if (!file_name.endsWith("*"))
		file_tabs_->setTabText(index, file_name + " *");

}

/**
 * @brief Combine the original file's INIParser with GUI values and
 * set the local copy to access both.
 */
void PreviewWindow::loadIniWithGui()
{
	const QString current_app( getMainWindow()->getCurrentApplication() );
	preview_ini_ = getMainWindow()->getIniCopy();
	preview_ini_.clear(true); //only keep unknown keys (which are transported from input to output)

	getMainWindow()->getControlPanel()->setIniValuesFromGui(&preview_ini_);
	file_save_and_load_->setText(tr("Save and load into ") + getMainWindow()->getCurrentApplication());
	file_load_->setText(tr("Load into ") + current_app);
	edit_insert_missing_->setText(tr("Missing keys for ") + current_app);
	edit_insert_missing_mandatory_->setText(tr("Mandatory keys for ") + current_app);
	transform_reset_full_->setText(tr("To full INI with %1 keys").arg(current_app));
}

/**
 * @brief Write an opened INI file to the file system.
 * @param[in] file_name The path to save to.
 */
void PreviewWindow::writeIniToFile(const QString &file_name)
{
	QFile outfile(file_name);
	if (!outfile.open(QIODevice::WriteOnly)) {
		previewStatus(tr("Could not open %1").arg(QDir::toNativeSeparators(file_name)));
		return;
	}
	QTextStream ss(&outfile);
	const auto text_box( getCurrentEditor() );
	if (text_box)
		ss << text_box->toPlainText();
	outfile.close();

	const QFileInfo finfo(file_name); //switch the displayed name to new file (without asterisk)
	const QString shown_name(finfo.fileName());
	file_tabs_->setTabText(file_tabs_->currentIndex(), shown_name);
	previewStatus(tr("Saved to ") + file_name);

	setSetting("auto::history::last_preview_write", "path", finfo.absoluteDir().path());
}

/**
 * @brief Ask a user if they want to cancel an action.
 * @details This function is called before closing INI tabs with unsaved changes.
 * @return Button the user has clicked.
 */
int PreviewWindow::warnOnUnsavedIni()
{
	QMessageBox msgNotSaved;
	msgNotSaved.setWindowTitle("Warning ~ " + QCoreApplication::applicationName());
	msgNotSaved.setText(tr("<b>INI file not saved yet.</b>"));
	msgNotSaved.setInformativeText(tr("Your INI file(s) may contain unsaved changes."));
	msgNotSaved.setIcon(QMessageBox::Warning);
	msgNotSaved.setStandardButtons(QMessageBox::Cancel | QMessageBox::Discard);
	msgNotSaved.setDefaultButton(QMessageBox::Cancel);
	int clicked = msgNotSaved.exec();
	return clicked;
}

/**
 * @brief Show a temporary status message in the preview window. This text is volatile.
 * @param[in] text Text to show in the status.
 */
void PreviewWindow::previewStatus(const QString &text)
{ //temporary message - erased for e. g. on menu hover
	statusBar()->showMessage(text);
	statusBar()->show(); //might be hidden from closing the find bar
	statusBar()->setToolTip(text);
}

/**
 * @brief Helper function to get the current tab's text editor.
 * @return The text editor currently in view.
 */
PreviewEdit * PreviewWindow::getCurrentEditor()
{
#ifdef DEBUG
	if (!qobject_cast<PreviewEdit *>(file_tabs_->currentWidget()))
		qDebug() << "PreviewWindow::getCurrentEditor() should have casted to a PreviewEdit but did not";
#endif //def DEBUG
	return qobject_cast<PreviewEdit *>(file_tabs_->currentWidget());
}

/**
 * @brief Helper function to retrieve the currently displayed file's name.
 * @return The current INI file's path.
 */
QString PreviewWindow::getCurrentFilename() const
{ //TODO: Store the filename properly without asterisk shuffling
	QString shown_name( file_tabs_->tabText(file_tabs_->currentIndex()) );
	if (shown_name.endsWith("*")) //remove "unsaved" asterisk for the file name
		shown_name.chop(2);
	return file_tabs_->tabToolTip(file_tabs_->currentIndex()) + "/" + shown_name;
}

/**
 * @brief Set an editor's text programmatically and add it to the undo history.
 * @details This is a small hack and inserts the text by selecting all and pasting;
 * otherwise this change is not remembered in the document's history.
 * @param[in] editor Editor to set text for.
 * @param[in] text Text to set.
 */
void PreviewWindow::setTextWithHistory(QPlainTextEdit *editor, const QString &text)
{
	QTextDocument *doc( editor->document() );
	QTextCursor cursor( doc );
	cursor.select(QTextCursor::Document);
	cursor.insertText(text);
}

/**
 * @brief Insert missing keys.
 * @details This function looks through the keys in the GUI and adds the
 * ones that are missing.
 * @param[in] mode Insert mode (fill missing, fill missing mandatory).
 */
void PreviewWindow::insertText(const insert_text &mode)
{
	if (mode == HEADER) {
		static const QString marker("############################################################");
		const QString year( QDate::currentDate().toString("yyyy") );
		const QString date( QDate::currentDate().toString("yyyy-MM-dd") );
		const QString username( os::getLogName() );
		const QString user_domain( QHostInfo::localDomainName() );
		QString copyright("# Copyright ");
		if (!username.isEmpty()) copyright.append(username);
		if (!user_domain.isEmpty()) {
			if (!username.isEmpty()) copyright.append(" - ");
			copyright.append(user_domain);
		}
		if (!username.isEmpty() || !user_domain.isEmpty()) copyright.append(", ");
		copyright.append(year);
		for (int ii=copyright.size()+1; ii<marker.size(); ii++)
			copyright.append(" ");

		QString header;
		header += marker + "\n";
		header += copyright + "#\n";
		header += marker + "\n";
		header += "#" + QCoreApplication::applicationName() + " " + APP_VERSION_STR;
		header += " for " + getMainWindow()->getCurrentApplication() + "\n";
		header += "#" + date + "\n\n";
		preview_ini_.parseText(getCurrentEditor()->toPlainText().prepend(header));
		return;
	}

	INIParser gui_ini(getMainWindow()->getLogger());
	getMainWindow()->getControlPanel()->setIniValuesFromGui(&gui_ini);
	int counter = 0;
	for (auto &sec : *gui_ini.getSections()) {
		for (auto &keyval : sec.getKeyValueList()) {
			if (!preview_ini_.hasKeyValue(keyval.first)) {
				if ((mode == MISSING) ||
				    (mode == MISSING_MANDATORY && keyval.second.isMandatory())) {
					QString value( keyval.second.getValue() );
					preview_ini_.set(sec.getName(), keyval.first,
					    value.isEmpty()? "MISSING" : value);
					counter++;
				}
			}
		}
	}
	previewStatus(tr("Inserted %1 keys").arg(counter));
}

/**
 * @brief Perform whitespaces related transformations on the local INIParser copy.
 * @param[in] mode Type of transformation.
 */
void PreviewWindow::transformWhitespaces(const transform_whitespaces &mode)
{
	switch (mode) {
	case SINGLE_WS:
		for (auto &sec : *preview_ini_.getSections()) {
			for (auto &key : sec.getKeyValueList())
				sec.getKeyValue(key.first)->setKeyValWhitespaces(
				    std::vector<QString>( {"", " ", " ", " "} )); //(0)key(1)=(2)value(3)#comment
		}
		break;
	case LONGEST_WS:
		for (auto &sec : *preview_ini_.getSections()) {
			int max_key_length = 0;
			for (auto &key : sec.getKeyValueList()) {
				if (!key.second.getValue().isNull() && key.first.length() > max_key_length)
					max_key_length = key.first.length();
			}
			for (auto &key : sec.getKeyValueList()) {
				const int nr_ws = max_key_length - key.first.length() + 1;
				sec.getKeyValue(key.first)->setKeyValWhitespaces(
				    std::vector<QString>( {"", QString(" ").repeated(nr_ws), " ", " "} ) );
			}
		}
	} //switch
}

/**
 * @brief Perform capitalization related transformations on the local INIParser copy.
 * @param[in] mode Type of transformation.
 */
void PreviewWindow::transformCapitalization(const transform_capitalization &mode)
{
	const bool lower = (mode == LOWER_CASE || mode == SECTIONS_LOWER ||
	    mode == KEYS_LOWER || mode == VALUES_LOWER);
	const bool value = (mode == VALUES_UPPER || mode == VALUES_LOWER);
	const bool all = (mode == UPPER_CASE || mode == LOWER_CASE);
	const bool section = (mode == SECTIONS_UPPER || mode == SECTIONS_LOWER);

	for (auto &sec : *preview_ini_.getSections()) {
		if (section || all)
			sec.setName(lower? sec.getName().toLower() : sec.getName().toUpper());
		if (!section || all) {
			for (auto &key : sec.getKeyValueList()) {
				auto *keyvalue = sec.getKeyValue(key.first);
				if (value || all) {
					keyvalue->setValue(lower? keyvalue->getValue().toLower() :
					    keyvalue->getValue().toUpper());
				}
				if (!value || all) {
					keyvalue->setKey(lower? keyvalue->getKey().toLower() :
					    keyvalue->getKey().toUpper());
				}
			} //endfor key
		} //endif section
	} //endfor sec
}

/**
 * @brief Perform transformations concerning INI comments on the local INIParser copy.
 * @param[in] mode Type of transformation.
 * @return True if at least one comment prefix was removed.
 */
bool PreviewWindow::transformComments(const transform_comments &mode)
{
	bool removed_comment = false;
	if (mode == BLOCK_COMMENT || mode == BLOCK_UNCOMMENT) { //block (un)comment
		int first_line, last_line;
		getSelectionMargins(first_line, last_line); //span of selected lines
		QStringList lines( getCurrentEditor()->toPlainText().split("\n") );
		for (int ii = first_line - 1; ii < last_line; ++ii) {
			if (mode == BLOCK_COMMENT) { //add comment prefix
				lines[ii] = "#" + lines[ii];
			} else { //remove comment prefix
				if (lines.at(ii).trimmed().startsWith("#") ||
				    lines.at(ii).trimmed().startsWith(";")) {
					//find first "#" or ";" and delete:
					int prefix_pos = lines.at(ii).indexOf("#");
					if (prefix_pos == -1)
						prefix_pos = lines.at(ii).indexOf(";");
					lines[ii].remove(prefix_pos, 1);
					removed_comment = true;
				}
			}
		}
		setTextWithHistory(getCurrentEditor(), lines.join("\n"));
	} else if (mode == ALL_CONTENT) { //put whole content in comment
		QStringList all_lines( getCurrentEditor()->toPlainText().split("\n") );
		for (auto &line : all_lines)
			line = "#" + line;
		const QString loaded_filename( preview_ini_.getFilename() );
		preview_ini_.clear();
		preview_ini_.setFilename(loaded_filename);
		preview_ini_.setBlockCommentAtEnd(all_lines.join("\n"));
	} else if (mode == DUPLICATE) { //copy content to comment
		QStringList all_lines( getCurrentEditor()->toPlainText().split("\n") );
		for (auto &line : all_lines)
			line = "#" + line;
		preview_ini_.setBlockCommentAtEnd(preview_ini_.getBlockCommentAtEnd() + "\n" +
		    all_lines.join("\n"));
	} else if (mode == MOVE_TO_VALUES) { //remove spaces before comments
		for (auto &sec : *preview_ini_.getSections()) {
			std::vector<QString> ws_sec(sec.getKeyValWhiteSpaces()); //(0)[SECTION](1)#comment
			ws_sec.at(1) = " ";
			sec.setKeyValWhitespaces(ws_sec);
			for (auto &key : sec.getKeyValueList()) {
				std::vector<QString> ws_key(sec.getKeyValue(key.first)->getKeyValWhiteSpaces());
				ws_key.at(3) = " ";
				sec.getKeyValue(key.first)->setKeyValWhitespaces(ws_key);
			}
		}
	} else if (mode == MOVE_TO_END) { //collect all comments at end of file
		QString comment;
		for (auto &sec : *preview_ini_.getSections()) {
			comment += carryIfContent(sec.getBlockComment());
			comment += carryIfContent(sec.getInlineComment());
			sec.setBlockComment(QString());
			sec.setInlineComment(QString());
			for (auto &key : sec.getKeyValueList()) {
				auto *keyvalue = sec.getKeyValue(key.first);
				comment += carryIfContent(keyvalue->getBlockComment());
				comment += carryIfContent(keyvalue->getInlineComment());
				keyvalue->setBlockComment(QString());
				keyvalue->setInlineComment(QString());
			}
		} //endfor sec
		preview_ini_.setBlockCommentAtEnd(preview_ini_.getBlockCommentAtEnd()
		    + "\n" + comment);
	} else if (mode == TRIM) {
		preview_ini_.setBlockCommentAtEnd(trimComment(preview_ini_.getBlockCommentAtEnd()));
		for (auto &sec : *preview_ini_.getSections()) {
			sec.setBlockComment(trimComment(sec.getBlockComment()));
			sec.setInlineComment(trimComment(sec.getInlineComment()));
			for (auto &key : sec.getKeyValueList()) {
				auto *keyvalue = sec.getKeyValue(key.first);
				keyvalue->setBlockComment(trimComment(keyvalue->getBlockComment()));
				keyvalue->setInlineComment(trimComment(keyvalue->getInlineComment()));
			}
		}
	} else if (mode == DELETE) { //delete all comments
		preview_ini_.setBlockCommentAtEnd(QString());
		for (auto &sec : *preview_ini_.getSections()) {
			sec.setBlockComment(QString());
			sec.setInlineComment(QString());
			for (auto &key : sec.getKeyValueList()) {
				auto *keyvalue = sec.getKeyValue(key.first);
				keyvalue->setBlockComment(QString());
				keyvalue->setInlineComment(QString());
			}
		}
	} else if (mode == CONVERT_NUMBERSIGN || mode == CONVERT_SEMICOLON) {
		const bool hash = (mode == CONVERT_NUMBERSIGN);
		for (auto &sec : *preview_ini_.getSections()) {
			if (!sec.getBlockComment().isEmpty())
				sec.setBlockComment(convertPrefix(sec.getBlockComment(), hash));
			if (!sec.getInlineComment().isEmpty())
				sec.setInlineComment(convertPrefix(sec.getInlineComment(), hash));
			for (auto &key : sec.getKeyValueList()) {
				auto *keyvalue = sec.getKeyValue(key.first);
				if (!keyvalue->getBlockComment().isEmpty())
					keyvalue->setBlockComment(convertPrefix(
					    keyvalue->getBlockComment(), hash));
				if (!keyvalue->getInlineComment().isEmpty())
					keyvalue->setInlineComment(convertPrefix(
					    keyvalue->getInlineComment(), hash));
			}
			if (!preview_ini_.getBlockCommentAtEnd().isEmpty())
				preview_ini_.setBlockCommentAtEnd(convertPrefix(
				    preview_ini_.getBlockCommentAtEnd(), hash));
		} //endfor sec
	}
	return removed_comment; //for text selection handling
}

/**
 * @brief Conversion between spaces and tabs and vice versa.
 * @param[in] mode Mode of operation
 */
void PreviewWindow::convertTabs(const convert_tabs &mode)
{
	QPlainTextEdit *current_editor( getCurrentEditor() );
	QString current_text( current_editor->toPlainText() );
	QString new_text;
	if (mode == LONG_SPACES_TO_TABS || mode == SHORT_SPACES_TO_TABS) {
		new_text = current_text.replace(QString(" ").repeated(
		    mode == SHORT_SPACES_TO_TABS? short_spaces_for_tabs : long_spaces_for_tabs), "\t");
	} else if (mode == TABS_TO_LONG_SPACES || mode == TABS_TO_SHORT_SPACES) {
		new_text = current_text.replace("\t", QString(" ").repeated(
		    mode == TABS_TO_SHORT_SPACES? short_spaces_for_tabs : long_spaces_for_tabs));
	}
	preview_ini_.parseText(new_text);
}

/**
 * @brief Return the current cursor's line number in the file.
 * @return The current line number.
 */
int PreviewWindow::getCurrentLineNumber()
{
	int nr_of_lines = 1;
	QTextCursor cursor( getCurrentEditor()->textCursor() );
	cursor.movePosition(QTextCursor::StartOfLine);
	//count line numbers in current block:
	while(cursor.positionInBlock() > 0) {
		cursor.movePosition(QTextCursor::Up);
		nr_of_lines++;
	}
	//count line numbers in all blocks above that:
	QTextBlock block( cursor.block().previous() );
	while(block.isValid()) {
		nr_of_lines += block.lineCount();
		block = block.previous();
	}
	return nr_of_lines;
} //this is obviously a bit of a HACK: but there doesn't seem to be a good way

/**
 * @brief Find the number of currently selected lines.
 * @return Number of selected lines.
 */
int PreviewWindow::getNrOfSelectedLines()
{
	const QTextCursor cursor( getCurrentEditor()->textCursor() );
	return cursor.selectedText().split(paragraph_separator).count();
}

/**
 * @brief Return line number of text selection start and selection end.
 * @details The current cursor line will change depending on if the user
 * selects text from top to bottom or from bottom to top.
 * @param[out] first_line Line number of selection start.
 * @param[out] last_line Line number of selection end.
 */
void PreviewWindow::getSelectionMargins(int &first_line, int &last_line)
{
	const QTextCursor cursor( getCurrentEditor()->textCursor() );
	const int current_line = getCurrentLineNumber();

	if (cursor.position() > cursor.selectionStart()) { //selected from top to bottom
		first_line = current_line - getNrOfSelectedLines() + 1;
		last_line = current_line;
	} else { //bottom to top or nothing
		first_line = current_line;
		last_line = current_line + getNrOfSelectedLines() - 1;
	}
}

/**
 * @brief Vim-style paste to new line.
 */
void PreviewWindow::pasteToNewline()
{
	QPlainTextEdit *current_editor( getCurrentEditor() );
	current_editor->moveCursor(QTextCursor::EndOfLine);
	current_editor->insertPlainText("\n");
	getCurrentEditor()->paste();
}

/**
 * @brief Remove comments' leading whitespaces.
 * @details For single lines, this does not trim the distance from the
 * value to the comment (separate menu), but rather trims the comment
 * itself. Block comments however are put to the beginning of the line.
 * @param[in] comment Comment to trim.
 * @return Trimmed comment.
 */
QString PreviewWindow::trimComment(const QString &comment) const
{
	if (comment.count("\n") == 0) {
		return comment.mid(0, 1) + comment.mid(1).trimmed();
	} else {
		QStringList lines(comment.split("\n"));
		for (auto &line : lines) {
			int pre = line.indexOf("#");
			if (pre == -1)
				pre = line.indexOf(";");
			line = line.mid(pre, 1) + line.mid(pre + 1).trimmed();
		}
		return lines.join("\n");
	}
}

/**
 * @brief Menu item to open an existing INI file in the Preview Editor.
 */
void PreviewWindow::openFile()
{
	QString start_path( getSetting("auto::history::last_preview_ini", "path") );
	if (start_path.isEmpty())
		start_path = QDir::currentPath();

	const QString path = QFileDialog::getOpenFileName(this, tr("Open INI file in preview"), start_path,
	    "INI files (*.ini);;All files (*)", nullptr,
	    QFileDialog::DontUseNativeDialog | QFileDialog::DontConfirmOverwrite);
	if (path.isNull()) //cancelled
		return;
	addIniTab(path);

	setSetting("auto::history::last_preview_ini", "path", QFileInfo( path ).absoluteDir().path());
}

/**
 * @brief Menu item to save an INI file to the file system.
 */
void PreviewWindow::saveFile()
{
	writeIniToFile(getCurrentFilename());
}

/**
 * @brief Menu item to save an INI file to the file system.
 */
void PreviewWindow::saveFileAs()
{
	QString start_path(getSetting("auto::history::last_preview_write", "path"));
	if (start_path.isEmpty())
		start_path = QDir::currentPath();
	QString proposed_name(file_tabs_->tabText(file_tabs_->currentIndex()));
	if (proposed_name.endsWith("*"))
		proposed_name.chop(2);
	const QString file_name = QFileDialog::getSaveFileName(this, tr("Save INI file"),
	    start_path + "/" + proposed_name, "INI files (*.ini *.INI);;All files (*)",
	    nullptr, QFileDialog::DontUseNativeDialog);
	if (file_name.isNull()) //cancelled
		return;
	writeIniToFile(file_name);
}

/**
 * @brief Menu item to save text to file and load into GUI.
 */
void PreviewWindow::saveFileAndLoadIntoGui()
{
	saveFile();
	getMainWindow()->openIni(getCurrentFilename());
}

/**
 * @brief Menu item to load into GUI.
 */
void PreviewWindow::loadIntoGui()
{
	INIParser preview_ini(getMainWindow()->getLogger());
	preview_ini.parseText(getCurrentEditor()->toPlainText());
	getMainWindow()->setGuiFromIni(preview_ini);
}

/**
 * @brief One-click action to save current contents to a new, unique file.
 */
void PreviewWindow::quickBackup()
{
	int counter = 1;
	const QString template_name( QFileInfo( getCurrentFilename() ).absoluteFilePath()
	    + ".bak%1" );
	while (QFile::exists(template_name.arg(counter)))
		counter++;
	QFile outfile(template_name.arg(counter));
	if (!outfile.open(QIODevice::WriteOnly)) {
		previewStatus(tr("Could not open INI file for writing"));
		return;
	}
	QTextStream ss(&outfile);
	ss << getCurrentEditor()->toPlainText();
	outfile.close();
	previewStatus(tr("Saved to %1").arg(template_name.arg(counter)));
}

/**
 * @brief Event listener to text being changed in the find bar - go look for it.
 * @param[in] text Text the user wants to find.
 */
void PreviewWindow::onFindTextChanged(const QString &text)
{
	auto current_text_box_( getCurrentEditor() );
	QTextCursor cursor( current_text_box_->textCursor() );
	cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
	current_text_box_->setTextCursor(cursor);
	const bool found = current_text_box_->find(text);
	if (found)
		find_text_->setStyleSheet(QString( ));
		//TODO: find next on Enter
	else
		find_text_->setStyleSheet("QLineEdit {color: " + colors::getQColor("warning").name() + "}");
}

/**
 * @brief Transformation related operations through other menus.
 * @param[in] action String denoting the mode of operation.
 */
void PreviewWindow::onInsertMenuClick(const QString &action)
{
	//load user's text changes back into parser:
	preview_ini_.parseText(getCurrentEditor()->toPlainText());

	QString ini_contents;
	QTextStream ss(&ini_contents);

	if (action == "insert_missing")
		insertText(MISSING);
	else if (action == "insert_missing_mandatory")
		insertText(MISSING_MANDATORY);
	else if (action == "insert_header")
		insertText(HEADER);
#ifdef DEBUG
	else
		qDebug() << "Signal mapping failed in PreviewWindow::onInsertMenuClick():" << action;
#endif //def DEBUG
	preview_ini_.outputIni(ss, has_sorted_alphabetically_);
	setTextWithHistory(getCurrentEditor(), ini_contents);
}

/**
 * @brief Perform INI file transformations from the menu.
 * @details This function is called by a lambda with a menu action string.
 * @param[in] action String denoting the transformation to perform.
 */
void PreviewWindow::onTransformMenuClick(const QString &action)
{
	QPlainTextEdit *current_editor( getCurrentEditor() );
	QString ini_contents;
	QTextStream ss(&ini_contents);

	/* transformations that do not actually stage changes in the INIParser */
	if (action == "transform_sort_alphabetically" || action == "transform_sort_order") {
		//sort sections and within them the keys alphabetically, or restore order:
		has_sorted_alphabetically_ = (action == "transform_sort_alphabetically");
		//(we need to remember this because nothing is actually set in the INIParser)
		preview_ini_.outputIni(ss, has_sorted_alphabetically_);
		setTextWithHistory(current_editor, ini_contents);
		//TODO: this loses changes the user has made in the editor... would need yet
		//another INIParser from which the local one is filled, keeping its keys in order.
		previewStatus(tr("Note: sort first, then start editing."));
		return; //don't parse the text back, or the order would be read back as alphabetical
		//TODO: if a section is ordered before the default section, and the default section is not
		//output, then the keys switch section.
	}

	//load user's text changes back into parser:
	preview_ini_.parseText(current_editor->toPlainText());

	/* special cases */
	if (action == "transform_comment_block" || action == "transform_uncomment_block") {
		const bool cmnt = (action == "transform_comment_block");
		const int sel_start = current_editor->textCursor().selectionStart(); //to restore selection
		const int sel_end = current_editor->textCursor().selectionEnd();
		const int nr_of_selected_lines = getNrOfSelectedLines();
		const bool comment_removed = transformComments(cmnt? BLOCK_COMMENT : BLOCK_UNCOMMENT);
		QTextCursor restore_cursor( current_editor->textCursor() );
		if (sel_start != sel_end) { //restore selection
			restore_cursor.setPosition(sel_start);
			restore_cursor.movePosition(QTextCursor::StartOfLine);
			for (int ii = 0; ii < nr_of_selected_lines - 1; ++ii)
				restore_cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
			restore_cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
		} else { //no selection - keep cursor where it was
			if (cmnt)
				restore_cursor.setPosition(sel_start + 1);
			else
				restore_cursor.setPosition(sel_start - (comment_removed? 1 : 0));
		}
		current_editor->setTextCursor(restore_cursor);
		return;
	}

	const int old_cursor_pos = current_editor->textCursor().position();
	if (action == "transform_whitespace_singlews") { //Transform all whitespaces to single spaces
		transformWhitespaces(SINGLE_WS);
	} else if (action == "transform_whitespace_longestws") { //look for longest key in section and use that many spaces
		transformWhitespaces(LONGEST_WS);
	} else if (action == "transform_capitalization_sections_upper") {
		transformCapitalization(SECTIONS_UPPER);
	} else if (action == "transform_capitalization_sections_lower") {
		transformCapitalization(SECTIONS_LOWER);
	} else if (action == "transform_capitalization_keys_upper") {
		transformCapitalization(KEYS_UPPER);
	} else if (action == "transform_capitalization_keys_lower") {
		transformCapitalization(KEYS_LOWER);
	} else if (action == "transform_capitalization_values_upper") {
		transformCapitalization(VALUES_UPPER);
	} else if (action == "transform_capitalization_values_lower") {
		transformCapitalization(VALUES_LOWER);
	} else if (action == "transform_capitalization_upper" || action == "transform_capitalization_lower") { //all sections, keys and values to upper/lower case
		//we do it through the INIParser so that comments stay the same
		const bool lower = (action == "transform_capitalization_lower");
		transformCapitalization(lower? LOWER_CASE : UPPER_CASE);
	} else if (action == "transform_comments_content") {
		transformComments(ALL_CONTENT); //comment out whole file
	} else if (action == "transform_comments_duplicate") {
		transformComments(DUPLICATE);
	} else if (action == "transform_comments_move_value") {
		transformComments(MOVE_TO_VALUES);
	} else if (action == "transform_comments_move_end") {
		transformComments(MOVE_TO_END);
	} else if (action == "transform_comments_numbersign") {
		transformComments(CONVERT_NUMBERSIGN);
	} else if (action == "transform_comments_semicolon") {
		transformComments(CONVERT_SEMICOLON);
	} else if (action == "transform_comments_trim") {
		transformComments(TRIM);
	} else if (action == "transform_comments_delete") { //delete all comments
		transformComments(DELETE);
	} else if (action == "transform_reset_original") { //reset to original INI
		preview_ini_ = getMainWindow()->getIniCopy();
		previewStatus(tr("Reset to file contents without GUI values."));
	} else if (action == "transform_reset_full") { //reset to original INI plus GUI values
		loadIniWithGui();
		previewStatus(tr("Reset to state of latest preview."));
#ifdef DEBUG
	} else {
		qDebug() << "Signal mapping failed in PreviewWindow::onTransformMenuClick():" << action;
#endif //def DEBUG
	}

	preview_ini_.outputIni(ss, has_sorted_alphabetically_);
	setTextWithHistory(current_editor, ini_contents);
	//if not handled by the specific transformation, set the cursor back to the last position:
	if (current_editor->toPlainText().length() >= old_cursor_pos) {
		QTextCursor restore_cursor( current_editor->textCursor() );
		restore_cursor.setPosition(old_cursor_pos);
		current_editor->setTextCursor(restore_cursor);
	}
}

/**
 * @brief Conversions outside of the INIParser.
 * @param[in] action Mode of operation.
 */
void PreviewWindow::onConvertMenuClick(const QString &action)
{
	//load user's text changes back into parser:
	preview_ini_.parseText(getCurrentEditor()->toPlainText());

	QString ini_contents;
	QTextStream ss(&ini_contents);

	if (action == "convert_long_spaces_to_tabs")
		convertTabs(LONG_SPACES_TO_TABS);
	else if (action == "convert_short_spaces_to_tabs")
		convertTabs(SHORT_SPACES_TO_TABS);
	else if (action == "convert_tabs_to_long_spaces")
		convertTabs(TABS_TO_LONG_SPACES);
	else if (action == "convert_tabs_to_short_spaces")
		convertTabs(TABS_TO_SHORT_SPACES);
#ifdef DEBUG
	else
		qDebug() << "Signal mapping failed in PreviewWindow::onConvertMenuClick():" << action;
#endif //def DEBUG

	preview_ini_.outputIni(ss, has_sorted_alphabetically_);
	setTextWithHistory(getCurrentEditor(), ini_contents);
}

/**
 * @brief Enable/disable hiding whitespace characters.
 * @param[in] show_ws True to show them.
 */
void PreviewWindow::onShowWhitespacesMenuClick(const bool &show_ws)
{
	setSetting("user::preview::show_ws", "value", show_ws? "TRUE" : "FALSE");
	QTextOption option;
	if (show_ws)
		option.setFlags(QTextOption::ShowLineAndParagraphSeparators | QTextOption::ShowTabsAndSpaces |
		    QTextOption::ShowDocumentTerminator); //TODO: how to syntax highlight the paragraph sign?
	else
		option.setFlags(option.flags() & (~QTextOption::ShowLineAndParagraphSeparators) &
		    (~QTextOption::ShowTabsAndSpaces) & (~QTextOption::ShowDocumentTerminator));
	QList<QPlainTextEdit *> all_editors( this->findChildren<QPlainTextEdit *>() );
	for (auto &editor : all_editors)
		editor->document()->setDefaultTextOption(option);
}
