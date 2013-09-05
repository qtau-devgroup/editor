#ifndef QTAU_AUDIO_CODECBASE_H
#define QTAU_AUDIO_CODECBASE_H

// basic codecs for QTau - wav, flac and ogg
// mp3, aac/m4a and true audio should be added via plugins

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
    qtauWavCodecFactory() { _ext = "wav"; _mime = "audio/wav"; }
    qtauAudioCodec* make(QIODevice &d, QObject *parent = 0) { return new qtauWavCodec(d, parent); }
};

//----------------

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
    qtauFlacCodecFactory() { _ext = "flac"; _mime = "audio/flac"; }
    qtauAudioCodec* make(QIODevice &d, QObject *parent = 0) { return new qtauFlacCodec(d, parent); }
};

//----------------

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
    qtauOggCodecFactory() { _ext = "ogg"; _mime = "audio/ogg"; }
    qtauAudioCodec* make(QIODevice &d, QObject *parent = 0) { return new qtauOggCodec(d, parent); }
};


#endif // QTAU_AUDIO_CODECBASE_H
