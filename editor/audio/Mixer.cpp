/* Mixer.cpp from QTau http://github.com/qtau-devgroup/editor by digited and HAL@ShurabaP, BSD license */

#include "audio/Mixer.h"
#include "Utils.h"
#include <math.h>
#include <qendian.h>
#include <QDebug>

#ifndef M_SQRT1_2
    #define M_SQRT1_2	0.70710678118654752440
#endif

qtauSoundMixer::qtauSoundMixer(QObject *parent) :
    qtauAudioSource(parent), replacingEffectsSmoothly(false), replacingTracksSmoothly(false)
{
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");
    fmt.setChannelCount(2);
    fmt.setSampleRate(44100);
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt);

    open(QIODevice::ReadOnly);
}

qtauSoundMixer::qtauSoundMixer(QList<qtauAudioSource*> &tracks, QObject *parent) :
    qtauAudioSource(parent), replacingEffectsSmoothly(false), replacingTracksSmoothly(false)
{
    foreach (qtauAudioSource *a, tracks)
        addTrack(a);
}

qint64 qtauSoundMixer::bytesAvailable() const
{
    qint64 result = 0;

    if (!atEnd())
    {
        foreach (qtauAudioSource *s, effects) result = qMax(s->bytesAvailable(), result);
        foreach (qtauAudioSource *s, tracks)  result = qMax(s->bytesAvailable(), result);
    }

    return result;
}

void qtauSoundMixer::addTrack(qtauAudioSource *t, bool replace, bool smoothly)
{
    if (t && t->size())
    {
        if (!t->isReadable())
            t->open(QIODevice::ReadOnly);

        if (t->isReadable())
        {
            if (replace)
            {
                if (smoothly) replacingTracksSmoothly = true;
                else          tracks.clear();
            }

            tracks.append(t);
        }
        else vsLog::e("Sound mixer could not open a track for reading, adding cancelled.");
    }
    else vsLog::e("Sound mixer can't add an empty track!");
}

void qtauSoundMixer::addEffect(qtauAudioSource *e, bool replace, bool smoothly)
{
    if (e && e->size())
    {
        if (!e->isReadable())
            e->open(QIODevice::ReadOnly);

        if (e->isReadable())
        {
            if (replace)
            {
                if (smoothly) replacingEffectsSmoothly = true;
                else          effects.clear();
            }

            effects.append(e);
        }
        else vsLog::e("Sound mixer could not open an effect for reading, adding cancelled.");
    }
    else vsLog::e("Sound mixer can't add an empty effect!");
}

template<typename T> int sampleTS16(const T &s) { return s; }

template<typename T = quint8> int sampleToS16(const quint8 &s) { return ((float)s - 128.0) / 127.0 * 32767.0;  }
template<typename T = qint16> int sampleToS16(const qint16 &s) { return s;                                     }
template<typename T = qint32> int sampleToS16(const qint32 &s) { float *fS = (float*)&s; return *fS * 32767.0; }

template<class T> inline qint64 mixSamplesT(const QByteArray &src, QByteArray &dst, bool stereo, qint64 n)
{
    const int sT = sizeof(T);
    const int sT2 = sT * 2;

    qint64 nBytes = stereo ? (sT2 * n) : (sT * n);
    qint64 max = qMin((qint64)src.size(), nBytes);
    n = stereo ? (max / sT2) : (max / sT);

    const char *inData  = src.data();
    char       *outData = dst.data();
    T       *srcS;
    qint16 *dstS;
    int iS, iD;

    if (stereo)
    {
        for (qint64 bS = 0, bD = 0; bS < max; bS += sT2, bD += 4)
        {
            // left channel
            srcS = (T*)     (inData  + bS);
            dstS = (qint16*)(outData + bD);
            iS = sampleToS16(*srcS);
            iD = *dstS;

            iD = M_SQRT1_2 * (iS + iD);
            *dstS = qMax(qMin(iD, SHRT_MAX), SHRT_MIN);

            // right channel - a bit redundant, but twice less cycles easily
            srcS = (T*)     (inData  + bS + 2);
            dstS = (qint16*)(outData + bD + 2);
            iS = sampleToS16<T>(*srcS);
            iD = *dstS;

            iD = M_SQRT1_2 * (iS + iD);
            *dstS = qMax(qMin(iD, SHRT_MAX), SHRT_MIN);
        }
    }
    else
    {
        for (qint64 bS = 0, bD = 0; bS < max; bS += sT, bD += 4)
        {
            // left channel
            srcS = (T*)     (inData  + bS);
            dstS = (qint16*)(outData + bD);
            iS = sampleToS16<T>(*srcS);
            iD = *dstS;

            iD = M_SQRT1_2 * (iS + iD);
            *dstS = qMax(qMin(iD, SHRT_MAX), SHRT_MIN);

            // right channel - dest val may be different
            dstS = (qint16*)(outData + bD + 2);
            iD = *dstS;

            iD = M_SQRT1_2 * (iS + iD);
            *dstS = qMax(qMin(iD, SHRT_MAX), SHRT_MIN);
        }
    }

    return n;
}


