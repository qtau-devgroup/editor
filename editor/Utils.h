/* Utils.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QPoint>
#include <QSize>
#include <QPair>


const QString c_qtau_name = QStringLiteral("QTau");
const QString c_qtau_ver  = QStringLiteral("α1");

/* Pulses Per Quarter note - standard timing resolution of a MIDI sequencer,
     needs to be divisible by 16 and 3 to support 1/64 notes and triplets.
     Used to determine note offset (in bars/seconds) and duration */
const int c_midi_ppq       = 480;
const int c_zoom_num       = 17;
const int cdef_zoom_index  = 4;
const int cdef_note_height = 14;

const int c_zoom_note_widths[c_zoom_num] = {16,  32,  48,  64,  80,
                                            96,  112, 128, 144, 160,
                                            176, 208, 240, 304, 368, 480, 608};

struct SNoteSetup {
    QSize note;     // gui dimensions (in pixels)

    int baseOctave;
    int numOctaves;

    int noteLength; // denominator of note length 1
    int notesInBar; // noteLength=4 and notesInBar=4 means music time signature 4/4
    int tempo;      // bpm
    int quantize;   // 1/x note length, used as unit of offset for singing notes
    int length;     // 1/x singing note length unit (len=4 means 1/4, 2/4 etc +1/4)

    int barWidth;
    int octHeight;

    SNoteSetup() : note(c_zoom_note_widths[cdef_zoom_index], cdef_note_height),
        baseOctave(1), numOctaves(7), noteLength(4), notesInBar(4), tempo(120), quantize(32), length(32),
        barWidth(note.width() * notesInBar), octHeight(note.height() * 12) {}
};
//----------------------------------------------------

enum class EPlayer : char {
    stopped,
    playing,
    paused,
    repeating
};

enum class EAudioPlayback : char {
    noAudio,
    needsSynth,
    playing,
    paused,
    stopped,
    repeating
};

enum class ELog : char {
    none,
    info,
    debug,
    error,
    success
};


class vsLog : public QObject
{
    Q_OBJECT

public:
    vsLog() : lastTime(QStringLiteral("none")), saving(true) {}
    static vsLog* instance();

    static void i(const QString &msg) { instance()->addMessage(msg, ELog::info);    }
    static void d(const QString &msg) { instance()->addMessage(msg, ELog::debug);   }
    static void e(const QString &msg) { instance()->addMessage(msg, ELog::error);   }
    static void s(const QString &msg) { instance()->addMessage(msg, ELog::success); }
    static void n()                   { instance()->addMessage("",  ELog::none);    }
    static void r(); // reemit stored message (removing it from history)

    void enableHistory(bool enable) { saving = enable; }

public slots:
    void addMessage(const QString&, ELog);

signals:
    void message(QString, ELog);  // int is msg type from enum "vsLog::msgType"

protected:
    QString lastTime;
    bool    saving;

    QList<QPair<ELog, QString>> history;
    void reemit(const QString &msg, ELog type);

};

union uichar {
    char    c[4];
    quint32 i;
};

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
