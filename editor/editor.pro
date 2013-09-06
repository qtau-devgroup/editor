#-------------------------------------------------
# http://github.com/qtau-devgroup/editor
#-------------------------------------------------

QT += core widgets network multimedia

TARGET = QTau
TEMPLATE = app

INCLUDEPATH += ../tools

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    Session.cpp \
    Controller.cpp \
    ui/piano.cpp \
    ui/noteEditor.cpp \
    ui/dynDrawer.cpp \
    ui/meter.cpp \
    Utils.cpp \
    ui/noteEditorHandlers.cpp \
    ui/waveform.cpp \
    audio/Source.cpp \
    audio/Player.cpp \
    audio/Mixer.cpp \
    audio/Codec.cpp \
    ../tools/utauloid/ust.cpp \
    audio/codecs/Wav.cpp \
    audio/codecs/AIFF.cpp \
    audio/codecs/Flac.cpp \
    audio/codecs/Ogg.cpp

HEADERS  += \
    mainwindow.h \
    PluginInterfaces.h \
    Events.h \
    NoteEvents.h \
    Controller.h \
    Session.h \
    ui/piano.h \
    ui/noteEditor.h \
    ui/dynDrawer.h \
    ui/meter.h \
    ui/Config.h \
    Utils.h \
    ui/noteEditorHandlers.h \
    ui/waveform.h \
    audio/Source.h \
    audio/Player.h \
    audio/Mixer.h \
    audio/Codec.h \
    ../tools/utauloid/ust.h \
    audio/codecs/Wav.h \
    audio/codecs/AIFF.h \
    audio/codecs/Flac.h \
    audio/codecs/Ogg.h

FORMS += ui/mainwindow.ui

RESOURCES += res/qtau.qrc

windows:RC_FILE = res/qtau_win.rc

QMAKE_CXXFLAGS += -Wall -std=c++11

#--------------------------------------------
CONFIG(debug, debug|release) {
    DESTDIR = $${OUT_PWD}/../debug
} else {
    DESTDIR = $${OUT_PWD}/../release
}

OBJECTS_DIR     = $${DESTDIR}/editor/.obj
MOC_DIR         = $${DESTDIR}/editor/.moc
RCC_DIR         = $${DESTDIR}/editor/.rcc
UI_DIR          = $${DESTDIR}/editor/.ui
#--------------------------------------------
