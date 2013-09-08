/* Resampler.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <QObject>
#include <QAudioFormat>


class qtauResampler : public QObject
{
    Q_OBJECT

    typedef enum {
        SameSamples = 0,
        U8toS16, // to S16
        F32toS16,
        U8toF32, // to F32
        S16toF32,
        S16toU8, // to U8
        F32toU8
    } resamplingType;

    typedef enum {
        SameByteorder = 0,
        Swap8,
        Swap16,
        Swap32
    } byteorderConversion;

public:
    explicit qtauResampler(const QByteArray &srcData, const QAudioFormat &srcFmt, const QAudioFormat &dstFmt, QObject *parent = 0);

    QByteArray encode();
    
protected:
    resamplingType      sampleChange;
    byteorderConversion byteorderChange;
    QByteArray          srcD;
    
};

#endif // RESAMPLER_H
