/* Source.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "audio/Source.h"
#include "Utils.h"
#include <qmath.h>
#include <qendian.h>

const qreal sinTableMult = 10000.0;

// supposed to have sinus values with key in radians from (int)([0..2*PI) * sinTableMult)
qreal *sinTable = 0;

qtauAudioSource::qtauAudioSource(QObject *parent) :
    QBuffer(parent)
{
    //
}

qtauAudioSource::qtauAudioSource(const QByteArray& data, const QAudioFormat &f, QObject *parent) :
    QBuffer(parent), fmt(f)
{
    if (!data.isEmpty())
    {
        open(QIODevice::WriteOnly);
        write(data);
        close();
    }
    else vsLog::d("Copying audio source with an empty buffer - what was the point of copying then?");
}

qtauAudioSource::qtauAudioSource(const SWavegenSetup &s, QObject *parent) :
    QBuffer(parent)
{
    if (!sinTable)
    {
        int numValues = (M_PI * 2 + 1) * sinTableMult; // +1 because phases go higher than 2PI before being reduced
        int tableLenBytes = numValues * sizeof(qreal) + 1;

        sinTable = (qreal*)malloc(tableLenBytes);
        memset(sinTable, 0, tableLenBytes);

        for (int i = 0; i < numValues; ++i)
            sinTable[i] = qSin((qreal)i / sinTableMult); // sin(0..2PI)
    }

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
    const int samplesInOut  = s.sampleRate / 20;

    const int frSt  = silenceFrames;
    const int frEnd = frames - silenceFrames;
    const int frIn  = frSt + samplesInOut;
    const int frOut = frames - frIn;

    const qreal d  = M_PI * 2 / s.sampleRate;
    const qreal maxAmplitude   = 0.85;
    const qreal amplitudeDelta = 1.0 / samplesInOut;

    qreal percent     = 0.0;
    qreal phase       = 0.1;
    qreal colorPhase  = 0.2; // random small shifts to colorize wave, should be >0
    qreal colorPhase2 = 0.0;
    qreal colorPhase3 = 0.25;

    qreal phaseStep       = d * s.frequencyHz;
    qreal colorPhaseStep  = phaseStep / 2; // lower octave harmonic to make tone less hurtful for ears
    qreal colorPhase2Step = phaseStep * 2;
    qreal colorPhase3Step = phaseStep * 3;

    qreal tone1Max = maxAmplitude * 0.7;
    qreal tone2Max = maxAmplitude * 0.15;
    qreal tone3Max = maxAmplitude * 0.08;
    qreal tone4Max = maxAmplitude * 0.07;

    qreal tone1Amp = 0.0;
    qreal tone2Amp = 0.0;
    qreal tone3Amp = 0.0;
    qreal tone4Amp = 0.0;

    QByteArray silenceBA(silenceFrames * frameSize, '\0');
    write(silenceBA);

    // smooth intro
    for (int fr = frSt; fr < frEnd; ++fr)
    {
        if (fr <  frIn || fr >= frOut)
        {
            if (fr < frIn) percent += amplitudeDelta;
            else           percent -= amplitudeDelta;

            tone1Amp = percent * tone1Max;
            tone2Amp = percent * tone2Max;
            tone3Amp = percent * tone3Max;
            tone4Amp = percent * tone4Max;
        }

        const qint16 value = (tone1Amp * sinTable[(int)(phase       * sinTableMult)]  +
                              tone2Amp * sinTable[(int)(colorPhase  * sinTableMult)]  +
                              tone3Amp * sinTable[(int)(colorPhase2 * sinTableMult)]  +
                              tone4Amp * sinTable[(int)(colorPhase3 * sinTableMult)]) * 32767;

        qToLittleEndian<qint16>(value,  S16LE);
        qToLittleEndian<qint16>(value, &S16LE[2]);
        write(reinterpret_cast<char*>(S16LE), frameSize);

        phase       += phaseStep;
        colorPhase  += colorPhaseStep;
        colorPhase2 += colorPhase2Step;
        colorPhase3 += colorPhase3Step;

        while (phase       > M_PI * 2) phase       -= M_PI * 2;
        while (colorPhase  > M_PI * 2) colorPhase  -= M_PI * 2;
        while (colorPhase2 > M_PI * 2) colorPhase2 -= M_PI * 2;
        while (colorPhase3 > M_PI * 2) colorPhase3 -= M_PI * 2;
    }

    write(silenceBA);
    close();
}

qtauAudioSource::~qtauAudioSource()
{
    if (isOpen())
        close();
}
