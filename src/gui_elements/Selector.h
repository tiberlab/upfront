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
 * A Selector is a special panel that takes a widget in its options and reproduces a parametrized clone
 * of it. It is one of the few panels that must handle INI keys that aren't present in the XML
 * (although the main work for this is done in MainWindow.cc).
 * 2019-10
 */

#ifndef SELECTOR_H
#define SELECTOR_H

#include "Atomic.h"
#include "Group.h"
#include "src/main/common.h"

#include <QComboBox>
#include <QPushButton>
#include <QString>
#include <QLineEdit>
#include <QWidget>
#include <QtXml>

#include <map>

class SelectorKeyPressFilter : public QObject { //detect key press events in the text input field
	public:
		bool eventFilter(QObject *object, QEvent *event) override;
};

class Selector : public Atomic {
	Q_OBJECT

	public:
		explicit Selector(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);
		~Selector() override;
		Selector(const Selector&) = delete; //these have to be properly implemented if ever needed
		Selector& operator =(Selector const&) = delete;
		Selector(Selector&&) = delete;
		Selector& operator=(Selector&&) = delete;
		int count() const { return static_cast<int>(container_map_.size()); }
		void clear(const bool &set_default = true) override;

	private:
		void setOptions(const QDomNode &options);
		void addPanel(const QString &param_text);
		void removePanel(const QString &param_text);
		inline QString getCurrentText() const { return textfield_? textfield_->text() : dropdown_->currentText(); }

		QDomNode templ_;
		std::map<QString, Group *, CaseInsensitiveCompare> container_map_;
		QComboBox *dropdown_ = nullptr;
		QLineEdit *textfield_ = nullptr;
		Group *container_ = nullptr;
		QPushButton *plus_button_ = nullptr;
		SelectorKeyPressFilter *key_events_input_ = nullptr;

	private slots:
		void guiAddPanel();
		void removeCurrentPanel();
		void onPropertySet() override;

	friend class SelectorKeyPressFilter; //allow key press filter to click private buttons
};

#endif //SELECTOR_H
