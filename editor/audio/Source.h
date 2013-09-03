#ifndef QTAU_AUDIO_SOURCE_H
#define QTAU_AUDIO_SOURCE_H

#include <QIODevice>
#include <QBuffer>
#include <QAudioBuffer>

class vsLog;


typedef struct WavegenSetup {
    qint64 lengthMS;
    float  frequencyHz;
    int    sampleRate;
    bool   stereo;

    WavegenSetup(qint64 len, float freq, int sr, bool st = false) :
        lengthMS(len), frequencyHz(freq), sampleRate(sr), stereo(st) {}
} SWavegenSetup;


class qtauAudioSource : public QBuffer
{
    Q_OBJECT

public:
    explicit qtauAudioSource(QObject *parent = 0);
    explicit qtauAudioSource(const QBuffer& b, const QAudioFormat &f, QObject *parent = 0);

    // generates tonal periodic wave
    explicit qtauAudioSource(const SWavegenSetup &s, QObject *parent = 0);
    ~qtauAudioSource();

    QAudioBuffer getAudioBuffer() { return QAudioBuffer(this->buffer(), fmt); }
    QAudioFormat getAudioFormat() { return fmt; }

    // use if rewriting buffer data completely
    void setAudioFormat(const QAudioFormat &f) { fmt = f; }

    // should read all contents of file/socket and decode it to PCM in buf
    virtual bool cacheAll() { return true; }

protected:
    QAudioFormat fmt; // format of that raw PCM data

};


#endif // QTAU_AUDIO_SOURCE_H
