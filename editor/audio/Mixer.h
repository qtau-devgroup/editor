/* Mixer.h from QTau http://github.com/qtau-devgroup/editor by digited and HAL@ShurabaP, BSD license */

#ifndef QTAU_AUDIO_MIXER_H
#define QTAU_AUDIO_MIXER_H

#include "audio/Source.h"

class qtauSoundMixer : public qtauAudioSource
{
    Q_OBJECT
public:
    explicit qtauSoundMixer(QObject *parent = 0);

    void addTrack (qtauAudioSource *t, bool replace = false, bool smoothly = true);
    void addEffect(qtauAudioSource *e, bool replace = false, bool smoothly = true);

    //--- QIODevice interface functions ---------
    bool   isSequential()   const  { return true;  } // always sequential
    qint64 pos()            const  { return 0;     } // don't have one.
    bool   seek(qint64)            { return false; } // nope.
    bool   reset()                 { return false; } // what? NO.
    bool   atEnd()          const  { return tracks.isEmpty() && effects.isEmpty(); }
    qint64 size()           const  { return bytesAvailable(); }
    qint64 bytesToWrite()   const  { return 0;     } // unwritable, use addTrack/addEffect

    qint64 bytesAvailable() const;
    //-------------------------------------------

    void clear()        { clearTracks(); clearEffects(); }
    void clearTracks()  { tracks.clear();  emit allTracksEnded();  }
    void clearEffects() { effects.clear(); emit allEffectsEnded(); }

    /* hack for QAudioOutput - mixer will continue to stream zeros even when no more data */
    void streamZeros(bool doSo = true) { genZeros = doSo; }

signals:
    void allTracksEnded();
    void allEffectsEnded();

    void trackEnded(qtauAudioSource*);
    void effectEnded(qtauAudioSource*);

protected:
    QList<qtauAudioSource*> tracks; // keeps its own copy of audio data because original may be chaged, in another thread even
    QList<qtauAudioSource*> effects;

    bool genZeros;
    bool replacingEffectsSmoothly;
    bool replacingTracksSmoothly;

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *, qint64)      { return 0; } // unwritable, use addTrack/addEffect

private:
    static void extract(QVector<float> &buffer, int sampleRate, int frameCount, int channelCount, qtauAudioSource *source);

};

#endif // QTAU_AUDIO_MIXER_H
