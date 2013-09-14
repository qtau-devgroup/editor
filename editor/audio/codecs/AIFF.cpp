/* AIFF.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "audio/codecs/AIFF.h"
#include "audio/Resampler.h"
#include "Utils.h"

#include <QDataStream>
#include <qendian.h>
#include <qmath.h>
#include <stdint.h>


// 80 bit float to uint32
inline quint32 readLD_be(quint16 &exp, quint64 &mnt)
{
    return ldexp(mnt, exp - 16383 - 63); // 80bit float isn't supported everywhere. Hacks from libav
}

inline void writeLD_be(const quint32 &val, quint16 &exp, quint64 &mnt)
{
    union dui64 {
        double  d;
        quint64 i; // reinterpret for bit shifting
    };

    dui64 uVal;
    uVal.d = val; // convert to double
    exp = (uVal.i >> 52) + (16383 - 1023);   // get and change double's exponent
    mnt = UINT64_C(1) << 63 | uVal.i << 11;  // get double's mantissa
}

//------ AIFF RIFF headers, everything is in Big Endian (including PCM data) --------------------
typedef struct SAIFFHeader {
    uichar chunkID;  // "FORM"
    qint32 chunkSize;
    uichar formType; // "AIFF" // yes, form is aiff, and aiff header is form. Go ask Apple.

    void clear() { memset(chunkID.c, 0, sizeof(SAIFFHeader)); }

    SAIFFHeader(const qint64 bufferSize = 0)
    {
        clear();

        if (bufferSize > 0)
        {
            memcpy(chunkID.c,  "FORM", 4);
            memcpy(formType.c, "AIFF", 4);
            chunkSize = 8 + bufferSize;
        }
    }

    SAIFFHeader(QDataStream &reader)
    {
        reader.setByteOrder(QDataStream::LittleEndian); reader >> chunkID.i;
        reader.setByteOrder(QDataStream::BigEndian);    reader >> chunkSize;
        reader.setByteOrder(QDataStream::LittleEndian); reader >> formType.i;

        if (reader.status() != QDataStream::Ok)
            chunkID.i = 0;
    }

    void write(QDataStream &writer)
    {
        if (isCorrect())
        {
            writer.setByteOrder(QDataStream::LittleEndian); writer << chunkID.i;
            writer.setByteOrder(QDataStream::BigEndian);    writer << chunkSize;
            writer.setByteOrder(QDataStream::LittleEndian); writer << formType.i;
        }
        else vsLog::e("AIFF header is incorrect, writing to device is cancelled.");
    }

    bool isCorrect()
    {
        return !memcmp(chunkID.c, "FORM", 4) && !memcmp(formType.c, "AIFF", 4) && chunkSize > 0;
    }
} AIFFHeader;


typedef struct SAIFFCommon {
    uichar  fmtChunkID;  // "COMM"     4
    qint32  fmtSize;                // 4
    qint16  numChannels;            // 2
    quint32 numSampleFrames;        // 4
    qint16  sampleSize;             // 2
    quint32 sampleRate;             // 10 <- supposed to be "extended" (long double, 10 bytes)

    void clear() { memset(fmtChunkID.c, 0, sizeof(SAIFFCommon)); }

    SAIFFCommon() { clear(); }

    SAIFFCommon(const QAudioFormat &fmt, qint64 size)
    {
        memcpy(fmtChunkID.c, "COMM", 4);

        fmtSize         = 18; // by spec
        numChannels     = fmt.channelCount();
        numSampleFrames = size / (fmt.sampleSize() / 8 * fmt.channelCount());
        sampleSize      = fmt.sampleSize();
        sampleRate      = fmt.sampleRate();
    }

    SAIFFCommon(QDataStream &reader)
    {
        reader.setByteOrder(QDataStream::LittleEndian); reader >> fmtChunkID.i;
        reader.setByteOrder(QDataStream::BigEndian);    reader >> fmtSize;

        if (isCorrect())
        {
            reader >> numChannels;
            reader >> numSampleFrames;
            reader >> sampleSize;

            quint16 exp;
            quint64 mnt;
            reader >> exp;
            reader >> mnt;
            sampleRate = readLD_be(exp, mnt);
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

            writer.setByteOrder(QDataStream::BigEndian);
            writer << fmtSize;
            writer << numChannels;
            writer << numSampleFrames;
            writer << sampleSize;

            quint16 exp;
            quint64 mnt;
            writeLD_be(sampleRate, exp, mnt);
            writer << exp;
            writer << mnt;
        }
        else vsLog::e("AIFF common chunk is incorrect, writing to device cancelled.");
    }

    // if those are read correctly, rest should be ok
    bool isCorrect() { return !memcmp(fmtChunkID.c, "COMM", 4) && fmtSize == 18; }

} AIFFCommon;


typedef struct SAIFFData {
    uichar  chunkID;      // "SSND"
    qint32  chunkSize;
    quint32 offset;
    quint32 blockSize;

    void clear() { memset(chunkID.c, 0, sizeof(SAIFFData)); }

    SAIFFData(const qint64 bufferSize = 0)
    {
        clear();

        if (bufferSize > 0)
        {
            memcpy(chunkID.c, "SSND", 4);
            chunkSize = bufferSize + 8;
        }
    }

    SAIFFData(QDataStream &reader)
    {
        reader.setByteOrder(QDataStream::LittleEndian); reader >> chunkID.i;
        reader.setByteOrder(QDataStream::BigEndian);    reader >> chunkSize;

        if (isCorrect())
        {
            reader >> offset;
            reader >> blockSize;
        }

        if (!(isCorrect() && reader.status() == QDataStream::Ok))
            chunkID.i = 0;
    }

    void write(QDataStream &writer)
    {
        if (isCorrect())
        {
            writer.setByteOrder(QDataStream::LittleEndian);
            writer << chunkID.i;

            writer.setByteOrder(QDataStream::BigEndian);
            writer << chunkSize;
            writer << offset;
            writer << blockSize;
        }
        else
            vsLog::e("AIFF data chunk header is incorrect, writing to device is cancelled.");
    }

    bool isCorrect() { return !memcmp(chunkID.c, "SSND", 4) && chunkSize > 8; }

} AIFFData;


//===================================================================


qtauAIFFCodec::qtauAIFFCodec(QIODevice &d, QObject *parent) :
    qtauAudioCodec(d, parent)
{
    if (!d.isOpen())
        vsLog::e("AIFF codec got a closed io device!");
}


bool qtauAIFFCodec::cacheAll()
{
    bool result = false;

    if (dev->bytesAvailable() > 0)
    {
        if (!dev->isSequential())
            dev->reset();

        QDataStream reader(dev);
        AIFFHeader ah(reader);

        if (ah.isCorrect())
            result = findCommonChunk(reader) && findSoundChunk(reader);
        else
            vsLog::e("Wav codec couldn't read AIFF header");
    }
    else vsLog::e("Audio Wav: empty data");

    if (result)
    {
        if (!dev->isSequential())
            dev->seek(_data_chunk_location); // else it should already be there

        fmt.setCodec("audio/pcm");
        fmt.setByteOrder(QAudioFormat::BigEndian);
        fmt.setSampleType(QAudioFormat::SignedInt);

        QAudioFormat preferredFmt = fmt;
        preferredFmt.setByteOrder(QAudioFormat::LittleEndian);
        preferredFmt.setSampleSize(16);
        preferredFmt.setSampleType(QAudioFormat::SignedInt);

        qtauResampler rsmp(dev->read(_data_chunk_length * fmt.sampleSize() / 8 * fmt.channelCount()), fmt, preferredFmt);
        fmt = preferredFmt;

        open(QIODevice::ReadWrite);
        write(rsmp.encode());
        close();
    }

    return result;
}


bool qtauAIFFCodec::saveToDevice()
{
    bool result = false;

    AIFFHeader aiffH(size());
    AIFFCommon aiffC(fmt, size());
    AIFFData   aiffD(size());

    if (!dev->isWritable())
        dev->open(QIODevice::WriteOnly);

    if (dev->isWritable())
    {
        if (!dev->isSequential())
            dev->reset();

        QDataStream writer(dev);

        aiffH.write(writer);
        aiffC.write(writer);
        aiffD.write(writer);

        QAudioFormat aiffSaveFormat; // always saving aiff as S16 BE whatever buffer may hold
        aiffSaveFormat.setByteOrder(QAudioFormat::BigEndian);
        aiffSaveFormat.setChannelCount(fmt.channelCount());
        aiffSaveFormat.setSampleRate(fmt.sampleRate());
        aiffSaveFormat.setSampleSize(16);
        aiffSaveFormat.setSampleType(QAudioFormat::SignedInt);

        qtauResampler rsmp(buffer(), fmt, aiffSaveFormat);
        dev->write(rsmp.encode()); // don't close device because who knows what is it - could end badly if it was a socket

        result = true;
    }
    else vsLog::e("AIFF codec could not open iodevice for writing, saving cancelled.");

    return result;
}


bool qtauAIFFCodec::findCommonChunk(QDataStream &reader)
{
    bool result = false;

    // search for a format chunk
    while (true)
    {
        AIFFCommon ac(reader);

        if (ac.isCorrect())
        {
            fmt.setChannelCount(ac.numChannels);
            fmt.setSampleSize  (ac.sampleSize);
            fmt.setSampleRate  (ac.sampleRate);

            result = true;
            break;
        }
        else // that's not the chunk we're looking for, need to skip it
        {
            if (ac.fmtSize == 0 || ac.fmtSize != reader.skipRawData(ac.fmtSize))
            {
                vsLog::e("AIFF codec could not skip a wrong chunk in findCommonChunk(). AIFF reading failed then.");
                break;
            }
        }
    }

    return result;
}


bool qtauAIFFCodec::findSoundChunk(QDataStream &reader)
{
    bool result = false;

    // search for a data chunk
    while (true)
    {
        AIFFData ad(reader);

        if (ad.isCorrect())
        {
            _data_chunk_location = dev->pos();
            _data_chunk_length   = (ad.chunkSize - 8) / (fmt.sampleSize() / 8 * fmt.channelCount());

            result = true;
            break;
        }
        else // that's not the chunk we're looking for, need to skip it
        {
            if (ad.chunkSize == 0 || ad.chunkSize != reader.skipRawData(ad.chunkSize))
            {
                vsLog::e("AIFF codec could not skip a wrong chunk in findSoundChunk(). AIFF reading failed then.");
                break;
            }
        }
    }

    return result;
}
