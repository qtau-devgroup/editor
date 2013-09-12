/* Mixer.h from QTau http://github.com/qtau-devgroup/editor by digited and HAL@ShurabaP, BSD license */

#ifndef QTAU_AUDIO_MIXER_H
#define QTAU_AUDIO_MIXER_H

#include "audio/Source.h"

/* Audio Mixer is aimed to be used for mix-on-demand, always ready to accept a new source to be mixed in.
 * Mixer does NOT manage memory of audio sources - they were created somewhere and must be deleted there too
 * To mix audio data: use constructor with list of audio sources, do readAll() */
class qtauSoundMixer : public qtauAudioSource
{
    Q_OBJECT

public:
    explicit qtauSoundMixer(QObject *parent = 0);
    explicit qtauSoundMixer(QList<qtauAudioSource*> &tracks, QObject *parent = 0);

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

signals:
    void allTracksEnded();
    void allEffectsEnded();

    void trackEnded(qtauAudioSource*);
    void effectEnded(qtauAudioSource*);

protected:
    QList<qtauAudioSource*> tracks; // keeps its own copy of audio data because original may be chaged, in another thread even
    QList<qtauAudioSource*> effects;

    bool replacingEffectsSmoothly = false;
    bool replacingTracksSmoothly  = false;

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *, qint64)      { return 0; } // unwritable, use addTrack/addEffect

};

#endif // QTAU_AUDIO_MIXER_H
