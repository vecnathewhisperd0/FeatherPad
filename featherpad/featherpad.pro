QT += core gui \
      widgets \
      printsupport \
      network \
      x11extras \
      svg

TARGET = featherpad
TEMPLATE = app
CONFIG += c++11

SOURCES += main.cpp \
           singleton.cpp \
           fpwin.cpp \
           encoding.cpp \
           tabwidget.cpp \
           lineedit.cpp \
           textedit.cpp \
           tabbar.cpp \
           x11.cpp \
           highlighter.cpp \
           find.cpp \
           replace.cpp \
           pref.cpp \
           config.cpp \
           brackets.cpp \
           syntax.cpp \
           highlighter-html.cpp \
           highlighter-patterns.cpp \
           vscrollbar.cpp \
           loading.cpp

HEADERS += singleton.h \
           fpwin.h \
           encoding.h \
           tabwidget.h \
           lineedit.h \
           textedit.h \
           tabbar.h \
           x11.h \
           highlighter.h \
           vscrollbar.h \
           filedialog.h \
           config.h \
           pref.h \
           loading.h

FORMS += fp.ui \
         predDialog.ui

RESOURCES += data/fp.qrc

unix:!macx: LIBS += -lX11

unix {
  #VARIABLES
  isEmpty(PREFIX) {
    PREFIX = /usr
  }
  BINDIR = $$PREFIX/bin
  DATADIR =$$PREFIX/share

  DEFINES += DATADIR=\\\"$$DATADIR\\\" PKGDATADIR=\\\"$$PKGDATADIR\\\"

  #MAKE INSTALL

  INSTALLS += target desktop iconsvg help

  target.path =$$BINDIR

  desktop.path = $$DATADIR/applications
  desktop.files += ./data/$${TARGET}.desktop

  iconsvg.path = $$DATADIR/icons/hicolor/scalable/apps
  iconsvg.files += ./data/$${TARGET}.svg

  help.path = $$DATADIR/featherpad
  help.files += ./data/help
}
