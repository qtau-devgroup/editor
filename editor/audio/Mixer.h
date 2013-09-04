#ifndef QTAU_AUDIO_MIXER_H
#define QTAU_AUDIO_MIXER_H

#include "audio/Source.h"

class qtauSoundMixer : public qtauAudioSource
{
    Q_OBJECT
public:
    explicit qtauSoundMixer(QObject *parent = 0);
    explicit qtauSoundMixer(const QList<qtauAudioSource *> &sources, QObject *parent = 0);

signals:
    
public slots:
private:
    static inline float getValueFromBuffer(char *buffer, const QAudioFormat &fmt);

};

#endif // QTAU_AUDIO_MIXER_H
