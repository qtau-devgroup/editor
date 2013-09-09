/* Mixer.cpp from QTau http://github.com/qtau-devgroup/editor by digited and HAL@ShurabaP, BSD license */

#include "audio/Mixer.h"
#include "Utils.h"

#include <qendian.h>

qtauSoundMixer::qtauSoundMixer(QObject *parent) :
    qtauAudioSource(parent), genZeros(true), replacingEffectsSmoothly(false), replacingTracksSmoothly(false)
{
    //
}

qint64 qtauSoundMixer::bytesAvailable() const
{
    qint64 result = 0;

    if (genZeros)
        result = 9000*9000; // a lot
    else if (!atEnd())
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

        result = framesProcessed * 4;

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

        //------- done working with stored audio ------------------

        memcpy(data, buf.data(), result);

        if (result < maxlen && genZeros)
        {
            memset(data + result, 0, maxlen - result);
            result = maxlen;
        }
    }

    return result;
}


//qtauSoundMixer::qtauSoundMixer(const QList<qtauAudioSource *> &sources, QObject *parent) :
//    qtauAudioSource(parent)
//{
//    // TODO : use audio format user gives.
//    fmt.setSampleRate(sources[0]->getAudioFormat().sampleRate());
//    fmt.setChannelCount(2);
//    fmt.setCodec("audio/pcm");
//    fmt.setByteOrder(QAudioFormat::LittleEndian);
//    fmt.setSampleSize(16);
//    fmt.setSampleType(QAudioFormat::SignedInt); // S16 LE

//    double sec = 0.0;
//    foreach(qtauAudioSource *s, sources)
//    {
//        sec = qMax(sec, s->getAudioFormat().framesForBytes(s->size()) / (double)s->getAudioFormat().sampleRate());
//    }
//    int frameCount = 0.5 + sec * fmt.sampleRate();

//    open(QIODevice::WriteOnly);

//    this->buffer().reserve(fmt.bytesForFrames(frameCount));

//    unsigned char S16LE[2];

//    QVector<QVector<float > > buffers;
//    buffers.resize(sources.size());
//    for(int i = 0; i < sources.size(); i++)
//    {
//        buffers[i].resize(frameCount * fmt.channelCount());
//        extract(buffers[i], fmt.sampleRate(), frameCount, fmt.channelCount(), sources[i]);
//    }

//    for(int f = 0; f < frameCount * fmt.channelCount(); f++)
//    {
//        float realValue = 0.0f;
//        foreach(const QVector<float> &buffer, buffers)
//        {
//            realValue += buffer[f];
//        }
//        qint16 value = qMin(1.0f, qMax(-1.0f, realValue)) * 32767;
//        qToLittleEndian<qint16>(value, S16LE);
//        write(reinterpret_cast<char*>(S16LE), 2);
//    }

//    close();
//}

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
