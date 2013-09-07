/* Ogg.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef QTAU_CODEC_OGG_H
#define QTAU_CODEC_OGG_H

#include "audio/Codec.h"

class qtauOggCodec : public qtauAudioCodec
{
    Q_OBJECT
    friend class qtauOggCodecFactory;

public:
    bool cacheAll();
    bool saveToDevice();

protected:
    qtauOggCodec(QIODevice &d, QObject *parent = 0);

};

class qtauOggCodecFactory : public qtauAudioCodecFactory
{
public:
    qtauOggCodecFactory()
    {
        _ext  = "ogg";
        _mime = "audio/ogg";
        _desc = "Ogg Vorbis lossy audio";
    }

    qtauAudioCodec* make(QIODevice &d, QObject *parent = 0)
    {
        return new qtauOggCodec(d, parent);
    }
};

#endif // QTAU_CODEC_OGG_H
