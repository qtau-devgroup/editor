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

    // TODO : normalize sample rate here.

    int frameCount = 0;
    foreach(qtauAudioSource *s, sources)
    {
        frameCount = qMax(frameCount, s->getAudioFormat().framesForBytes(s->size()));
    }

    open(QIODevice::WriteOnly);
    foreach(qtauAudioSource *s, sources)
    {
        s->open(QIODevice::ReadOnly);
    }

    this->buffer().reserve(fmt.bytesForFrames(frameCount));

    unsigned char S16LE[2];
    char readBuffer[4];

    for(int f = 0; f < frameCount; f++)
    {
        for(int c = 0; c < fmt.channelCount(); c++)
        {
            float realValue = 0.0f;

            foreach(qtauAudioSource *s, sources)
            {
                if(s->getAudioFormat().channelCount() <= c)
                {
                    continue;
                }
                int bytes = s->read(readBuffer, s->getAudioFormat().sampleSize() / 8);
                if(bytes <= 0)
                {
                    continue;
                }
                realValue += getValueFromBuffer(readBuffer, s->getAudioFormat());
            }

            qint16 value = qMin(1.0f, qMax(-1.0f, realValue)) * 32767;
            qToLittleEndian<qint16>(value, S16LE);
            write(reinterpret_cast<char*>(S16LE), 2);
        }
    }
    foreach(qtauAudioSource *s, sources)
    {
        s->close();
    }
    close();
}

float qtauSoundMixer::getValueFromBuffer(char */*buffer*/, const QAudioFormat &/*fmt*/)
{
    return 0.0f;
}
