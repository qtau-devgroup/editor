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
    audio/codecs/Ogg.cpp \
    audio/Resampler.cpp \

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
    audio/codecs/Ogg.h \
    audio/Resampler.h

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

INCLUDEPATH += ../tools/libogg-1.3.1/include ../tools/flac-1.3.0/include ../tools/flac-1.3.0/src/libFLAC/include

DEFINES += HAVE_CONFIG_H

CONFIG(linux):DEFINES  += FLAC__SYS_LINUX
CONFIG(darwin):DEFINES += FLAC__SYS_DARWIN

QMAKE_CFLAGS += -std=c99

SOURCES += \
    ../tools/libogg-1.3.1/src/bitwise.c \
    ../tools/libogg-1.3.1/src/framing.c \
    ../tools/flac-1.3.0/src/libFLAC/bitmath.c \
    ../tools/flac-1.3.0/src/libFLAC/bitreader.c \
    ../tools/flac-1.3.0/src/libFLAC/bitwriter.c \
    ../tools/flac-1.3.0/src/libFLAC/cpu.c \
    ../tools/flac-1.3.0/src/libFLAC/crc.c \
    ../tools/flac-1.3.0/src/libFLAC/fixed.c \
    ../tools/flac-1.3.0/src/libFLAC/float.c \
    ../tools/flac-1.3.0/src/libFLAC/format.c \
    ../tools/flac-1.3.0/src/libFLAC/lpc.c \
    ../tools/flac-1.3.0/src/libFLAC/md5.c \
    ../tools/flac-1.3.0/src/libFLAC/memory.c \
    ../tools/flac-1.3.0/src/libFLAC/metadata_iterators.c \
    ../tools/flac-1.3.0/src/libFLAC/metadata_object.c \
    ../tools/flac-1.3.0/src/libFLAC/stream_decoder.c \
    ../tools/flac-1.3.0/src/libFLAC/stream_encoder.c \
    ../tools/flac-1.3.0/src/libFLAC/stream_encoder_framing.c \
    ../tools/flac-1.3.0/src/libFLAC/window.c \
    ../tools/flac-1.3.0/src/libFLAC/ogg_decoder_aspect.c \
    ../tools/flac-1.3.0/src/libFLAC/ogg_encoder_aspect.c \
    ../tools/flac-1.3.0/src/libFLAC/ogg_helper.c \
    ../tools/flac-1.3.0/src/libFLAC/ogg_mapping.c
