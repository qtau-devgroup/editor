#include "audio/codecs/Ogg.h"
#include "Utils.h"

qtauOggCodec::qtauOggCodec(QIODevice &d, QObject *parent) :
    qtauAudioCodec(d, parent)
{
    if (!d.isOpen())
        vsLog::e("Ogg codec got a closed io device!");
}

bool qtauOggCodec::cacheAll()  { return false; }
bool qtauOggCodec::saveToDevice()  { return false; }
