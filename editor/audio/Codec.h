/* Codec.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef QTAU_AUDIO_FILE_H
#define QTAU_AUDIO_FILE_H

#include "audio/Source.h"
#include <QMap>

// codec is intermediate between buffered PCM data and some source of encoded audio
class qtauAudioCodec : public qtauAudioSource
{
    Q_OBJECT
    friend class qtauAudioCodecFactory;

protected:
    QIODevice *dev;
    qtauAudioCodec(QIODevice &d, QObject *parent = 0);

};

class qtauAudioCodecFactory
{
public:
    virtual qtauAudioCodec* make(QIODevice &d, QObject *parent = 0) = 0;
    virtual ~qtauAudioCodecFactory() {}

    const QString& ext () const { return _ext;  }
    const QString& mime() const { return _mime; }
    const QString& desc() const { return _desc; }

protected:
    QString _ext;
    QString _mime;
    QString _desc;

};



class qtauCodecRegistry
{
public:
    ~qtauCodecRegistry();

    static qtauCodecRegistry* instance();

    inline bool hasCodecForMime(const QString &mime) const { return (mime.isEmpty()) ? false : codecsByMime.contains(mime); }
    inline bool hasCodecForExt (const QString &ext)  const { return (ext.isEmpty())  ? false : codecsByExt .contains(ext);  }

    // returns new codec if factory is registered for this mime/ext, else returns 0
    qtauAudioCodec* getCodecByMime(const QString &mime, QIODevice &d, QObject *parent = 0);
    qtauAudioCodec* getCodecByExt (const QString &ext,  QIODevice &d, QObject *parent = 0);

    bool addCodec(qtauAudioCodecFactory *factory, bool replace = false);

    QList<QString> listCodecs(); // utility for load/save dialogs, returns strings in format "description (*.ext)"

protected:
    qtauCodecRegistry() {}
    Q_DISABLE_COPY(qtauCodecRegistry)

    QMap<QString, qtauAudioCodecFactory*> codecsByMime; // mime is primary when adding/replacing codecs
    QMap<QString, qtauAudioCodecFactory*> codecsByExt;

};


inline bool isAudioMimeSupported(const QString &mime)
{
    return qtauCodecRegistry::instance()->hasCodecForMime(mime);
}

inline bool isAudioExtSupported(const QString &ext)
{
    return qtauCodecRegistry::instance()->hasCodecForExt(ext);
}

inline qtauAudioCodec* codecForMime(const QString &mime, QIODevice &d, QObject *parent = 0)
{
    return qtauCodecRegistry::instance()->getCodecByMime(mime, d, parent);
}

inline qtauAudioCodec* codecForExt(const QString &ext, QIODevice &d, QObject *parent = 0)
{
    return qtauCodecRegistry::instance()->getCodecByExt(ext, d, parent);
}

#endif // QTAU_AUDIO_FILE_H
