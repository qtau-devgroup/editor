/* AIFF.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef QTAU_CODEC_AIFF_H
#define QTAU_CODEC_AIFF_H

#include "audio/Codec.h"

class QDataStream;


class qtauAIFFCodec : public qtauAudioCodec
{
    Q_OBJECT
    friend class qtauAIFFCodecFactory;

public:
    bool cacheAll();
    bool saveToDevice();

protected:
    qtauAIFFCodec(QIODevice &d, QObject *parent = 0);

    bool findCommonChunk(QDataStream &reader);
    bool findSoundChunk(QDataStream &reader);

    quint64 _data_chunk_location;  // bytes
    int     _data_chunk_length;    // in frames

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
