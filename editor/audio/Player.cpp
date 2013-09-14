/* Player.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "audio/Player.h"
#include "audio/Source.h"
#include "audio/Mixer.h"
#include "Utils.h"

#include <QAudioOutput>
#include <QTimer>
#include <QDebug>


qtmmPlayer::qtmmPlayer()
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
    delete audioOutput;
    delete mixer;
}

void qtmmPlayer::threadedInit()
{
    mixer = new qtauSoundMixer();

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

void qtmmPlayer::addEffect(qtauAudioSource *e, bool replace, bool smoothly)
{
    qtauAudioSource *eCopy = new qtauAudioSource(e->data(), e->getAudioFormat());
    effects.append(eCopy);

    mixer->addEffect(eCopy, replace, smoothly);
}

void qtmmPlayer::addTrack (qtauAudioSource *t, bool replace, bool smoothly)
{
    qtauAudioSource *tCopy = new qtauAudioSource(t->data(), t->getAudioFormat());
    tracks.append(tCopy);

    mixer->addTrack(tCopy, replace, smoothly);
}

qint64 qtmmPlayer::readData(char *data, qint64 maxlen)
{
    mixer->reset();
    qint64 result = mixer->read(data, maxlen);

    if (result < maxlen)
    {
        QTimer::singleShot(1, this, SLOT(stop()));
        memset(data + result, 0, maxlen - result); // silence
        result = maxlen; // else it'll complain on "buffer underflow"... and will keep asking for more
    }

    return result;
}

void qtmmPlayer::play()
{
    if (audioOutput->state() == QAudio::SuspendedState)
        audioOutput->resume();
    else
        audioOutput->start(this);
}

void qtmmPlayer::pause()
{
    audioOutput->suspend();
}

void qtmmPlayer::stop()
{
    audioOutput->stop();
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

void qtmmPlayer::onQtmmStateChanged(QAudio::State /*st*/)
{
//    switch (st)   // doesn't really matter now
//    {
//    case QAudio::ActiveState:
//    case QAudio::SuspendedState:
//        break;

//    case QAudio::StoppedState:
//    case QAudio::IdleState:
//        emit playbackEnded();
//        break;

//    default:
//        vsLog::e(QString("Unknown Qtmm Audio state: %1").arg(st));
//        break;
//    }
}

void qtmmPlayer::onTick() { emit tick(audioOutput->processedUSecs()); }
