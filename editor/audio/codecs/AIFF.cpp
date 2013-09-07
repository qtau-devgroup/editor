/* AIFF.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "audio/codecs/AIFF.h"
#include "Utils.h"
#include <qendian.h>


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
            if (d.read(chunkID, 12) != 12)
                clear();
        }
        else vsLog::e("Can't read AIFF header from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
                d.write(chunkID, 12);
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
    qint16  numChannels;
    quint32 numSampleFrames;
    qint16  sampleSize;
    quint32 sampleRate;
    char    _pad[6]; // common chunk size should be 18 bytes

    void clear() { memset(fmtChunkID, 0, sizeof(SAIFFCommon)); }

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

            if (readBytes == 8 && !memcmp(fmtChunkID, "COMM", 4) && fmtSize > 0)
                readOK = d.read((char*)&numChannels, 18) == 18;

            if (readOK)
                sampleRate = readLD_be((char*)&sampleRate); // from 80bit float to uint32
            else
                clear();
        }
        else vsLog::e("Can't read AIFF common chunk from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
            {
                d.write(fmtChunkID, 16);
                char extendedPlusPad[10];
                memset(extendedPlusPad, 0, 10);

                // TODO: encode uint32 to 80bit float manually
                d.write(extendedPlusPad, 10);
            }
            else
                vsLog::e("AIFF common chunk is incorrect, writing to device cancelled.");
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

            if (d.read(chunkID, 8) == 8)
                readOK = d.read((char*)&offset, 8) == 8;

            if (!readOK)
                clear();
        }
        else vsLog::e("Can't read AIFF data chunk header from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
                d.write(chunkID, sizeof(SAIFFData));
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
    if (!d.isReadable())
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
        fmt.setByteOrder(QAudioFormat::LittleEndian);

        open(QIODevice::WriteOnly);
        write(dev->read(_data_chunk_length * fmt.sampleSize() * 8));
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

        dev->write(buffer()); // don't close device because who knows what is it - could end badly if it was a socket
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
                vsLog::e("AIFF codec could not skip a wrong chunk in findFormatChunk(). AIFF reading failed then.");
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
            _data_chunk_length   = (ad.chunkSize - 8) / (fmt.sampleSize() / 8);

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
                vsLog::e("AIFF codec could not skip a wrong chunk in findDataChunk(). AIFF reading failed then.");
                break;
            }
        }
    }

    return result;
}
