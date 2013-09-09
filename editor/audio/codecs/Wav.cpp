/* Wav.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "audio/codecs/Wav.h"
#include "audio/Resampler.h"
#include "Utils.h"
#include <qendian.h>


//----- WAV PCM RIFF header parts -----------------------
// all char[] must be big-endian, while all integers should be unsigned little-endian

typedef struct SWavRiff {
    uichar  chunkID;     // "RIFF" 0x52494646 BE
    quint32 chunkSize;
    uichar  chunkFormat; // "WAVE" 0x57415645 BE

    void clear() { memset(chunkID.c, 0, sizeof(SWavRiff)); }

    SWavRiff(const qint64 bufferSize = 0)
    {
        clear();

        if (bufferSize > 0)
        {
            memcpy(chunkID.c,     "RIFF", 4);
            memcpy(chunkFormat.c, "WAVE", 4);
            chunkSize = 36 + bufferSize;
        }
    }

    SWavRiff(QDataStream &reader)
    {
        reader.setByteOrder(QDataStream::LittleEndian);
        reader >> chunkID.i;
        reader >> chunkSize;
        reader >> chunkFormat.i;

        if (reader.status() != QDataStream::Ok)
            chunkID.i = 0;
    }

    void write(QDataStream &writer)
    {
        if (isCorrect())
        {
            writer.setByteOrder(QDataStream::LittleEndian);
            writer << chunkID.i;
            writer << chunkSize;
            writer << chunkFormat.i;
        }
        else vsLog::e("Wav RIFF header is incorrect, writing to device is cancelled.");
    }

    bool isCorrect()
    {
        return !memcmp(chunkID.c, "RIFF", 4) && !memcmp(chunkFormat.c, "WAVE", 4) && chunkSize > 0;
    }
} wavRIFF;


typedef struct SWavFmt {
    uichar  fmtChunkID;  // "fmt "  0x666d7420 BE
    quint32 fmtSize;
    quint16 audioFormat;    // 2
    quint16 numChannels;    // 2
    quint32 sampleRate;     // 4
    quint32 byteRate;       // 4
    quint16 blockAlign;     // 2
    quint16 bitsPerSample;  // 2

    void clear() { memset(fmtChunkID.c, 0, sizeof(SWavFmt)); }

    SWavFmt() { clear(); }

    SWavFmt(const QAudioFormat &fmt)
    {
        memcpy(fmtChunkID.c, "fmt ", 4);

        fmtSize       = 16;
        audioFormat   = 1;
        numChannels   = fmt.channelCount();
        sampleRate    = fmt.sampleRate();
        byteRate      = fmt.bytesPerFrame() * sampleRate;
        blockAlign    = fmt.bytesPerFrame();
        bitsPerSample = fmt.sampleSize();
    }

    SWavFmt(QDataStream &reader)
    {
        reader.setByteOrder(QDataStream::LittleEndian);
        reader >> fmtChunkID.i;
        reader >> fmtSize;

        if (isCorrect())
        {
            reader >> audioFormat;
            reader >> numChannels;
            reader >> sampleRate;
            reader >> byteRate;
            reader >> blockAlign;
            reader >> bitsPerSample;

            if (fmtSize > 16)
                reader.skipRawData(fmtSize - 16);
        }

        if (!(isCorrect() && reader.status() == QDataStream::Ok))
            fmtChunkID.i = 0;
    }

    void write(QDataStream &writer)
    {
        if (isCorrect())
        {
            writer.setByteOrder(QDataStream::LittleEndian);
            writer << fmtChunkID.i;
            writer << fmtSize;
            writer << audioFormat;
            writer << numChannels;
            writer << sampleRate;
            writer << byteRate;
            writer << blockAlign;
            writer << bitsPerSample;
        }
        else vsLog::e("Wav Fmt chunk is incorrect, writing to device cancelled.");
    }

    // if those are read correctly, rest should be ok
    bool isCorrect() { return !memcmp(fmtChunkID.c, "fmt ", 4) && fmtSize >= 16; }

} wavFmt;


typedef struct SWavData {
    uichar  dataID;    // "data" 0x64617461 BE
    quint32 dataSize;

    void clear() { memset(dataID.c, 0, 8); }

    SWavData(const qint64 bufferSize = 0)
    {
        clear();

        if (bufferSize > 0)
        {
            memcpy(dataID.c, "data", 4);
            dataSize = bufferSize;
        }
    }

    SWavData(QDataStream &reader)
    {
        reader.setByteOrder(QDataStream::LittleEndian);
        reader >> dataID.i;
        reader >> dataSize;
    }

    void write(QDataStream &writer)
    {
        if (isCorrect())
        {
            writer.setByteOrder(QDataStream::LittleEndian);
            writer << dataID.i;
            writer << dataSize;
        }
        else vsLog::e("Wav data chunk header is incorrect, writing to device is cancelled.");
    }

    bool isCorrect() { return !memcmp(dataID.c, "data", 4) && dataSize > 0; }

} wavData;

//-------------------------------------------------------


bool qtauWavCodec::cacheAll()
{
    bool result = false;

    if (dev->bytesAvailable() > 0)
    {
        if (!dev->isSequential())
            dev->reset();

        QDataStream reader(dev);
        wavRIFF rh(reader);

        if (rh.isCorrect())
            result = findFormatChunk(reader) && findDataChunk(reader);
        else
            vsLog::e("Wav codec couldn't read RIFF header");
    }
    else vsLog::e("Audio Wav: empty data");

    if (result)
    {
        if (!dev->isSequential())
            dev->seek(_data_chunk_location); // else it should already be there

        fmt.setCodec("audio/pcm");
        fmt.setByteOrder(QAudioFormat::LittleEndian);

        open(QIODevice::WriteOnly);
        write(dev->read(_data_chunk_length * fmt.sampleSize() / 8 * fmt.channelCount()));
        close();
    }

    return result;
}


bool qtauWavCodec::saveToDevice()
{
    bool result = false;

    wavRIFF wavR(size());
    wavFmt  wavF(fmt);
    wavData wavD(size());

    if (!dev->isWritable())
        dev->open(QIODevice::WriteOnly);

    if (dev->isWritable())
    {
        if (!dev->isSequential())
            dev->reset();

        QDataStream writer(dev);

        wavR.write(writer);
        wavF.write(writer);
        wavD.write(writer);

        QAudioFormat wavSaveFormat; // always saving wav as S16 LE whatever buffer may hold
        wavSaveFormat.setByteOrder(QAudioFormat::LittleEndian);
        wavSaveFormat.setChannelCount(fmt.channelCount());
        wavSaveFormat.setSampleRate(fmt.sampleRate());
        wavSaveFormat.setSampleSize(16);
        wavSaveFormat.setSampleType(QAudioFormat::SignedInt);

        qtauResampler rsmp(buffer(), fmt, wavSaveFormat);
        dev->write(rsmp.encode()); // don't close device because who knows what is it - could end badly if it was a socket

        result = true;
    }
    else vsLog::e("Wav codec could not open iodevice for writing, saving cancelled.");

    return result;
}


qtauWavCodec::qtauWavCodec(QIODevice &d, QObject *parent) :
    qtauAudioCodec(d, parent)
{
    if (!d.isOpen())
        vsLog::e("Wav codec got a closed io device!");
}


bool qtauWavCodec::findFormatChunk(QDataStream &reader)
{
    bool result = false;

    // search for a format chunk
    while (true)
    {
        wavFmt wf(reader);

        if (wf.isCorrect())
        {
            if      (wf.bitsPerSample == 8)  fmt.setSampleType(QAudioFormat::UnSignedInt);
            else if (wf.bitsPerSample == 32) fmt.setSampleType(QAudioFormat::Float);
            else                             fmt.setSampleType(QAudioFormat::SignedInt);

            fmt.setSampleSize  (wf.bitsPerSample);
            fmt.setChannelCount(wf.numChannels  );
            fmt.setSampleRate  (wf.sampleRate   );

            result = true;
            break;
        }
        else // that's not the chunk we're looking for, need to skip it
        {
            if (wf.fmtSize == 0 || wf.fmtSize != (quint32)reader.skipRawData(wf.fmtSize))
            {
                vsLog::e("Wav codec could not skip a wrong chunk in findFormatChunk(). Wav reading failed then.");
                break;
            }
        }
    }

    return result;
}


bool qtauWavCodec::findDataChunk(QDataStream &reader)
{
    bool result = false;

    // search for a data chunk
    while (true)
    {
        wavData wd(reader);

        if (wd.isCorrect())
        {
            _data_chunk_location = dev->pos();
            _data_chunk_length   = wd.dataSize / (fmt.sampleSize() / 8 * fmt.channelCount());

            result = true;
            break;
        }
        else // that's not the chunk we're looking for, need to skip it
        {
            if (wd.dataSize == 0 || wd.dataSize != (quint32)reader.skipRawData(wd.dataSize))
            {
                vsLog::e("Wav codec could not skip a wrong chunk in findDataChunk(). Wav reading failed then.");
                break;
            }
        }
    }

    return result;
}
