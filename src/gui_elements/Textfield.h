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
 * A Textfield panel enters simple raw text and can perform some syntax checks on it.
 * 2019-10
 */

#ifndef TEXTFIELD_H
#define TEXTFIELD_H

#include "Atomic.h"

#include <QLineEdit>
#include <QString>
#include <QWidget>
#include <QtXml>

#include <utility>
#include <vector>

class Textfield : public Atomic {
	Q_OBJECT

	public:
		explicit Textfield(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);

	private:
		void setOptions(const QDomNode &options);

		std::vector<std::pair<QString, QString>> substitutions_; //user-set substitutions to translate to tinyexpr
		QString validation_regex_;
		QLineEdit *textfield_ = nullptr;
		QToolButton *check_button_ = nullptr;
		bool needs_prefix_for_evaluation_ = true;

	private slots:
		void onPropertySet() override;
		void checkValue(const QString &text);
		void checkButtonClicked();
};

#endif // TEXTFIELD_H
