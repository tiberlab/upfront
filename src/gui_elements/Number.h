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
 * A Number panel can input integers, doubles, and arithmetic expressions which it will check.
 * 2019-10
 */

#ifndef NUMBER_H
#define NUMBER_H

#include "Atomic.h"

#include <QAbstractSpinBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QString>
#include <QToolButton>
#include <QWidget>
#include <QtXml>

class KeyPressFilter : public QObject { //detect key press events in the number_element_
	public:
		bool eventFilter(QObject *object, QEvent *event) override;
};

class Number : public Atomic {
	Q_OBJECT

	public:
		explicit Number(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);
		~Number() override;
		Number(const Number&) = delete; //these have to be properly implemented if ever needed
		Number& operator =(Number const&) = delete;
		Number(Number&&) = delete;
		Number& operator=(Number&&) = delete;
		void setDefaultPanelStyles(const QString &in_value) override;
		void clear(const bool &set_default = true) override;

	private:
		enum number_mode {
			NR_DECIMAL,
			NR_INTEGER,
			NR_INTEGERPLUS,
		};
		void setOptions(const QDomNode &options);
		int getPrecisionOfNumber(const QString &str_number) const;
		void setEmpty(const bool &is_empty);

		std::vector<std::pair<QString, QString>> substitutions_; //user-set substitutions to translate to tinyexpr
		KeyPressFilter *key_filter_ = nullptr;
		QAbstractSpinBox *number_element_ = nullptr;
		QLineEdit *expression_element_ = nullptr;
		QHBoxLayout *switcher_layout_ = nullptr;
		QToolButton *switch_button_ = nullptr;
		int default_precision_ = 2;
		int precision_ = default_precision_; //current precision
		number_mode mode_;
		bool show_sign = false;

	private slots:
		void checkValue(const double &to_check);
		void checkValue(const int &to_check);
		void checkStrValue(const QString &str_check);
		bool isNumber(const QString &expression) const;
		void onPropertySet() override;
		void switchToggle(bool checked);
};

#endif //NUMBER_H
