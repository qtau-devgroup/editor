#include "audio/codecs/Wav.h"
#include "Utils.h"
#include <qendian.h>


//----- WAV PCM RIFF header parts -----------------------
// all char[] must be big-endian, while all integers should be unsigned little-endian

typedef struct SWavRiff {
    char    chunkID[4];     // "RIFF" 0x52494646 BE
    quint32 chunkSize;
    char    chunkFormat[4]; // "WAVE" 0x57415645 BE

    void clear() { memset(chunkID, 0, sizeof(SWavRiff)); }

    SWavRiff(const qint64 bufferSize = 0)
    {
        clear();

        if (bufferSize > 0)
        {
            memcpy(chunkID,     "RIFF", 4);
            memcpy(chunkFormat, "WAVE", 4);
            chunkSize = 36 + bufferSize;
        }
    }

    SWavRiff(QIODevice &d)
    {
        if (d.isReadable())
        {
            qint64 readBytes = d.read(chunkID, 12);

            if (readBytes == 12)
                chunkSize = qFromLittleEndian<quint32>((uchar*)&chunkSize);
            else
                clear();
        }
        else vsLog::e("Can't read WAV RIFF header from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
            {
                char chunkSizeBufLE[4];
                qToLittleEndian<quint32>(chunkSize, (uchar*)chunkSizeBufLE);

                d.write(chunkID,        4);
                d.write(chunkSizeBufLE, 4);
                d.write(chunkFormat,    4);
            }
            else vsLog::e("Wav RIFF header is incorrect, writing to device is cancelled.");
        }
        else vsLog::e("Can't write Wav RIFF header to a closed device.");
    }

    bool isCorrect()
    {
        return !memcmp(chunkID, "RIFF", 4) && !memcmp(chunkFormat, "WAVE", 4) && chunkSize > 0;
    }
} wavRIFF;


typedef struct SWavFmt {
    char    fmtChunkID[4];  // "fmt "  0x666d7420 BE
    quint32 fmtSize;
    quint16 audioFormat;
    quint16 numChannels;
    quint32 sampleRate;
    quint32 byteRate;
    quint16 blockAlign;
    quint16 bitsPerSample;

    void clear() { memset(fmtChunkID, 0, sizeof(SWavFmt)); }

    SWavFmt() { clear(); }

    SWavFmt(const QAudioFormat &fmt)
    {
        memcpy(fmtChunkID, "fmt ", 4);

        fmtSize       = 16;
        audioFormat   = 1;
        numChannels   = fmt.channelCount();
        sampleRate    = fmt.sampleRate();
        byteRate      = fmt.bytesPerFrame() * sampleRate;
        blockAlign    = fmt.bytesPerFrame();
        bitsPerSample = fmt.sampleSize();
    }

    SWavFmt(QIODevice &d)
    {
        if (d.isReadable())
        {
            bool readOK = false;

            // all wav chunks have same structure of 8 first bytes: char[4] + uint32 size
            // in case this is a wrong chunk being read, read just 8 first bytes and check them - used in findFormatChunk()
            qint64 readBytes = d.read(fmtChunkID, 8);
            fmtSize = qFromLittleEndian<quint32>((uchar*)&fmtSize);

            if (readBytes == 8 && !memcmp(fmtChunkID, "fmt ", 4) && fmtSize > 0)
            {
                qint64 toRead = sizeof(SWavFmt) - 8;
                readBytes = d.read((char*)&audioFormat, toRead);

                if (readBytes == toRead)
                {
                    fmtSize       = qFromLittleEndian<quint32>((uchar*)&fmtSize);
                    audioFormat   = qFromLittleEndian<quint16>((uchar*)&audioFormat);
                    numChannels   = qFromLittleEndian<quint16>((uchar*)&numChannels);
                    sampleRate    = qFromLittleEndian<quint32>((uchar*)&sampleRate);
                    byteRate      = qFromLittleEndian<quint32>((uchar*)&byteRate);
                    blockAlign    = qFromLittleEndian<quint16>((uchar*)&blockAlign);
                    bitsPerSample = qFromLittleEndian<quint16>((uchar*)&bitsPerSample);

                    if (fmtSize > 16)
                    {
                        QByteArray ba = d.read(fmtSize - 16);
                        readOK = ba.size() == (int)fmtSize - 16;
                    }
                    else
                        readOK = true;
                }
            }

            if (!readOK)
                clear();
        }
        else vsLog::e("Can't read Wav Fmt chunk from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
            {
                char ints[20];
                qToLittleEndian<quint32>(fmtSize,       (uchar*)&ints[0]);
                qToLittleEndian<quint16>(audioFormat,   (uchar*)&ints[4]);
                qToLittleEndian<quint16>(numChannels,   (uchar*)&ints[6]);
                qToLittleEndian<quint32>(sampleRate,    (uchar*)&ints[8]);
                qToLittleEndian<quint32>(byteRate,      (uchar*)&ints[12]);
                qToLittleEndian<quint16>(blockAlign,    (uchar*)&ints[16]);
                qToLittleEndian<quint16>(bitsPerSample, (uchar*)&ints[18]);

                d.write(fmtChunkID, 4);
                d.write(ints, 20);
            }
            else vsLog::e("Wav Fmt chunk is incorrect, writing to device cancelled.");
        }
        else vsLog::e("Can't write Wav Fmt chunk to a closed device.");
    }

    // if those are read correctly, rest should be ok
    bool isCorrect()
    {
        return !memcmp(fmtChunkID, "fmt ", 4) && fmtSize >= 16 && audioFormat == 1; // 1 is raw PCM
    }
} wavFmt;


