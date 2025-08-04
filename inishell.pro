###############################################################################
#   Copyright 2019 WSL Institute for Snow and Avalanche Research  SLF-DAVOS   #
###############################################################################
# This file is part of INIshell.
# INIshell is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# INIshell is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with INIshell.  If not, see <http://www.gnu.org/licenses/>.

#The only qmake file for this project. Use 'qmake inishell.pro; make'.
#2019-10

#CONFIG += debug
CONFIG -= debug
#CONFIG -= app_bundle
CONFIG += release

message("Building from $$_PRO_FILE_ ...")

QT += core gui widgets xml xmlpatterns

CONFIG += c++11
#CONFIG += static

CONFIG(debug) { #in release, we try everything
    message("Debug build, enabling check.")
    lessThan(QT_MAJOR_VERSION, 5): error("Qt5 is required for this project.")
    CONFIG += strict_c++ #disable compiler extensions
    CONFIG += warn_on
    QMAKE_CXXFLAGS += -Wall
    DEFINES += QT_DEPRECATED_WARNINGS
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000 #error for all APIs deprecated before Qt 6.0.0
    DEFINES += DEBUG #to be able to check at runtime
} else {
    message("Release build.")
    QMAKE_CXXFLAGS += -Wall -lto
}

VERSION = "2.0.4"
VERSION_PE_HEADER = "2.0"
DEFINES += APP_VERSION_STR=\\\"$$VERSION\\\"
QMAKE_TARGET_COMPANY = "WSL Institute for Snow and Avalanche Research"
QMAKE_TARGET_DESCRIPTION = "Graphical User Interface for various models. The GUI is dynamically generated from a semantic description contained in XML files."
QMAKE_TARGET_COPYRIGHT = "GPLv3"

RESOURCES = resources/inishell.qrc
RC_ICONS = resources/icons/inishell_192.ico #for Windows
ICON = resources/icons/inishell_192.icns #for Mac OS when not set in the program yet (launching)

##package the applications' XML files into Mac's dmg file
applicationsXML.files = $$PWD/inishell-apps
applicationsXML.path = Contents/
QMAKE_BUNDLE_DATA += applicationsXML

##package meteoio_timeseries into Mac's dmg file
#meteoio.files = $$PWD/../meteoio/bin/meteoio_timeseries $$PWD/../meteoio/lib/libmeteoio.dylib
#meteoio.path = Contents/MacOS

SOURCES += \
    src/gui/AboutWindow.cc \
    src/gui/ApplicationsView.cc \
    src/gui/IniFolderView.cc \
    src/gui/Logger.cc \
    src/gui/MainPanel.cc \
    src/gui/MainWindow.cc \
    src/gui/PathView.cc \
    src/gui/PreviewEdit.cc \
    src/gui/PreviewWindow.cc \
    src/gui/TerminalView.cc \
    src/gui/WorkflowPanel.cc \
    src/gui_elements/Atomic.cc \
    src/gui_elements/Checkbox.cc \
    src/gui_elements/Checklist.cc \
    src/gui_elements/Choice.cc \
    src/gui_elements/Datepicker.cc \
    src/gui_elements/Dropdown.cc \
    src/gui_elements/FilePath.cc \
    src/gui_elements/GridPanel.cc \
    src/gui_elements/Group.cc \
    src/gui_elements/gui_elements.cc \
    src/gui_elements/Helptext.cc \
    src/gui_elements/HorizontalPanel.cc \
    src/gui_elements/Label.cc \
    src/gui_elements/Number.cc \
    src/gui_elements/Replicator.cc \
    src/gui_elements/Selector.cc \
    src/gui_elements/Spacer.cc \
    src/gui_elements/Textfield.cc \
    src/main/colors.cc \
    src/main/common.cc \
    src/main/dimensions.cc \
    src/main/Error.cc \
    src/main/INIParser.cc \
    src/main/expressions.cc \
    src/main/inishell.cc \
    src/main/main.cc \
    src/main/os.cc \
    src/main/settings.cc \
    src/main/XMLReader.cc \
    lib/tinyexpr.c

