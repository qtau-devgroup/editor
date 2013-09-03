#include "utauloid/oto.h"
#include <QRegExp>
#include <QDebug>

/*
    pink part to be stretched
    blue part end and beginning

    offset (left blank)
    consonant (fixed part) lengh
    cutoff (right blank)

    pre utterance,overlap TODO
    red line -> previous option, sound start
    green line -> overlap (vowels,n's,m's)
*/

QOtoMap otoFromStrings(QStringList otoStrings)
{
    QOtoMap result;

    QRegExp newlineRgxp   ("[\r\n]+");
    QRegExp delimitersRgxp("[=,]+");

    foreach (QString s, otoStrings)
    {
        Oto o;

        s.remove (newlineRgxp);
        s.replace(delimitersRgxp, " ");

        QStringList splitted = s.split(' ');

        if (splitted.count() >= 8)
        {
            o.fileName      = splitted.at(0);
            o.en            = splitted.at(1);
            o.nonEn         = splitted.at(2);
            o.offset        = splitted.at(3).toFloat();
            o.consonant     = splitted.at(4).toFloat();
            o.cutoff        = splitted.at(5).toFloat();
            o.pre_utterance = splitted.at(6).toFloat();
            o.overlap       = splitted.at(7).toFloat();

            result[QString("%1_%2_%3").arg(o.fileName).arg(o.en).arg(o.nonEn)] = o;
        }
        else
            qDebug() << "Oto parsing: insufficient parsed element number, skipping" << s;
    }

    return result;
}

QStringList otoToStrings(QOtoMap /*om*/)
{
    QStringList result;

    // TODO:

    return result;
}

QByteArray otoToBytes(QOtoMap /*om*/)
{
    QByteArray result;

    // TODO:

    return result;
}
