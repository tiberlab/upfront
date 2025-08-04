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
 * The FilePath panel opens a dialog to select a file or path and performs some permissions checks.
 * 2019-10
 */

#ifndef FILEPATH_H
#define FILEPATH_H

#include "Atomic.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QtXml>

class FilePath : public Atomic {
	Q_OBJECT

	public:
		explicit FilePath(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);

	private:
		enum input_output_mode { //used for info label only
			UNSPECIFIED,
			INPUT,
			OUTPUT
		};
		void setOptions(const QDomNode &options);

		QString extensions_; //file extension filter
		input_output_mode io_mode = UNSPECIFIED;
		QLineEdit *path_text_ = nullptr;
		QLabel *info_text_ = nullptr;
		QPushButton *open_button_ = nullptr;
		bool path_only_ = false, filename_only_ = false;

	private slots:
		void openFile();
		void onPropertySet() override;
		void checkValue(const QString &filename);
};

#endif //FILEPATH_H