typedef struct SWavData {
    char    dataID[4];      // "data" 0x64617461 BE
    quint32 dataSize;

    void clear() { memset(dataID, 0, 8); }

    SWavData(const qint64 bufferSize = 0)
    {
        clear();

        if (bufferSize > 0)
        {
            memcpy(dataID, "data", 4);
            dataSize = bufferSize;
        }
    }

    SWavData(QIODevice &d)
    {
        if (d.isReadable())
        {
            qint64 readBytes = d.read(dataID, 8);

            if (readBytes == 8)
                dataSize = qFromLittleEndian<quint32>((uchar*)&dataSize);
            else
                clear();
        }
        else vsLog::e("Can't read WAV data chunk header from a closed device.");
    }

    void saveToDevice(QIODevice &d)
    {
        if (d.isWritable())
        {
            if (isCorrect())
            {
                char dataSizeBufLE[4];
                qToLittleEndian<quint32>(dataSize, (uchar*)dataSizeBufLE);

                d.write(dataID,        4);
                d.write(dataSizeBufLE, 4);
            }
            else vsLog::e("Wav data chunk header is incorrect, writing to device is cancelled.");
        }
        else vsLog::e("Can't write Wav data chunk header to a closed device.");
    }

    bool isCorrect() { return !memcmp(dataID, "data", 4) && dataSize > 0; }
} wavData;

//-------------------------------------------------------


bool qtauWavCodec::cacheAll()
{
    // read all contents of iodevice into buffer, make format from riff header and cut it from pcm data
    bool result = false;

    if (dev->bytesAvailable() > 0)
    {
        // read the RIFF header
        if (!dev->isSequential())
            dev->reset();

        wavRIFF rh(*dev);

        if (rh.isCorrect())
            result = findFormatChunk() && findDataChunk();
        else
            vsLog::e("Wav codec couldn't read RIFF header");
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
        write(dev->readAll());
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

    if (!dev->isOpen())
        dev->open(QIODevice::WriteOnly);

    if (dev->isOpen())
    {
        if (!dev->isSequential())
            dev->reset();

        dev->write((char*)&wavR, sizeof(wavR));
        dev->write((char*)&wavF, sizeof(wavF));
        dev->write((char*)&wavD, sizeof(wavD));

        dev->write(buffer());
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


bool qtauWavCodec::findFormatChunk()
{
    bool result = false;

    if (!dev->isSequential())
        dev->seek(12);

    // search for a format chunk
    while (true)
    {
        wavFmt wf(*dev);

        if (wf.isCorrect())
        {
            fmt.setSampleType ((wf.bitsPerSample == 8) ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);
            fmt.setSampleSize  (wf.bitsPerSample);
            fmt.setChannelCount(wf.numChannels  );
            fmt.setSampleRate  (wf.sampleRate   );

            result = true;
            break;
        }
        else // that's not the chunk we're looking for, need to skip it
        {
            bool skippedChunk = false;

            if (wf.fmtSize > 0)
            {
                if (dev->isSequential())
                {
                    QByteArray ba = dev->read(wf.fmtSize);
                    skippedChunk = ba.size() == (int)wf.fmtSize;
                }
                else
                    skippedChunk = dev->seek(dev->pos() + wf.fmtSize);
            }

            if (!skippedChunk)
            {
                vsLog::e("Wav codec could not skip a wrong chunk in findFormatChunk(). Wav reading failed then.");
                break;
            }
        }
    }

    return result;
}


bool qtauWavCodec::findDataChunk()
{
    bool result = false;

    if (!dev->isSequential())
        dev->seek(12);

    // search for a data chunk
    while (true)
    {
        wavData wd(*dev);

        if (wd.isCorrect())
        {
            _data_chunk_location = dev->pos();
            _data_chunk_length   = wd.dataSize / fmt.sampleSize() / 8;

            result = true;
            break;
        }
        else // that's not the chunk we're looking for, need to skip it
        {
            bool skippedChunk = false;

            if (wd.dataSize > 0)
            {
                if (dev->isSequential())
                {
                    QByteArray ba = dev->read(wd.dataSize);
                    skippedChunk = ba.size() == (int)wd.dataSize;
                }
                else
                    skippedChunk = dev->seek(dev->pos() + wd.dataSize);
            }

            if (!skippedChunk)
            {
                vsLog::e("Wav codec could not skip a wrong chunk in findDataChunk(). Wav reading failed then.");
                break;
            }
        }
    }

    return result;
}
