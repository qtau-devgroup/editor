/* Utils.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "Utils.h"
#include <QTime>
#include <qmath.h>


Q_GLOBAL_STATIC(vsLog, vslog_instance)

vsLog* vsLog::instance()
{
    return vslog_instance();
}

void vsLog::r()
{
    vslog_instance()->enableHistory(false);
    QList<QString> &hst = vslog_instance()->history;

    while (!hst.isEmpty())
    {
        QString &stored = hst.first();

        if (stored.isEmpty())
            vslog_instance()->reemit("",  ELog::none);
        else
            vslog_instance()->reemit(stored.remove(0,1), (ELog)stored.left(1).toInt());

        hst.removeFirst();
    }
}

void vsLog::reemit(const QString &msg, ELog type) { emit message(msg, type); }

void vsLog::addMessage(const QString &msg, ELog type)
{
    QString m = msg;

    if (type == ELog::none)
        m = " ";
    else
    {
        qDebug() << m;

        m.prepend("\t");
        QString time = QTime::currentTime().toString();

        if (time.compare(lastTime))
        {
            m.prepend(time);
            lastTime = time;
        }
    }

    emit message(m, type);

    if (saving)
        history.append(QString("%1").arg((char)type) + m);
}

int snap(int value, int unit, int baseValue)
{
    int baseOffset = baseValue % unit;
    value -= baseOffset;
    int prev = value / unit;
    float percent = (float)(value % unit) / (float)unit;

    return ((percent >= 0.5) ? (prev+1) * unit : prev * unit) + baseOffset;
}
