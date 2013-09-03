#include "audio/Source.h"
#include "Utils.h"
#include <qmath.h>
#include <qendian.h>


qtauAudioSource::qtauAudioSource(QObject *parent) :
    QBuffer(parent)
{
}

qtauAudioSource::qtauAudioSource(const QBuffer& b, const QAudioFormat &f, QObject *parent) :
    QBuffer(parent), fmt(f)
{
    if (b.size() > 0)
    {
        open(QIODevice::WriteOnly);
        write(b.data());
        close();
    }
    else
        vsLog::d("Copying audio source with an empty buffer - what was the point of copying then?");
}

qtauAudioSource::qtauAudioSource(const SWavegenSetup &s, QObject *parent) :
    QBuffer(parent)
{
    fmt.setSampleRate(s.sampleRate);
    fmt.setChannelCount(s.stereo ? 2 : 1);
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt); // S16 LE

    open(QIODevice::WriteOnly);
    qint64 frames = s.lengthMS * s.sampleRate / 1000;
    buffer().reserve(frames * (s.stereo ? 4 : 2) + 1);

    unsigned char S16LE[4];

    const qreal toneAmplitude = 0.8;
    const qreal d = M_PI * 2 / s.sampleRate;

    qreal phase = 0.0;
    qreal phaseStep = d * s.frequencyHz;

    if (s.stereo) // moved if() up to optimize cycle a bit
    {
        for (int fr = 0; fr < frames; ++fr)
        {
            const qint16 value = toneAmplitude * qSin(phase) * 32767;
            qToLittleEndian<qint16>(value,  S16LE);
            qToLittleEndian<qint16>(value, &S16LE[2]);
            write(reinterpret_cast<char*>(S16LE), 4);

            phase += phaseStep;

            while (phase > M_PI * 2)
                phase -= M_PI * 2;
        }
    }
    else
        for (int fr = 0; fr < frames; ++fr)
        {
            const qint16 value = toneAmplitude * qSin(phase) * 32767;
            qToLittleEndian<qint16>(value, S16LE);
            write(reinterpret_cast<char*>(S16LE), 2);

            phase += phaseStep;

            while (phase > M_PI * 2)
                phase -= M_PI * 2;
        }

    write("\0\0", 1);
    close();
}

qtauAudioSource::~qtauAudioSource()
{
    if (isOpen())
        close();
}
