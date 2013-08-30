#ifndef QTAU_AUDIO_PLAYER_H
#define QTAU_AUDIO_PLAYER_H

#include <QObject>
#include <QAudio>

class QAudioOutput;
class qtauAudioSource;


class qtmmPlayer : public QObject
{
    Q_OBJECT

public:
    qtmmPlayer(QObject *parent = 0);
    ~qtmmPlayer();

    bool play(qtauAudioSource *a = 0);
    void pause();
    void stop();

    void setVolume(int level); // 0..100

signals:
    void playbackEnded();
    void tick(qint64 mcsec);

public slots:
    void onQtmmStateChanged(QAudio::State);
    void onTick();

protected:
    QAudioOutput *audioOutput;
    int volume;

};

#endif // QTAU_AUDIO_PLAYER_H
