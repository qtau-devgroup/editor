#include "editor/Utils.h"
#include <QTime>
#include <qmath.h>


Q_GLOBAL_STATIC(vsLog, vslog_instance)

vsLog* vsLog::instance()
{
    return vslog_instance();
}

vsLog::~vsLog() {}

void vsLog::i(const QString &msg) { vslog_instance()->addMessage(msg, vsLog::info);   }
void vsLog::d(const QString &msg) { vslog_instance()->addMessage(msg, vsLog::debug);  }
void vsLog::e(const QString &msg) { vslog_instance()->addMessage(msg, vsLog::error);  }
void vsLog::s(const QString &msg) { vslog_instance()->addMessage(msg, vsLog::success);}
void vsLog::n()                   { vslog_instance()->addMessage("",  vsLog::none);   }

void vsLog::r()
{
    vslog_instance()->enableHistory(false);
    QList<QString> &hst = vslog_instance()->history;

    while (!hst.isEmpty())
    {
        QString &stored = hst.first();

        if (stored.isEmpty())
            vslog_instance()->reemit("",  vsLog::none);
        else
            vslog_instance()->reemit(stored.remove(0,1), (vsLog::msgType)stored.left(1).toInt());

        hst.removeFirst();
    }
}

void vsLog::reemit(const QString &msg, vsLog::msgType type) { emit message(msg, type); }

void vsLog::addMessage(QString msg, vsLog::msgType type)
{
    if (type == vsLog::none)
        msg = " ";
    else
    {
        qDebug() << msg;

        msg.prepend("\t");
        QString time = QTime::currentTime().toString();

        if (time.compare(lastTime) != 0)
        {
            msg.prepend(time);
            lastTime = time;
        }
    }

    emit message(msg, type);

    if (saving)
        history.append(QString("%1%2").arg((int)type).arg(msg));
}

int snap(int value, int unit, int baseValue)
{
    int baseOffset = baseValue % unit;
    value -= baseOffset;
    int prev = value / unit;
    float percent = (float)(value % unit) / (float)unit;

    return ((percent >= 0.5) ? (prev+1) * unit : prev * unit) + baseOffset;
}