qint64 qtauSoundMixer::readData(char *data, qint64 maxlen)
{
    qint64 result = 0;
    qint64 frames = maxlen / 4;
    qint64 truncated = maxlen - frames*4;
    qint64 framesProcessed = 0;

    if (truncated > 0)
        vsLog::d(QString("Sound mixer was asked to give %1 bytes, %2 more than equeal to frame size!")
                         .arg(maxlen).arg(truncated));

    maxlen -= truncated;

    if (maxlen > 0) // mix samples of all tracks and effects
    {
        /*
         * all audios are considered to be open for reading, U8 or S16 or F32 LE, mono or stereo, 44100Hz
         * output from readData is considered to be 16LE stereo, 4 bytes per frame (2*2)
         * need to read same amount of frames from all tracks and sources, and if any one is giving less, it's ended
         * signal ended audios so that they may be released
         * */

        if (buffer().size() < maxlen)
            buffer().resize(maxlen);

        QByteArray &buf = buffer();
        memset(buf.data(), 0, maxlen);

        QList<qtauAudioSource*> endedEffects;
        QList<qtauAudioSource*> endedTracks;

        // cycle all effects and tracks and try to get required amount of frames from them
        foreach (qtauAudioSource *e, effects)
        {
            qint64 effFrames = 0;
            qint64 numCh = (e->getAudioFormat().channelCount() > 1) ? 2 : 1;

            switch (e->getAudioFormat().sampleSize())
            {
            case 8:  effFrames = mixSamplesT<quint8>(e->read(frames     * numCh), buf, numCh > 1, frames); break;
            case 16: effFrames = mixSamplesT<qint16>(e->read(frames * 2 * numCh), buf, numCh > 1, frames); break;
            case 32: effFrames = mixSamplesT<qint32>(e->read(frames * 4 * numCh), buf, numCh > 1, frames); break;
            default:
                vsLog::e("Sound mixer is processing an effect with unsupported sample size, dropping.");
                endedEffects.append(e);
            }

            if (effFrames < frames)
                endedEffects.append(e);

            framesProcessed = qMax(effFrames, framesProcessed);
        }

        foreach (qtauAudioSource *t, tracks)
        {
            qint64 trFrames = 0;
            qint64 numCh = (t->getAudioFormat().channelCount() > 1) ? 2 : 1;

            switch (t->getAudioFormat().sampleSize())
            {
            case 8:  trFrames = mixSamplesT<quint8>(t->read(frames     * numCh), buf, numCh > 1, frames); break;
            case 16: trFrames = mixSamplesT<qint16>(t->read(frames * 2 * numCh), buf, numCh > 1, frames); break;
            case 32: trFrames = mixSamplesT<qint32>(t->read(frames * 4 * numCh), buf, numCh > 1, frames); break;
            default:
                vsLog::e("Sound mixer is processing an effect with unsupported sample size, dropping.");
                endedTracks.append(t);
            }

            if (trFrames < frames)
                endedTracks.append(t);

            framesProcessed = qMax(trFrames, framesProcessed);
        }

        //-- cleanup ---------------------------------

        if (!endedEffects.isEmpty())
        {
            foreach (qtauAudioSource *e, endedEffects)
            {
                emit effectEnded(e);
                effects.removeOne(e);
            }

            if (effects.isEmpty())
            {
                emit allEffectsEnded();
                replacingEffectsSmoothly = false;
            }
        }

        if (!endedTracks.isEmpty())
        {
            foreach (qtauAudioSource *t, endedTracks)
            {
                emit trackEnded(t);
                tracks.removeOne(t);
            }

            if (tracks.isEmpty())
            {
                emit allTracksEnded();
                replacingTracksSmoothly = false;
            }
        }

        result = framesProcessed * 4; // 4 bytes per frame (16LE stereo, always)

        if (result > 0)
            memcpy(data, buf.data(), result);
    }

    return result;
}
