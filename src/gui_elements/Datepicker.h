/*****************************************************************************/
/*  Copyright 2020 ALPsolut.eu                                               */
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
 * 2020-08
 */

#ifndef DATEPICKER_H
#define DATEPICKER_H

#include "Atomic.h"

#include <QDateTime>
#include <QDateTimeEdit>
#include <QString>
#include <QWidget>
#include <QtXml>

class DateKeyPressFilter : public QObject { //detect key press events in the datepicker_
	public:
		bool eventFilter(QObject *object, QEvent *event) override;
};

class Datepicker : public Atomic {
	Q_OBJECT

	public:
		explicit Datepicker(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);
		~Datepicker() override;

	private:
		void setOptions(const QDomNode &options);
		void setEmpty(const bool &is_empty);

		QString date_format_ = "yyyy-MM-ddThh:mm:ss"; //ISO date format is default
		QDateTimeEdit *datepicker_ = nullptr;
		DateKeyPressFilter *key_filter_ = nullptr;

	private slots:
		void onPropertySet() override;
		void checkValue(const QDateTime &datetime);
};

#endif // DATEPICKER_H 
