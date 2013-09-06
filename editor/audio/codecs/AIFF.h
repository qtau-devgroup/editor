#ifndef QTAU_CODEC_AIFF_H
#define QTAU_CODEC_AIFF_H

#include "audio/Codec.h"

class qtauAIFFCodec : public qtauAudioCodec
{
    Q_OBJECT
    friend class qtauAIFFCodecFactory;

public:
    bool cacheAll();
    bool saveToDevice();

protected:
    qtauAIFFCodec(QIODevice &d, QObject *parent = 0);

};

class qtauAIFFCodecFactory : public qtauAudioCodecFactory
{
public:
    qtauAIFFCodecFactory()
    {
        _ext  = "aiff";
        _mime = "audio/aiff";
        _desc = "Apple lossless audio";
    }

    qtauAudioCodec* make(QIODevice &d, QObject *parent = 0)
    {
        return new qtauAIFFCodec(d, parent);
    }
};

#endif // QTAU_CODEC_AIFF_H
