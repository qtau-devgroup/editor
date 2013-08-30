#ifndef QTAU_AUDIO_MIXER_H
#define QTAU_AUDIO_MIXER_H

#include "editor/audio/Source.h"

class qtauSoundMixer : public qtauAudioSource
{
    Q_OBJECT
public:
    explicit qtauSoundMixer(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // QTAU_AUDIO_MIXER_H