HEADERS += \
    src/gui/AboutWindow.h \
    src/gui/ApplicationsView.h \
    src/gui/IniFolderView.h \
    src/gui/Logger.h \
    src/gui/MainPanel.h \
    src/gui/MainWindow.h \
    src/gui/PathView.h \
    src/gui/PreviewEdit.h \
    src/gui/PreviewWindow.h \
    src/gui/TerminalView.h \
    src/gui/WorkflowPanel.h \
    src/gui_elements/Atomic.h \
    src/gui_elements/Checkbox.h \
    src/gui_elements/Checklist.h \
    src/gui_elements/Choice.h \
    src/gui_elements/Datepicker.h \
    src/gui_elements/Dropdown.h \
    src/gui_elements/FilePath.h \
    src/gui_elements/gui_elements.h \
    src/gui_elements/GridPanel.h \
    src/gui_elements/Group.h \
    src/gui_elements/Helptext.h \
    src/gui_elements/HorizontalPanel.h \
    src/gui_elements/Label.h \
    src/gui_elements/Number.h \
    src/gui_elements/Replicator.h \
    src/gui_elements/Selector.h \
    src/gui_elements/Spacer.h \
    src/gui_elements/Textfield.h \
    src/main/XMLReader.h \
    src/main/colors.h \
    src/main/common.h \
    src/main/constants.h \
    src/main/dimensions.h \
    src/main/Error.h \
    src/main/INIParser.h \
    src/main/expressions.h \
    src/main/inishell.h \
    src/main/os.h \
    src/main/settings.h \
    lib/tinyexpr.h

#automatic creation of .qm language files from .ts language dictionaries:
LANGUAGES = de
for(language, LANGUAGES) {
    TRANSLATIONS += "resources/languages/inishell_$${language}.ts"
}
TRANSLATIONS_FILES = ""
qtPrepareTool(LRELEASE, lrelease) #get real lrelease tool
for(file_ts, TRANSLATIONS) {
    file_qm = $$file_ts #same path as .ts (not build) because it will be embedded as resource
    file_qm ~= s,.ts$,.qm #regex replacement
    cmd = $$LRELEASE -removeidentical $$file_ts -qm $$file_qm
    system($$cmd) | message("Error: could not execute '$${cmd}'")
    TRANSLATIONS_FILES += $$file_qm
}
#TRANSLATIONS = resources/languages/inishell_de.ts #manual way

DESTDIR = ./build #put executable here
MOC_DIR = ./tmp/obj #clutter build directory instead of source
OBJECTS_DIR = $$MOC_DIR

#deploy configuration
isEmpty(TARGET_EXT) {
        win32 {
                TARGET_CUSTOM_EXT = .exe
        }
        macx {
                TARGET_CUSTOM_EXT = .app
        }
} else {
        TARGET_CUSTOM_EXT = $${TARGET_EXT}
}
DEPLOY_TARGET = $$shell_quote($$shell_path($${DESTDIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))

win32 {
    DEPLOY_COMMAND = $$shell_quote($$shell_path($$[QT_INSTALL_BINS]\windeployqt --no-system-d3d-compiler --no-opengl-sw --no-angle --no-opengl))
    # Use += instead of = if you use multiple QMAKE_POST_LINKs
    # Force windeployqt to run in cmd, because powershell has different syntax
    # for running executables.
    QMAKE_POST_LINK = cmd /c $${DEPLOY_COMMAND} $${DEPLOY_TARGET}
}
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
    DEPLOY_COMMAND = $$shell_quote($$shell_path($$[QT_INSTALL_BINS]\macdeployqt))
    # Use += instead of = if you use multiple QMAKE_POST_LINKs
    QMAKE_POST_LINK = $${DEPLOY_COMMAND} $${DEPLOY_TARGET} -dmg -always-overwrite -verbose=1
}


#  # Uncomment the following line to help debug the deploy command when running qmake
#warning($${DEPLOY_COMMAND} $${DEPLOY_TARGET})

message("Configuration done.")
