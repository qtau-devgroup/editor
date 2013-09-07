/* ust.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef UST_H
#define UST_H

#include <QVector>
#include <QString>
#include <QPoint>
#include <QStringList>

typedef struct _ust_note
{
    quint64 id;
    QString lyric;
    int pulseOffset;
    int pulseLength;
    int keyNumber;
    int velocity;

    QStringList userData; // unsupported info

    _ust_note() { clear(); }

    _ust_note(quint64 i, const QString &txt, int pOff, int pLen, int kNum) :
        id(i), lyric(txt), pulseOffset(pOff), pulseLength(pLen), keyNumber(kNum) {}

    _ust_note(const _ust_note &other)
    {
        id          = other.id;
        lyric       = other.lyric;
        pulseOffset = other.pulseOffset;
        pulseLength = other.pulseLength;
        keyNumber   = other.keyNumber;
        velocity    = other.velocity;
        userData    = other.userData;
    }

    void clear()
    {
        id          = 0;
        pulseOffset = 0;
        pulseLength = 0;
        keyNumber   = 0;
        velocity    = 0;

        lyric.clear();
        userData.clear();
    }
} ust_note;


typedef struct
{
    int tempo;
    float gfactor;
    QVector<ust_note> notes;

    QStringList userData;

    void removeID(quint64 id)
    {
        for (int i = 0; i < notes.size(); ++i)
            if (notes.at(i).id == id)
            {
                notes.remove(i);
                break;
            }
    }

    void clear()
    {
        tempo   = -1;
        gfactor = -1;
        notes.resize(0);
        userData.clear();
    }
} ust;


ust ustFromStrings(const QStringList &sl);

QStringList ustToStrings(const ust &u);
QByteArray  ustToBytes(const ust &u);

#endif // UST_H
