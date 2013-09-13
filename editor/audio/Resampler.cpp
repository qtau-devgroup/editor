/* Resampler.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "Resampler.h"
#include "Utils.h"

#include <qendian.h>

qtauResampler::qtauResampler(const QByteArray &srcData, const QAudioFormat &srcFmt, const QAudioFormat &dstFmt, QObject *parent) :
    QObject(parent), srcD(srcData)
{
    sampleChange    = EResampFormat::none;
    byteorderChange = EResampBSwap::none;

    bool hasData = !srcD.isEmpty();
    bool differentSamples =
            srcFmt.sampleSize() != dstFmt.sampleSize() ||
            srcFmt.sampleType() != dstFmt.sampleType();

    bool differentByteOrder = srcFmt.byteOrder() != dstFmt.byteOrder();

    if (hasData)
    {
        switch (srcFmt.sampleType())
        {
        case QAudioFormat::UnSignedInt: // U8
            if (differentSamples)
            {
                if      (dstFmt.sampleType() == QAudioFormat::SignedInt)   sampleChange = EResampFormat::U8toS16;
                else if (dstFmt.sampleType() == QAudioFormat::Float)       sampleChange = EResampFormat::U8toF32;
            }

            if (differentByteOrder) byteorderChange = EResampBSwap::Swap8;
            break;
        case QAudioFormat::SignedInt:   // S16
            if (differentSamples)
            {
                if      (dstFmt.sampleType() == QAudioFormat::UnSignedInt) sampleChange = EResampFormat::S16toU8;
                else if (dstFmt.sampleType() == QAudioFormat::Float)       sampleChange = EResampFormat::S16toF32;
            }

            if (differentByteOrder) byteorderChange = EResampBSwap::Swap16;
            break;
        case QAudioFormat::Float:       // F32
            if (differentSamples)
            {
                if      (dstFmt.sampleType() == QAudioFormat::UnSignedInt) sampleChange = EResampFormat::F32toU8;
                else if (dstFmt.sampleType() == QAudioFormat::SignedInt)   sampleChange = EResampFormat::F32toS16;
            }

            if (differentByteOrder) byteorderChange = EResampBSwap::Swap32;
            break;
        default:
            vsLog::e(QString("Resampler got an unknown source audio format %1").arg(srcFmt.sampleType()));
        }

        if ((differentSamples && sampleChange == EResampFormat::none) ||
            (differentByteOrder && byteorderChange == EResampBSwap::none))
            vsLog::e("Resampler encountered an error comparing audio formats! This won't end well.");
    }
}

template<typename T> inline void swapByteorder(const char* in, char* out, int size)
{
    const T* srcT;
    T* dstT;

    for (int i = 0; i < size; ++i)
    {
        srcT = reinterpret_cast<const T*>(in + i);
        dstT = reinterpret_cast<T*>(out + i);
        *dstT = qbswap<T>(*srcT);
    }
}

QByteArray qtauResampler::encode()
{
    QByteArray out;
    const char* inData = srcD.data();
    int bytes = srcD.size();

    int i = 0, o = 0;

    switch (sampleChange) // pure Indian code here, sorry.
    {                     // ps Indian for Endian - sounds interesting, doesn't it?
    case EResampFormat::none:     // pps yes of course I thought about templates and inlines, but it still looks like this.
    {
        if (byteorderChange != EResampBSwap::none)
        {
            out.resize(srcD.size());
            char *outData = out.data();

            switch(byteorderChange)
            {
            case EResampBSwap::Swap8:  swapByteorder<quint8> (inData, outData, bytes); break;
            case EResampBSwap::Swap16: swapByteorder<quint16>(inData, outData, bytes); break;
            case EResampBSwap::Swap32: swapByteorder<quint32>(inData, outData, bytes); break; // treating float as a uint32
            default:
                vsLog::e(QString("Resampler can't encode data with unknown type of byteorder change: %1")
                         .arg((char)byteorderChange));
            }
        }
        else out = srcD;

        break;
    }
    case EResampFormat::U8toS16:
    {
        out.resize(srcD.size() * 2);
        char* outData = out.data(); // can't point to earlier it because resize may relocate it

        int bytes   = srcD.size();
        int smpSize   = 1;
        int dstSSize  = 2;
        quint8 *srcSmp;
        qint16  dstSmp = 0;

        if (byteorderChange == EResampBSwap::none)
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                srcSmp = (quint8*)&inData[i];
                dstSmp = ((float)*srcSmp - 128.0) / 127.0 * 32767.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }
        else
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                srcSmp = (quint8*)&inData[i];
                dstSmp = ((float)qbswap<quint8>(*srcSmp) - 128.0) / 127.0 * 32767.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }

        break;
    }
    case EResampFormat::U8toF32:
    {
        out.resize(srcD.size() * 4);
        char* outData = out.data();

        int smpSize   = 1;
        int dstSSize  = 4;
        quint8* srcSmp;
        float   dstSmp = 0;

        if (byteorderChange == EResampBSwap::none)
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                srcSmp = (quint8*)&inData[i];
                dstSmp = ((float)*srcSmp - 128.0) / 127.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }
        else
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                srcSmp = (quint8*)&inData[i];
                dstSmp = ((float)qbswap<quint8>(*srcSmp) - 128.0) / 127.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }

        break;
    }
    case EResampFormat::S16toU8:
    {
        out.resize(srcD.size() / 2);
        char* outData = out.data();

        int smpSize   = 2;
        int dstSSize  = 1;
        qint16 *srcSmp;
        quint8  dstSmp = 0;

        if (byteorderChange == EResampBSwap::none)
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                srcSmp = (qint16*)&inData[i];
                dstSmp = ((float)*srcSmp / 32767.0) * 127.0 + 128.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }
        else
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                srcSmp = (qint16*)&inData[i];
                dstSmp = ((float)qbswap<qint16>(*srcSmp) / 32767.0) * 127.0 + 128.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }

        break;
    }
    case EResampFormat::S16toF32:
    {
        out.resize(srcD.size() * 2);
        char* outData = out.data();

        int smpSize   = 2;
        int dstSSize  = 4;
        qint16 *srcSmp;
        float   dstSmp = 0;

        if (byteorderChange == EResampBSwap::none)
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                srcSmp = (qint16*)&inData[i];
                dstSmp = (float)*srcSmp / 32767.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }
        else
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                srcSmp = (qint16*)&inData[i];
                dstSmp = (float)qbswap<qint16>(*srcSmp) / 32767.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }

        break;
    }
    case EResampFormat::F32toU8:
    {
        out.resize(srcD.size() / 4);
        char* outData = out.data();

        int smpSize   = 4;
        int dstSSize  = 1;
        quint8 dstSmp = 0;

        union fiU {
            float   *smpF;
            quint32 *smpI;
        };

        fiU src;

        if (byteorderChange == EResampBSwap::none)
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                src.smpF = (float*)&inData[i];
                dstSmp = *src.smpF * 127.0 + 128.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }
        else
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                src.smpF = (float*)&inData[i];
                *src.smpI = qbswap<quint32>(*src.smpI);
                dstSmp = *src.smpF * 127.0 + 128.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }

        break;
    }
    case EResampFormat::F32toS16:
    {
        out.resize(srcD.size() / 2);
        char* outData = out.data();

        int smpSize   = 4;
        int dstSSize  = 2;
        qint16 dstSmp = 0;

        union fiU {
            float   *smpF;
            quint32 *smpI;
        };

        fiU src;

        if (byteorderChange == EResampBSwap::none)
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                src.smpF = (float*)&inData[i];
                dstSmp = *src.smpF * 32767.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }
        else
            for (; i < bytes; i += smpSize, o += dstSSize)
            {
                src.smpF = (float*)&inData[i];
                *src.smpI = qbswap<quint32>(*src.smpI);
                dstSmp = *src.smpF * 32767.0;
                memcpy(outData + o, (char*)&dstSmp, dstSSize);
            }

        break;
    }
    default:
        vsLog::e(QString("Unknown conversion type in resampler: %1").arg((char)sampleChange));
    }

    return out;
}
