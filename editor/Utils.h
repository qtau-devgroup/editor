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

// conversion table of pianoroll keyboard key code (C1-B7) to fundamental frequency
//float *semitoneToFrequency[] = { 0, // skip index 0 since note numbers start from 1
//    32.7,   34.6,   36.7,   38.9,   41.2,   43.7,   46.2,   49.0,   51.9,   55.0,   58.3,   61.7,    // 1st octave
//    65.4,   69.3,   73.4,   77.8,   82.4,   87.3,   92.5,   98.0,   103.8,  110.0,  116.5,  123.5,   // 2nd
//    130.8,  138.6,  146.8,  155.6,  164.8,  174.6,  185.0,  196.0,  207.7,  220.0,  233.1,  246.9,   // 3rd
//    261.6,  277.2,  293.7,  311.1,  329.6,  349.2,  370.0,  392.0,  415.3,  440.0,  466.2,  493.9,   // 4th
//    523.3,  554.4,  587.3,  622.3,  659.3,  698.5,  740.0,  784.0,  830.6,  880.0,  932.3,  987.8,   // 5th
//    1046.5, 1108.7, 1174.7, 1244.5, 1318.5, 1396.9, 1480.0, 1568.0, 1661.2, 1760.0, 1864.7, 1975.5,  // 6th
//    2093.0, 2217.5, 2349.3, 2489.0, 2637.0, 2793.8, 2960.0, 3136.0, 3322.4, 3520.0, 3729.3, 3951.1}; // 7th


#endif // UTILS_H
