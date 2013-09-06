#include "audio/codecs/AIFF.h"
#include "Utils.h"

qtauAIFFCodec::qtauAIFFCodec(QIODevice &d, QObject *parent) :
    qtauAudioCodec(d, parent)
{
    if (!d.isOpen())
        vsLog::e("AIFF codec got a closed io device!");
}

bool qtauAIFFCodec::cacheAll() { return false; }
bool qtauAIFFCodec::saveToDevice() { return false; }
