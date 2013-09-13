/* Wav.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef QTAU_CODEC_WAV_H
#define QTAU_CODEC_WAV_H

#include "audio/Codec.h"

class QDataStream;

class qtauWavCodec : public qtauAudioCodec
{
    Q_OBJECT
    friend class qtauWavCodecFactory;

public:
    bool cacheAll()     override;
    bool saveToDevice() override;

protected:
    qtauWavCodec(QIODevice &d, QObject *parent = 0);

    bool findFormatChunk(QDataStream &reader);
    bool findDataChunk(QDataStream &reader);

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

    qtauAudioCodec* make(QIODevice &d, QObject *parent = 0) override
    {
        return new qtauWavCodec(d, parent);
    }
};

#endif // QTAU_CODEC_WAV_H
