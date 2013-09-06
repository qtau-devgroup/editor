#include "audio/codecs/Flac.h"
#include "Utils.h"

qtauFlacCodec::qtauFlacCodec(QIODevice &d, QObject *parent) :
    qtauAudioCodec(d, parent)
{
    if (!d.isOpen())
        vsLog::e("Flac codec got a closed io device!");
}

bool qtauFlacCodec::cacheAll() { return false; }
bool qtauFlacCodec::saveToDevice() { return false; }
