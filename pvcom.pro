QT += widgets serialport
requires(qtConfig(combobox))

TARGET = pvcom
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

RESOURCES += \
    pvcom.qrc

#target.path = $$[QT_INSTALL_EXAMPLES]/serialport/terminal
#INSTALLS += target
