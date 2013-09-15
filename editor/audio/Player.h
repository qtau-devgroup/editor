/* Player.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef QTAU_AUDIO_PLAYER_H
#define QTAU_AUDIO_PLAYER_H

#include <QObject>
#include <QAudio>
#include <QIODevice>

class QAudioOutput;
class qtauAudioSource;
class qtauSoundMixer;

class QTimer;


// player is designed to work in a separate thread to avoid audio glitches on playback
// stores copies of audio sources, uses mixer to combine their data, adds zeros if asked for more data than mixer has
class qtmmPlayer : public QIODevice
{
    Q_OBJECT

public:
    qtmmPlayer();
    ~qtmmPlayer();

    bool   isSequential()   const override { return true;   }
    qint64 pos()            const override { return 0;      }
    bool   seek(qint64)           override { return false;  }
    bool   atEnd()          const override { return bytesAvailable() == 0;   }
    bool   reset()                override { return false;  }
    qint64 bytesAvailable() const override { return size(); }
    qint64 size()           const override;

signals:
    void playbackEnded();
    void tick(qint64 mcsec);

public slots: // all slots should be called indirectly with connect + emit because player is in separate thread
    void addEffect(qtauAudioSource *e, bool replace = false, bool smoothly = true, bool copy = true);
    void addTrack (qtauAudioSource *t, bool replace = false, bool smoothly = true, bool copy = true);

    void play();
    void pause();
    void stop();

    void setVolume(int level); // 0..100

    void threadedInit(); // should be called after instance is moved to a separate thread

private slots:
    void onQtmmStateChanged(QAudio::State);
    void onTick();

    void onEffectEnded(qtauAudioSource* e);
    void onTrackEnded(qtauAudioSource* t);

    void onAllEffectsEnded();
    void onAllTracksEnded();

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *, qint64)     override { return 0; }

    QList<qtauAudioSource*> tracks;  // copies are stored here to ensure playback if originals will change
    QList<qtauAudioSource*> effects; // tracks and effects are read in a separate thread

    QAudioOutput   *audioOutput;
    qtauSoundMixer *mixer;
    QTimer         *stopTimer;

    int volume;

};

#endif // QTAU_AUDIO_PLAYER_H
