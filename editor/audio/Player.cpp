/* Player.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "audio/Player.h"
#include "audio/Source.h"
#include "audio/Mixer.h"
#include "Utils.h"

#include <QAudioOutput>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QApplication>


qtmmPlayer::qtmmPlayer() :
    audioOutput(nullptr), mixer(nullptr), stopTimer(nullptr), volume(50)
{
    vsLog::d("QtMultimedia :: supported output devices and codecs:");
    QList<QAudioDeviceInfo> advs = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    foreach (QAudioDeviceInfo i, advs)
        vsLog::d(QString("%1 %2").arg(i.deviceName()).arg(i.supportedCodecs().join(' ')));

    open(QIODevice::ReadOnly);
}

qtmmPlayer::~qtmmPlayer()
{
    close();
    delete stopTimer;
    delete audioOutput;
    delete mixer;
}

qint64 qtmmPlayer::size() const
{
    return mixer->bytesAvailable();
}

void qtmmPlayer::threadedInit()
{
    stopTimer = new QTimer();
    stopTimer->setSingleShot(true);
    connect(stopTimer, &QTimer::timeout, this, &qtmmPlayer::stop);

    mixer = new qtauSoundMixer();
    connect(mixer, &qtauSoundMixer::effectEnded,     this, &qtmmPlayer::onEffectEnded);
    connect(mixer, &qtauSoundMixer::trackEnded,      this, &qtmmPlayer::onTrackEnded);
    connect(mixer, &qtauSoundMixer::allEffectsEnded, this, &qtmmPlayer::onAllEffectsEnded);
    connect(mixer, &qtauSoundMixer::allTracksEnded,  this, &qtmmPlayer::onAllTracksEnded);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    QAudioFormat fmt = mixer->getAudioFormat();

    if (info.isFormatSupported(fmt))
    {
        QAudioDeviceInfo di(QAudioDeviceInfo::defaultOutputDevice());
        audioOutput = new QAudioOutput(di, fmt, this);
        audioOutput->setVolume((qreal)volume / 100.f);

        connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), SLOT(onQtmmStateChanged(QAudio::State)));;
        connect(audioOutput, SIGNAL(notify()), SLOT(onTick()));
    }
    else vsLog::e("Default audio format not supported by QtMultimedia backend, cannot play audio.");
}

void qtmmPlayer::addEffect(qtauAudioSource *e, bool replace, bool smoothly, bool copy)
{
    qtauAudioSource *added = e;

    if (copy)
        added = new qtauAudioSource(e->data(), e->getAudioFormat());

    effects.append(added);
    mixer->addEffect(added, replace, smoothly);
}

void qtmmPlayer::addTrack (qtauAudioSource *t, bool replace, bool smoothly, bool copy)
{
    qtauAudioSource *added = t;

    if (copy)
        added = new qtauAudioSource(t->data(), t->getAudioFormat());

    tracks.append(added);
    mixer->addTrack(added, replace, smoothly);
}

qint64 qtmmPlayer::readData(char *data, qint64 maxlen)
{
    qint64 result = mixer->read(data, maxlen);

    if (result < maxlen)
    {
        if (result == 0)
            stopTimer->start(500);

        memset(data + result, 0, maxlen - result); // silence
        result = maxlen; // else it'll complain on "buffer underflow"... and will keep asking for more
    }

    return result;
}

void qtmmPlayer::play()
{
    stopTimer->stop();

    if (audioOutput->state() == QAudio::SuspendedState)
        audioOutput->resume();
    else
        if (audioOutput->state() != QAudio::ActiveState)
        {
            audioOutput->reset();
            audioOutput->start(this);
        }
}

void qtmmPlayer::pause()
{
    stopTimer->stop();
    audioOutput->suspend();
}

void qtmmPlayer::stop()
{
    stopTimer->stop();
    audioOutput->stop();

    if (!mixer->atEnd())
        mixer->clear();
}

void qtmmPlayer::setVolume(int level)
{
    level = qMax(qMin(level, 100), 0);
    volume = level;

    if (audioOutput)
        audioOutput->setVolume((qreal)level / 100.f);
}

void qtmmPlayer::onEffectEnded(qtauAudioSource* e)
{
    int ind = effects.indexOf(e);

    if (ind != -1)
    {
        delete e;
        effects.removeAt(ind);
    }
}

void qtmmPlayer::onTrackEnded(qtauAudioSource* t)
{
    int ind = tracks.indexOf(t);

    if (ind != -1)
    {
        delete t;
        tracks.removeAt(ind);
    }
}

void qtmmPlayer::onAllEffectsEnded()
{
    if (!effects.isEmpty())
    {
        for (auto &e: effects)
            delete e;

        effects.clear();
    }
}

void qtmmPlayer::onAllTracksEnded()
{
    if (!tracks.isEmpty())
    {
        for (auto &t: tracks)
            delete t;

        tracks.clear();
    }

    emit playbackEnded();
}

inline QString audioStatusToString(QAudio::State st)
{
    QString result = QStringLiteral("unknown");

    switch (st)
    {
    case QAudio::ActiveState:    result = QStringLiteral("active");    break;
    case QAudio::IdleState:      result = QStringLiteral("idle");      break;
    case QAudio::StoppedState:   result = QStringLiteral("stopped");   break;
    case QAudio::SuspendedState: result = QStringLiteral("suspended"); break;
    default:
        break;
    }

    return result;
}

void qtmmPlayer::onQtmmStateChanged(QAudio::State /*st*/)
{
//    qDebug() << "audio status:" << audioStatusToString(st);

//    switch (st)   // doesn't really matter now
//    {
//    case QAudio::ActiveState:
//    case QAudio::SuspendedState:
//        break;

//    case QAudio::StoppedState:
//    case QAudio::IdleState:
//        mixer->clear();
//        break;

//    default:
//        vsLog::e(QString("Unknown Qtmm Audio state: %1").arg(st));
//        break;
//    }
}

void qtmmPlayer::onTick() { emit tick(audioOutput->processedUSecs()); }
