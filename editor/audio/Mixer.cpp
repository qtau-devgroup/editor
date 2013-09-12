/* Mixer.cpp from QTau http://github.com/qtau-devgroup/editor by digited and HAL@ShurabaP, BSD license */

#include "audio/Mixer.h"
#include "Utils.h"

#include <qendian.h>

qtauSoundMixer::qtauSoundMixer(QObject *parent) :
    qtauAudioSource(parent)
{
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");
    fmt.setChannelCount(2);
    fmt.setSampleRate(44100);
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt);
}

qtauSoundMixer::qtauSoundMixer(QList<qtauAudioSource*> &tracks, QObject *parent) :
    qtauAudioSource(parent)
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


const qint16 CONST_SHORT_MAX = 32767;

inline qint64 mixU8(const QByteArray &src, QByteArray &dst, bool stereo, qint64 n)
{
    qint64 nBytes = stereo ? (2 * n) : n;
    qint64 max = qMin((qint64)src.size(), nBytes);
    n = stereo ? (max / 2) : max;

    const char *inData  = src.data();
    char *outData = dst.data();
    quint8 *srcS;
    qint16 *dstS;
    qint64 fSinc = 1;
    qint64 dSinc = stereo ? 2 : 4; // if not stereo, skip odd samples in dst

    for (qint64 fS = 0, fD = 0; fS < max; fS += fSinc, fD += dSinc)
    {
        srcS = (quint8*)&inData[fS];
        dstS = (qint16*)&outData[fD];

        *dstS += ((float)*srcS - 128.0) / 127.0 * 32767.0;
        *dstS = qMax(*dstS, CONST_SHORT_MAX);
    }

    return n;
}

inline qint64 mixS16(const QByteArray &src, QByteArray &dst, bool stereo, qint64 n)
{
    qint64 nBytes = stereo ? (4 * n) : (2 * n);
    qint64 max = qMin((qint64)src.size(), nBytes);
    n = stereo ? (max / 4) : (max / 2);

    const char *inData  = src.data();
    char *outData = dst.data();
    qint16 *srcS;
    qint16 *dstS;
    qint64 fSinc = 2;
    qint64 dSinc = stereo ? 2 : 4; // if not stereo, skip odd samples in dst

    for (qint64 fS = 0, fD = 0; fS < max; fS += fSinc, fD += dSinc)
    {
        srcS = (qint16*)&inData[fS];
        dstS = (qint16*)&outData[fD];

        *dstS += *srcS;
        *dstS = qMax(*dstS, CONST_SHORT_MAX);
    }

    return n;
}

inline qint64 mixF32(const QByteArray &src, QByteArray &dst, bool stereo, qint64 n)
{
    qint64 nBytes = stereo ? (8 * n) : (4 * n);
    qint64 max = qMin((qint64)src.size(), nBytes);
    n = stereo ? (max / 8) : (max / 4);

    const char *inData  = src.data();
    char *outData = dst.data();
    float  *srcS;
    qint16 *dstS;
    qint64 fSinc = 4;
    qint64 dSinc = stereo ? 2 : 4; // if not stereo, skip odd samples in dst

    for (qint64 fS = 0, fD = 0; fS < max; fS += fSinc, fD += dSinc)
    {
        srcS = (float*)&inData[fS];
        dstS = (qint16*)&outData[fD];

        *dstS += *srcS * 32767.0;
        *dstS = qMax(*dstS, CONST_SHORT_MAX);
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
        QByteArray &buf = buffer();

        if (buf.size() < maxlen)
            buf.resize(maxlen);

        QList<qtauAudioSource*> endedEffects;
        QList<qtauAudioSource*> endedTracks;

        // cycle all effects and tracks and try to get required amount of frames from them
        foreach (qtauAudioSource *e, effects)
        {
            qint64 effFrames = 0;

            switch (e->getAudioFormat().sampleSize())
            {
            case 8:  effFrames = mixU8 (e->buffer(), buf, e->getAudioFormat().channelCount() > 1, frames); break;
            case 16: effFrames = mixS16(e->buffer(), buf, e->getAudioFormat().channelCount() > 1, frames); break;
            case 32: effFrames = mixF32(e->buffer(), buf, e->getAudioFormat().channelCount() > 1, frames); break;
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

            switch (t->getAudioFormat().sampleSize())
            {
            case 8:  trFrames = mixU8 (t->buffer(), buf, t->getAudioFormat().channelCount() > 1, frames); break;
            case 16: trFrames = mixS16(t->buffer(), buf, t->getAudioFormat().channelCount() > 1, frames); break;
            case 32: trFrames = mixF32(t->buffer(), buf, t->getAudioFormat().channelCount() > 1, frames); break;
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
        memcpy(data, buf.data(), result);
    }

    return result;
}
