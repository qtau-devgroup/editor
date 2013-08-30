#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDebug>
#include <assert.h>
#include <QPoint>
#include <QSize>

#include <QtWidgets/QWidget>


const QString QTAU_VERSION = QString::fromUtf8("α1");

/* Pulses Per Quarter note - standard timing resolution of a MIDI sequencer,
     needs to be divisible by 16 and 3 to support 1/64 notes and triplets.
     Used to determine note offset (in bars/seconds) and duration */
const int MIDI_PPQ = 480;

const int CONST_ZOOMS = 17;
const int DEFAULT_ZOOM_INDEX = 4;
const int ZOOM_NOTE_WIDTHS[CONST_ZOOMS] = {16,  32,  48,  64,  80,
                                           96,  112, 128, 144, 160,
                                           176, 208, 240, 304, 368, 480, 608};

typedef struct _ns {
    QSize note;     // gui dimensions (in pixels)

    int baseOctave;
    int numOctaves;

    int noteLength; // denominator of note length 1
    int notesInBar; // noteLength=4 and notesInBar=4 means music time signature 4/4
    int tempo;      // bpm
    int quantize;   // 1/x note length, used as unit of offset for singing notes
    int length;     // 1/x singing note length unit (len=4 means 1/4, 2/4 etc +1/4)

    int barWidth;   // derived
    int octHeight;  // derived

    _ns() : note(ZOOM_NOTE_WIDTHS[DEFAULT_ZOOM_INDEX], 14), baseOctave(1), numOctaves(7),
        noteLength(4), notesInBar(4), tempo(120), quantize(32), length(32),
        barWidth(note.width()*notesInBar), octHeight(note.height()*12)
    {}
} noteSetup;
//----------------------------------------------------


class vsLog : public QObject
{
    Q_OBJECT

public:
    inline vsLog() : lastTime("none"), saving(true) {}
    ~vsLog();

    static vsLog* instance();

    typedef enum
    {
        info = 0,
        debug,
        error,
        success,
        none
    } msgType;

    static void i(const QString &msg); /// info
    static void d(const QString &msg); /// debug
    static void e(const QString &msg); /// error
    static void s(const QString &msg); /// success
    static void n();                   /// separator (empty line)
    static void r();                   /// reemit stored message (removing it from history)

    void enableHistory(bool enable) { saving = enable; }

public slots:
    void addMessage(QString, vsLog::msgType);

signals:
    void message(QString, int);  // int is msg type from enum "vsLog::msgType"

protected:
    QString lastTime;
    bool    saving;

    QList<QString> history;
    void reemit(const QString &msg, vsLog::msgType type);

};


int snap(int value, int unit, int baseValue = 0);  /// baseValue % unit is added to result


inline quint16 read16_le(const quint8* b) { return b[0] + (b[1] << 8); }
inline quint16 read16_be(const quint8* b) { return (b[0] << 8) + b[1]; }
inline quint32 read32_le(const quint8* b) { return read16_le(b) + (read16_le(b + 2) << 16); }
inline quint32 read32_be(const quint8* b) { return (read16_be(b) << 16) + read16_be(b + 2); }


namespace qtauSessionPlayback {
    typedef enum State {
        NothingToPlay = 0,
        NeedsSynthesis,
        Playing,
        Paused,
        Stopped,
        Repeating
    } EState;
}


#endif // UTILS_H
