/* Resampler.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <QObject>
#include <QAudioFormat>


class qtauResampler : public QObject
{
    Q_OBJECT

public:
    explicit qtauResampler(const QByteArray &srcData, const QAudioFormat &srcFmt, const QAudioFormat &dstFmt, QObject *parent = 0);

    QByteArray encode();
    
protected:
    enum class EResampFormat : char {
        none,
        U8toS16, // to S16
        F32toS16,
        U8toF32, // to F32
        S16toF32,
        S16toU8, // to U8
        F32toU8
    };

    enum class EResampBSwap : char {
        none,
        Swap8,
        Swap16,
        Swap32
    };

    EResampFormat sampleChange;
    EResampBSwap  byteorderChange;
    QByteArray    srcD;
    
};

#endif // RESAMPLER_H
