#ifndef OTO_H
#define OTO_H

#include <QStringList>
#include <QMap>

// UTAU oto.ini phoneme description structure
typedef struct
{
    QString fileName;
    QString en;       // latin phoneme name
    QString nonEn;    // localized phoneme name, Japanese usually

    float   offset;
    float   consonant;
    float   cutoff;
    float   pre_utterance;
    float   overlap;
    float   samples;

    void   *userdata;
} Oto;

typedef QMap<QString, Oto> QOtoMap;

QOtoMap otoFromStrings(QStringList otoStrings);

QStringList otoToStrings(QOtoMap om);
QByteArray  otoToBytes  (QOtoMap om);


#endif // OTO_H
