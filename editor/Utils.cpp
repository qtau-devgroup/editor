/* Utils.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "Utils.h"
#include <QTime>
#include <QDebug>

#include <qmath.h>


Q_GLOBAL_STATIC(vsLog, vslog_instance)

vsLog* vsLog::instance()
{
    return vslog_instance();
}

void vsLog::r()
{
    vslog_instance()->enableHistory(false);
    auto &hst = vslog_instance()->history;

    for (auto &stored: hst)
    {
        if (stored.second.isEmpty()) vslog_instance()->reemit("", ELog::none);
        else                         vslog_instance()->reemit(stored.second, stored.first);
    }

    hst.clear();
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
        history.append(QPair<ELog, QString>(type, m));
}
