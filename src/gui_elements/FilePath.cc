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

#include "FilePath.h"
#include "Label.h"
#include "src/main/colors.h"
#include "src/main/settings.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>

/**
 * @class FilePath
 * @brief Default constructor for a file/path picker.
 * @details The file/path picker displays a dialog to select either a file or a folder.
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this file/path picker.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
FilePath::FilePath(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	/* label and text field */
	auto *key_label( new Label(QString(), QString(), options, no_spacers, key_, this) );
	path_text_ = new QLineEdit; //textfield with one line
	connect(path_text_, &QLineEdit::textEdited, this, &FilePath::checkValue);
	setPrimaryWidget(path_text_);

	/* "open" button and info label */
	open_button_ = new QPushButton("…");
	open_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(open_button_, &QPushButton::clicked, this, &FilePath::openFile);
	info_text_ = new QLabel();
	info_text_->setAlignment(Qt::AlignCenter);
	QPalette label_palette( info_text_->palette() ); //custom color
	label_palette.setColor(QPalette::WindowText, colors::getQColor("warning"));
	info_text_->setPalette(label_palette);
	info_text_->setVisible(false);

	/* layout of the basic elements */
	auto *filepath_layout( new QHBoxLayout );
	filepath_layout->addWidget(key_label, 0, Qt::AlignLeft);
	filepath_layout->addWidget(path_text_);
	filepath_layout->addWidget(open_button_);
	path_text_->setMinimumWidth(no_spacers? Cst::tiny : Cst::width_filepath_min);
	addHelp(filepath_layout, options, no_spacers);

	/* main layout with interactive widgets and info label */
	auto *main_layout( new QVBoxLayout );
	main_layout->addLayout(filepath_layout);
	main_layout->addWidget(info_text_, 0, Qt::AlignLeft);
	setLayoutMargins(main_layout);
	this->setLayout(main_layout);

	setOptions(options); //file_and_path, filename or path
	path_text_->setPlaceholderText(path_only_? tr("<no path set>") : tr("<no file set>"));
}

/**
 * @brief Parse options for a file/patch picker from XML.
 * @param[in] options XML node holding the file/path picker.
 */
void FilePath::setOptions(const QDomNode &options)
{
	const QString type(options.toElement().attribute("type"));
	if ( type == "path") {
		path_only_ = true;
		open_button_->setToolTip(tr("Open path"));
	} else {
		if (type == "filename") filename_only_ = true;
		open_button_->setToolTip(tr("Open file"));
	}

	for (QDomElement op = options.firstChildElement("option"); !op.isNull(); op = op.nextSiblingElement("option")) {
		const QString ext( op.attribute("extension") ); //selectable file extensions can be set
		extensions_ += ext + (ext.isEmpty()? "" : ";;");
	}
	if (extensions_.isEmpty())
		extensions_= tr("All Files (*)");
	else
		extensions_.chop(2); //remove trailing ;;

	if (options.toElement().attribute("mode") == "input") //so we can do a little more checking
		io_mode = INPUT;
	else if (options.toElement().attribute("mode").toLower() == "output")
		io_mode = OUTPUT;

	setDefaultPanelStyles(path_text_->text());
}

/**
 * @brief Event listener for changed INI values.
 * @details The "ini_value" property is set when parsing default values and potentially again
 * when setting INI keys while parsing a file.
 */
void FilePath::onPropertySet()
{
	const QString filename( this->property("ini_value").toString() );
	if (ini_value_ == filename)
		return;
	path_text_->setText(filename);
	checkValue(filename);
}

/**
 * @brief Perform checks on the selected file name.
 * @details While we always set the file name in the INI (could run on different machines)
 * some integrity checks regarding existence, permissions, ..., are performed here.
 * @param[in] filename The chosen file name or path.
 */
void FilePath::checkValue(const QString &filename)
{
	path_text_->setText(filename);
	info_text_->setVisible(true);
	const QFileInfo file_info( filename ); //TODO: Handle paths relative to the loaded INI

	if (filename.isEmpty()) {
		setUpdatesEnabled(false);
		info_text_->setVisible(false);
	} else if (filename.trimmed().isEmpty()) {
		info_text_->setText(tr("[Empty file name]"));
	} else if (filename_only_) { //no other checks possible, we don't know which path the file belongs to
		setUpdatesEnabled(false);
		info_text_->setVisible(false);
	} else if (io_mode == INPUT && !file_info.exists()) {
		info_text_->setText(path_only_? tr("[Folder does not exist]") : tr("[File does not exist]"));
	} else if (path_only_ && file_info.isFile()) {
		info_text_->setText(tr("[Directory path points to a file]"));
	} else if (!path_only_ && file_info.isDir()) {
		info_text_->setText(tr("[File path points to a directory]"));
	} else if (io_mode == INPUT && file_info.exists() && !file_info.isReadable()) {
		info_text_->setText(tr(
		    R"([File not readable for current user (owned by "%1")])").arg(file_info.owner()));
	} else if (io_mode == OUTPUT && file_info.exists() && !file_info.isWritable()) {
		info_text_->setText(tr(
		    R"([File not writable for current user (owned by "%1")])").arg(file_info.owner()));
	} else if (io_mode == UNSPECIFIED && file_info.exists() && !file_info.isReadable() && !file_info.isWritable()) {
		info_text_->setText(tr(
		    R"([File not accessible for current user (owned by "%1")])").arg(file_info.owner()));
	} else if (file_info.isExecutable() && !file_info.isDir()) {
		info_text_->setText(tr("[File is an executable]"));
	} else if (file_info.isSymLink()) {
		info_text_->setText(tr("[File is a symbolic link]"));
	} else if (io_mode == OUTPUT && !path_only_ && file_info.exists()) {
		info_text_->setText(tr("[File already exists]"));
	} else if (io_mode == OUTPUT && filename.trimmed() != filename) {
		info_text_->setText(tr("[File name has leading or trailing whitespaces]"));
	} else {
		setUpdatesEnabled(false);
		info_text_->setVisible(false);
	}

	setDefaultPanelStyles(filename);
	setIniValue(filename); //the label is just info -> file does not actually have to exist
	setBufferedUpdatesEnabled(1); //hiding the info text sometimes flickers
}

/**
 * @brief Event listener for the open button.
 * @details Open a file or path by displaying a dialog window.
 */
void FilePath::openFile()
{
	path_text_->setProperty("shows_default", "true");
	QString start_path( getSetting("auto::history::last_panel_path", "path") );
	if (start_path.isEmpty())
		start_path = QDir::currentPath();

	QString path;
	if (path_only_) {
		path = QFileDialog::getExistingDirectory(this, tr("Open Folder"), start_path,
		    QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly);
	} else {
		if (io_mode == INPUT) {
			path = QFileDialog::getOpenFileName(this, tr("Open File"), start_path,
			    extensions_, nullptr, QFileDialog::DontUseNativeDialog | QFileDialog::DontConfirmOverwrite);
		} else if (io_mode == OUTPUT || io_mode == UNSPECIFIED) {
			path = QFileDialog::getSaveFileName(this, tr("Open File"), start_path,
			    extensions_, nullptr, QFileDialog::DontUseNativeDialog | QFileDialog::DontConfirmOverwrite);
		}
	}
	
	if (!path.isNull()) {
		setSetting("auto::history::last_panel_path", "path", QFileInfo( path ).absoluteDir().path());
		//setProperty calls checkValue()
		if (filename_only_)
			this->setProperty("ini_value", QFileInfo( path ).fileName());
		else
			this->setProperty("ini_value", path);
	}
}
