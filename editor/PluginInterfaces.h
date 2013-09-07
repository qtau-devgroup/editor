/* PluginInterfaces.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef PLUGININTERFACES_H
#define PLUGININTERFACES_H

#include <QtPlugin>
#include "Utils.h"
#include "utauloid/ust.h"

class qtauAudioSource;

typedef struct SynthConfig {
    vsLog *log;
    // TODO: some other settings

    SynthConfig(vsLog &l) : log(&l) {}
} SSynthConfig;

class ISynth
{
public:
    virtual ~ISynth() {}

    virtual QString name()                     = 0;
    virtual QString description()              = 0;
    virtual QString version()                  = 0;

    virtual void setup(SSynthConfig &cfg)      = 0;
    virtual bool setVoicebank(const QString&)  = 0;

    virtual bool setVocals(const ust&)         = 0;
    virtual bool setVocals(const QStringList&) = 0;

    virtual bool synthesize(qtauAudioSource&)  = 0;
    virtual bool synthesize(const QString&)    = 0;

    virtual bool isVbReady()                   = 0;
    virtual bool isVocalsReady()               = 0;

    // if synth can stream data as it's being created
    virtual bool supportsStreaming()           = 0;
};

Q_DECLARE_INTERFACE(ISynth, "org.qtau.awesomesauce.ISynth")

#endif // PLUGININTERFACES_H
