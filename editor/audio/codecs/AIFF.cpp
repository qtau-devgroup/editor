/* AIFF.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "audio/codecs/AIFF.h"
#include "Utils.h"
#include "audio/Resampler.h"

#include <qendian.h>
#include <qmath.h>


// 80 bit float to uint32
inline quint32 readLD_be(uchar* b)
{
    quint16 exp = qFromBigEndian<quint16>(b);
    quint64 mnt = qFromBigEndian<quint64>(b + 2);
    return ldexp(mnt, exp - 16383 - 63);
}

inline void writeLD_be(const quint32 &val, uchar* b)
{
    union di64 {
        double  d;
        quint64 i;
    };

    di64 di;
    di.d = val;
    quint16 exp = (di.i >> 52) + (16383 - 1023);
    quint64 one = 1;
    quint64 mnt = one << 63 | val << 11;
    qToBigEndian<quint16>(exp, b);
    qToBigEndian<quint64>(mnt, b + 2);
}


typedef struct SAIFFHeader {
    char   chunkID[4];  // "FORM"
    qint32 chunkSize;
    char   formType[4]; // "AIFF" // yes, form is aiff, and aiff header is form. Go ask Apple.

    void clear() { memset(chunkID, 0, sizeof(SAIFFHeader)); }

    SAIFFHeader(const qint64 bufferSize = 0)
    {
        clear();

        if (bufferSize > 0)
        {
            memcpy(chunkID,  "FORM", 4);
            memcpy(formType, "AIFF", 4);
            chunkSize = 8 + bufferSize;
        }
    }

    SAIFFHeader(QIODevice &d)
    {
        if (d.isReadable())
        {
            if (d.read(chunkID, 12) == 12)
                chunkSize = qFromBigEndian<qint32>((uchar*)&chunkSize);
            else
                memset(chunkID, 0, 4);
        }
        else vsLog::e("Can't read AIFF header from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
            {
                char chSbuf[4];
                qToBigEndian<qint32>(chunkSize, (uchar*)chSbuf);

                d.write(chunkID,  4);
                d.write(chSbuf,   4);
                d.write(formType, 12);
            }
            else
                vsLog::e("AIFF header is incorrect, writing to device is cancelled.");
        }
        else vsLog::e("Can't write AIFF header to a closed device.");
    }

    bool isCorrect()
    {
        return !memcmp(chunkID, "FORM", 4) && !memcmp(formType, "AIFF", 4) && chunkSize > 0;
    }
} AIFFHeader;


typedef struct SAIFFCommon {
    char    fmtChunkID[4];  // "COMM"
    qint32  fmtSize;
    char    fmtFields[18];  // gcc doesn't like mix of variables of different size and aligns them, making gaps

    qint16      numChannels;        // 2
    quint32     numSampleFrames;    // 4
    qint16      sampleSize;         // 2
    long double sampleRate;         // 10

    void clear() { memset(fmtChunkID, 0, 26); }

    SAIFFCommon() { clear(); }

    SAIFFCommon(const QAudioFormat &fmt, qint64 size)
    {
        memcpy(fmtChunkID, "COMM", 4);

        fmtSize         = 18; // by spec
        numChannels     = fmt.channelCount();
        numSampleFrames = size / (fmt.sampleSize() / 8 * fmt.channelCount());
        sampleSize      = fmt.sampleSize();
        sampleRate      = fmt.sampleRate();
    }

    SAIFFCommon(QIODevice &d)
    {
        if (d.isReadable())
        {
            bool readOK = false;

            // all aiff chunks have same structure of 8 first bytes: char[4] + uint32 size
            // in case this is a wrong chunk being read, read just 8 first bytes and check them - used in findFormatChunk()
            qint64 readBytes = d.read(fmtChunkID, 8);
            fmtSize = qFromBigEndian<qint32>((uchar*)&fmtSize);

            if (readBytes == 8 && isCorrect())
                if (d.read(fmtFields, 18) == 18)
                {
                    numChannels     = qFromBigEndian<qint16> ((uchar*)fmtFields);
                    numSampleFrames = qFromBigEndian<quint32>((uchar*)fmtFields + 2);
                    sampleSize      = qFromBigEndian<quint16>((uchar*)fmtFields + 6);
                    sampleRate      = readLD_be              ((uchar*)fmtFields + 8); // from 80bit float to uint32

                    readOK = true;
                }

            if (!readOK)
                memset(fmtChunkID, 0, 4);
        }
        else vsLog::e("Can't read AIFF common chunk from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
            {
                uchar beInts[22];
                memset(beInts, 0, 22);

                qToBigEndian<qint32> (fmtSize,         beInts);
                qToBigEndian<qint16> (numChannels,     beInts + 4);
                qToBigEndian<quint32>(numSampleFrames, beInts + 6);
                qToBigEndian<qint16> (sampleSize,      beInts + 10);
                writeLD_be           (sampleRate,      beInts + 12); // 12 + 10 == 22

                d.write(fmtChunkID, 4);
                d.write((char*)beInts, 22);
            }
            else vsLog::e("AIFF common chunk is incorrect, writing to device cancelled.");
        }
        else vsLog::e("Can't write AIFF common chunk to a closed device.");
    }

    // if those are read correctly, rest should be ok
    bool isCorrect() { return !memcmp(fmtChunkID, "COMM", 4) && fmtSize == 18; }

} AIFFCommon;


typedef struct SAIFFData {
    char    chunkID[4];      // "SSND"
    qint32  chunkSize;
    quint32 offset;
    quint32 blockSize;

    void clear() { memset(chunkID, 0, sizeof(SAIFFData)); }

    SAIFFData(const qint64 bufferSize = 0)
    {
        clear();

        if (bufferSize > 0)
        {
            memcpy(chunkID, "SSND", 4);
            chunkSize = bufferSize + 8;
        }
    }

    SAIFFData(QIODevice &d)
    {
        if (d.isReadable())
        {
            bool readOK = false;
            qint64 readBytes = d.read(chunkID, 8);
            chunkSize = qFromBigEndian<qint32>((uchar*)&chunkSize);

            if (readBytes == 8 && isCorrect())
                if (d.read((char*)&offset, 8) == 8)
                {
                    offset    = qFromBigEndian<quint32>((uchar*)&offset);
                    blockSize = qFromBigEndian<quint32>((uchar*)&blockSize);
                    readOK = true;
                }

            if (!readOK)
                memset(chunkID, 0, 4);
        }
        else vsLog::e("Can't read AIFF data chunk header from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
            {
                uchar beInts[12];
                memset(beInts, 0, 12);
                qToBigEndian<qint32> (chunkSize, beInts);

                d.write(chunkID, 4);
                d.write((char*)beInts, 12);
            }
            else
                vsLog::e("AIFF data chunk header is incorrect, writing to device is cancelled.");
        }
        else vsLog::e("Can't write AIFF data chunk header to a closed device.");
    }

    bool isCorrect() { return !memcmp(chunkID, "SSND", 4) && chunkSize > 8; }

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

        AIFFHeader ah(*dev);

        if (ah.isCorrect())
            result = findCommonChunk() && findSoundChunk();
        else
            vsLog::e("Wav codec couldn't read AIFF header");
    }
    else
        vsLog::e("Audio Wav: empty data");

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

        dev->write((char*)&aiffH, sizeof(aiffH));
        dev->write((char*)&aiffC, sizeof(aiffC));
        dev->write((char*)&aiffD, sizeof(aiffD));

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


bool qtauAIFFCodec::findCommonChunk()
{
    bool result = false;

    if (!dev->isSequential())
        dev->seek(12);

    // search for a format chunk
    while (true)
    {
        AIFFCommon ac(*dev);

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
            bool skippedChunk = false;

            if (ac.fmtSize > 0)
            {
                if (dev->isSequential())
                {
                    QByteArray ba = dev->read(ac.fmtSize);
                    skippedChunk = ba.size() == (int)ac.fmtSize;
                }
                else
                    skippedChunk = dev->seek(dev->pos() + ac.fmtSize);
            }

            if (!skippedChunk)
            {
                vsLog::e("AIFF codec could not skip a wrong chunk in findCommonChunk(). AIFF reading failed then.");
                break;
            }
        }
    }

    return result;
}


bool qtauAIFFCodec::findSoundChunk()
{
    bool result = false;

    if (!dev->isSequential())
        dev->seek(12);

    // search for a data chunk
    while (true)
    {
        AIFFData ad(*dev);

        if (ad.isCorrect())
        {
            _data_chunk_location = dev->pos();
            _data_chunk_length   = (ad.chunkSize - 8) / (fmt.sampleSize() / 8 * fmt.channelCount());

            result = true;
            break;
        }
        else // that's not the chunk we're looking for, need to skip it
        {
            bool skippedChunk = false;

            if (ad.chunkSize > 0)
            {
                if (dev->isSequential())
                {
                    QByteArray ba = dev->read(ad.chunkSize);
                    skippedChunk = ba.size() == (int)ad.chunkSize;
                }
                else
                    skippedChunk = dev->seek(dev->pos() + ad.chunkSize);
            }

            if (!skippedChunk)
            {
                vsLog::e("AIFF codec could not skip a wrong chunk in findSoundChunk(). AIFF reading failed then.");
                break;
            }
        }
    }

    return result;
}
