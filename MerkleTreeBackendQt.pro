QT       += core gui widgets network


CONFIG += c++17


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    merkletree.cpp

HEADERS += \
    mainwindow.h \
    merkletree.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
INCLUDEPATH += C:\OpenSSL-Win64\include
LIBS += C:\OpenSSL-Win64\lib\VC\x64\MD\libssl.lib
LIBS += C:\OpenSSL-Win64\lib\VC\x64\MD\libcrypto.lib


