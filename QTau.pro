#-------------------------------------------------
# http://github.com/digited/qtau
#-------------------------------------------------

QT += core widgets network multimedia

TARGET = QTau
TEMPLATE = app

INCLUDEPATH += tools

SOURCES += \
    editor/main.cpp \
    editor/mainwindow.cpp \
    editor/Session.cpp \
    editor/Controller.cpp \
    tools/utauloid/ust.cpp \
    editor/ui/piano.cpp \
    editor/ui/noteEditor.cpp \
    editor/ui/dynDrawer.cpp \
    editor/ui/meter.cpp \
    editor/Utils.cpp \
    editor/ui/noteEditorHandlers.cpp \
    editor/ui/waveform.cpp \
    editor/audio/Source.cpp \
    editor/audio/Player.cpp \
    editor/audio/Mixer.cpp \
    editor/audio/Codec.cpp \
    editor/audio/CodecBase.cpp

HEADERS  += \
    editor/mainwindow.h \
    editor/PluginInterfaces.h \
    editor/Events.h \
    editor/Controller.h \
    editor/Session.h \
    tools/utauloid/ust.h \
    editor/ui/piano.h \
    editor/ui/noteEditor.h \
    editor/ui/dynDrawer.h \
    editor/ui/meter.h \
    editor/ui/Config.h \
    editor/Utils.h \
    editor/NoteEvents.h \
    editor/ui/noteEditorHandlers.h \
    tools/ffft/OscSinCos.hpp \
    tools/ffft/OscSinCos.h \
    tools/ffft/FFTRealUseTrigo.hpp \
    tools/ffft/FFTRealUseTrigo.h \
    tools/ffft/FFTRealSelect.hpp \
    tools/ffft/FFTRealSelect.h \
    tools/ffft/FFTRealPassInverse.hpp \
    tools/ffft/FFTRealPassInverse.h \
    tools/ffft/FFTRealPassDirect.hpp \
    tools/ffft/FFTRealPassDirect.h \
    tools/ffft/FFTRealFixLenParam.h \
    tools/ffft/FFTRealFixLen.hpp \
    tools/ffft/FFTRealFixLen.h \
    tools/ffft/FFTReal.hpp \
    tools/ffft/FFTReal.h \
    tools/ffft/DynArray.hpp \
    tools/ffft/DynArray.h \
    tools/ffft/def.h \
    tools/ffft/Array.hpp \
    tools/ffft/Array.h \
    editor/ui/waveform.h \
    editor/audio/Source.h \
    editor/audio/Player.h \
    editor/audio/Mixer.h \
    editor/audio/Codec.h \
    editor/audio/CodecBase.h

FORMS += editor/ui/mainwindow.ui

RESOURCES += editor/res/qtau.qrc

windows:RC_FILE = editor/res/qtau_win.rc

QMAKE_CXXFLAGS += -Wunused-parameter
