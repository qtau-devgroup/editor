#ifndef QTAU_CODEC_FLAC_H
#define QTAU_CODEC_FLAC_H

#include "audio/Codec.h"

class qtauFlacCodec : public qtauAudioCodec
{
    Q_OBJECT
    friend class qtauFlacCodecFactory;

public:
    bool cacheAll();
    bool saveToDevice();

protected:
    qtauFlacCodec(QIODevice &d, QObject *parent = 0);

};

class qtauFlacCodecFactory : public qtauAudioCodecFactory
{
public:
    qtauFlacCodecFactory()
    {
        _ext  = "flac";
        _mime = "audio/flac";
        _desc = "Free Lossless Audio Codec";
    }

    qtauAudioCodec* make(QIODevice &d, QObject *parent = 0)
    {
        return new qtauFlacCodec(d, parent);
    }
};

#endif // QTAU_CODEC_FLAC_H
