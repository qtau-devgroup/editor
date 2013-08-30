#include "editor/audio/Player.h"
#include "editor/audio/Source.h"
#include "editor/Utils.h"

#include <QAudioOutput>


qtmmPlayer::qtmmPlayer(QObject *parent) :
    QObject(parent), audioOutput(0), volume(50)
{
    vsLog::d("QtMultimedia :: supported output devices and codecs:");
    QList<QAudioDeviceInfo> advs = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    foreach (QAudioDeviceInfo i, advs)
        vsLog::d(QString("%1 %2").arg(i.deviceName()).arg(i.supportedCodecs().join(' ')));
}

qtmmPlayer::~qtmmPlayer()
{
    //
}


bool qtmmPlayer::play(qtauAudioSource *a)
{
    bool result = false;

    if (a != 0)
    {
        if (audioOutput)
        {
            if (audioOutput->state() != QAudio::IdleState ||
                audioOutput->state() != QAudio::StoppedState)
                stop();

            delete audioOutput;
        }

        if (a->size() > 0)
        {
            QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

            if (info.isFormatSupported(a->getAudioFormat()))
            {
                QAudioDeviceInfo di(QAudioDeviceInfo::defaultOutputDevice());
                audioOutput = new QAudioOutput(di, a->getAudioFormat(), this);
                audioOutput->setVolume((qreal)volume / 100.f);

                connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), SLOT(onQtmmStateChanged(QAudio::State)));;
                connect(audioOutput, SIGNAL(notify()), SLOT(onTick()));

                bool opened = a->isOpen();

                if (!opened)
                    opened = a->open(QIODevice::ReadOnly);

                if (opened)
                {
                    audioOutput->start(a); // audioOutput::start requires a device, opened for reading
                    result = true;
                }
                else
                    vsLog::e("Could not open audio source device at all. How is this even possible?");
            }
            else
                vsLog::e("audio format not supported by backend, cannot play audio.");
        }
        else
            vsLog::e("no audio data to play");
    }
    else
        if (audioOutput)
        {
            if (audioOutput->state() == QAudio::SuspendedState) // continue playing
            {
                audioOutput->resume();
                result = true;
            }
            else if (audioOutput->state() == QAudio::StoppedState)
            {
                audioOutput->start();
                result = true;
            }
        }

    if (!result)
        emit playbackEnded();

    return result;
}

void qtmmPlayer::pause()
{
    if (audioOutput)
        audioOutput->suspend();
}

void qtmmPlayer::stop()
{
    if (audioOutput)
        audioOutput->stop();
}

void qtmmPlayer::setVolume(int level)
{
    level = qMax(qMin(level, 100), 0);
    volume = level;

    if (audioOutput)
        audioOutput->setVolume((qreal)level / 100.f);
}

void qtmmPlayer::onQtmmStateChanged(QAudio::State st)
{
    switch (st)
    {
    case QAudio::ActiveState:
    case QAudio::SuspendedState:
        break;

    case QAudio::StoppedState:
    case QAudio::IdleState:
        emit playbackEnded();
        break;

    default:
        vsLog::e(QString("Unknown Qtmm Audio state: %1").arg(st));
        break;
    }
}

void qtmmPlayer::onTick()
{
    emit tick(audioOutput->elapsedUSecs());
}
