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
 * The base class for input panels handling some common functionality.
 * 2019-10
 */

#ifndef ATOMIC_H
#define ATOMIC_H

#include "src/gui_elements/Helptext.h"
#include "src/main/common.h"
#include "src/main/constants.h"
#include "src/main/INIParser.h"

#include <QFont>
#include <QHBoxLayout>
#include <QMenu>
#include <QSpacerItem>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtXml>

class Atomic : public QWidget {
	Q_OBJECT

	public:
		enum PanelStyle {
			MANDATORY,
			DEFAULT,
			VALID,
			FAULTY
		};
		Atomic(QString section, QString key, QWidget *parent = nullptr);
		virtual void setDefaultPanelStyles(const QString &in_value);
		static QString getQtKey(const QString &ini_key);
		QString getIniValue(QString &section, QString &key) const noexcept;
		virtual void clear(const bool &set_default = true);

	protected:
		QWidget * getPrimaryWidget() { return primary_widget_; }
		void setPrimaryWidget(QWidget *primary_widget, const bool &set_object_name = true,
		    const bool &no_styles = false);
		QString getId() const noexcept { return section_ + Cst::sep + key_; }
		void setPanelStyle(const PanelStyle &style, const bool &set = true, QWidget *widget = nullptr);
		void setValidPanelStyle(const bool &on);
		void substituteKeys(QDomElement &parent_element, const QString &replace,
		    const QString &replace_with);
		QSpacerItem * buildSpacer();
		void setLayoutMargins(QLayout *layout);
		Helptext * addHelp(QHBoxLayout *layout, const QDomNode &options, const bool &tight = false,
		    const bool &force = false);
		void setBufferedUpdatesEnabled(const int &time = 0);
		int getElementTextWidth (const QStringList &text_list, const int &element_min_width,
		    const int &element_max_width);
		void setFontOptions(QWidget *widget, const QDomNode &options);
		QFont setFontOptions(const QFont &item_font, const QDomElement &in_options);

		QString section_; //INI section
		QString key_; //INI key
		QString ini_value_ = QString(); //the current value of the panel
		QWidget *primary_widget_ = nullptr; //widget to use for mandatory field highlighting etc.

	protected slots:
		virtual void onPropertySet();
		void setIniValue(const int &value); //called when value is set on user input or from the code
		void setIniValue(const double &value);
		void setIniValue(const QString &value);

	private:
		void createContextMenu();

		QMenu panel_context_menu_;
		INIParser *ini_ = nullptr; //pointer to the main INIParser

	private slots:
		void onTimerBufferedUpdatesEnabled();
		void onConextMenuRequest(const QPoint &);
};

#endif //ATOMIC_H
