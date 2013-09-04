#include <qendian.h>

#include "audio/Mixer.h"
#include "Utils.h"

qtauSoundMixer::qtauSoundMixer(QObject *parent) :
    qtauAudioSource(parent)
{
}

qtauSoundMixer::qtauSoundMixer(const QList<qtauAudioSource *> &sources, QObject *parent) :
    qtauAudioSource(parent)
{
    // TODO : use audio format user gives.
    fmt.setSampleRate(sources[0]->getAudioFormat().sampleRate());
    fmt.setChannelCount(2);
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt); // S16 LE

    double sec = 0.0;
    foreach(qtauAudioSource *s, sources)
    {
        sec = qMax(sec, s->getAudioFormat().framesForBytes(s->size()) / (double)s->getAudioFormat().sampleRate());
    }
    int frameCount = 0.5 + sec * fmt.sampleRate();

    open(QIODevice::WriteOnly);

    this->buffer().reserve(fmt.bytesForFrames(frameCount));

    unsigned char S16LE[2];

    QVector<QVector<float > > buffers;
    buffers.resize(sources.size());
    for(int i = 0; i < sources.size(); i++)
    {
        buffers[i].resize(frameCount * fmt.channelCount());
        extract(buffers[i], fmt.sampleRate(), frameCount, fmt.channelCount(), sources[i]);
    }

    for(int f = 0; f < frameCount * fmt.channelCount(); f++)
    {
        float realValue = 0.0f;
        foreach(const QVector<float> &buffer, buffers)
        {
            realValue += buffer[f];
        }
        qint16 value = qMin(1.0f, qMax(-1.0f, realValue)) * 32767;
        qToLittleEndian<qint16>(value, S16LE);
        write(reinterpret_cast<char*>(S16LE), 2);
    }

    close();
}

void qtauSoundMixer::extract(QVector<float> &buffer, int sampleRate, int frameCount, int channelCount, qtauAudioSource *source)
{
    // To simplify at first, support only 16bit, PCM, SignedInt and LittleEndian waveform of the same sample rate.
    if(source->getAudioFormat().sampleRate() != sampleRate
            || source->getAudioFormat().sampleSize() != 16
            || source->getAudioFormat().sampleType() != QAudioFormat::SignedInt)
    {
        vsLog::e("Mixer class does not support given audio format.");
        return;
    }

    int sourceChannelCount = source->getAudioFormat().channelCount();
    int sourceFrameCount = source->getAudioFormat().framesForBytes(source->size());
    int bufferIndex = 0;
    for(int c = 0; c < channelCount; c++)
    {
        int channelOffset = qMin(c, sourceChannelCount);
        for(int f = 0; f < sourceFrameCount; f++)
        {
            bufferIndex = f * channelCount + c;
            int sourceIndex = (f * sourceChannelCount + channelOffset) * 2;
            qint16 val = qFromLittleEndian<qint16>(reinterpret_cast<const uchar *>(source->buffer().data() + sourceIndex));
            buffer[bufferIndex] = val / 32768.0f;
        }
    }
    for(; bufferIndex < frameCount; bufferIndex++)
    {
        buffer[bufferIndex] = 0.0f;
    }
}
