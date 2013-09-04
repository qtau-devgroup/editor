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

    const int frameSize = s.stereo ? 4 : 2;

    open(QIODevice::WriteOnly);
    int frames = s.lengthMS * s.sampleRate / 1000;
    buffer().reserve(frames * frameSize + 1);

    unsigned char S16LE[5];
    memset(S16LE, 0, 5);

    const int silenceFrames = 10;
    int fr                  = silenceFrames;
    const int framesIntro   = s.sampleRate / 20;
    const int framesOutro   = frames - framesIntro - silenceFrames;

    const qreal d  = M_PI * 2 / s.sampleRate;
    const qreal toneAmplitude  = 0.85;
    const qreal ampIncPerFrame = 1.0 / framesIntro;
    qreal introAmplitude = 0;

    qreal phase       = 0.0;
    qreal colorPhase  = 0.0;
    qreal colorPhase2 = 0.0;

    qreal phaseStep       = d * s.frequencyHz;
    qreal colorPhaseStep  = phaseStep * 2;
    qreal colorPhase2Step = phaseStep * 5;

    qreal tone1Amp = toneAmplitude * 0.8;
    qreal tone2Amp = toneAmplitude * 0.15;
    qreal tone3Amp = toneAmplitude * 0.05;

    QByteArray silenceBA(silenceFrames * frameSize, '\0');
    write(silenceBA);

    // smooth intro
    for (; fr < framesIntro; ++fr)
    {
        introAmplitude += ampIncPerFrame;

        const qint16 value = (introAmplitude * tone1Amp * qSin(phase)      +
                              introAmplitude * tone2Amp * qSin(colorPhase) +
                              introAmplitude * tone3Amp * qSin(colorPhase2)) * 32767;

        qToLittleEndian<qint16>(value,  S16LE);
        qToLittleEndian<qint16>(value, &S16LE[2]);
        write(reinterpret_cast<char*>(S16LE), frameSize);

        phase += phaseStep;
        colorPhase += colorPhaseStep;
        colorPhase2 += colorPhase2Step;

        while (phase       > M_PI * 2) phase       -= M_PI * 2;
        while (colorPhase  > M_PI * 2) colorPhase  -= M_PI * 2;
        while (colorPhase2 > M_PI * 2) colorPhase2 -= M_PI * 2;
    }

    // main part
    for (; fr < framesOutro; ++fr)
    {
        const qint16 value = (tone1Amp * qSin(phase)      +
                              tone2Amp * qSin(colorPhase) +
                              tone3Amp * qSin(colorPhase2)) * 32767;

        qToLittleEndian<qint16>(value,  S16LE);
        qToLittleEndian<qint16>(value, &S16LE[2]);
        write(reinterpret_cast<char*>(S16LE), frameSize);

        phase += phaseStep;
        colorPhase += colorPhaseStep;
        colorPhase2 += colorPhase2Step;

        while (phase       > M_PI * 2) phase       -= M_PI * 2;
        while (colorPhase  > M_PI * 2) colorPhase  -= M_PI * 2;
        while (colorPhase2 > M_PI * 2) colorPhase2 -= M_PI * 2;
    }

    // smooth outro
    for (; fr < frames; ++fr)
    {
        introAmplitude -= ampIncPerFrame;

        const qint16 value = (introAmplitude * tone1Amp * qSin(phase)      +
                              introAmplitude * tone2Amp * qSin(colorPhase) +
                              introAmplitude * tone3Amp * qSin(colorPhase2)) * 32767;

        qToLittleEndian<qint16>(value,  S16LE);
        qToLittleEndian<qint16>(value, &S16LE[2]);
        write(reinterpret_cast<char*>(S16LE), frameSize);

        phase += phaseStep;
        colorPhase += colorPhaseStep;
        colorPhase2 += colorPhase2Step;

        while (phase       > M_PI * 2) phase       -= M_PI * 2;
        while (colorPhase  > M_PI * 2) colorPhase  -= M_PI * 2;
        while (colorPhase2 > M_PI * 2) colorPhase2 -= M_PI * 2;
    }

    write(silenceBA);
    close();
}

qtauAudioSource::~qtauAudioSource()
{
    if (isOpen())
        close();
}
