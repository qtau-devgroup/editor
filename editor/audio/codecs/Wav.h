#ifndef QTAU_CODEC_WAV_H
#define QTAU_CODEC_WAV_H

#include "audio/Codec.h"

class qtauWavCodec : public qtauAudioCodec
{
    Q_OBJECT
    friend class qtauWavCodecFactory;

public:
    bool cacheAll();
    bool saveToDevice();

protected:
    qtauWavCodec(QIODevice &d, QObject *parent = 0);

    bool findFormatChunk();
    bool findDataChunk();

    quint64 _data_chunk_location;  // bytes
    int     _data_chunk_length;    // in frames

};

class qtauWavCodecFactory : public qtauAudioCodecFactory
{
public:
    qtauWavCodecFactory()
    {
        _ext  = "wav";
        _mime = "audio/wav";
        _desc = "Microsoft lossless audio";
    }

    qtauAudioCodec* make(QIODevice &d, QObject *parent = 0)
    {
        return new qtauWavCodec(d, parent);
    }
};

#endif // QTAU_CODEC_WAV_H
