#include "utauloid/ust.h"
#include <QRegExp>
#include <QDebug>

ust ustFromStrings(const QStringList &sl)
{
    ust u;

    if (!sl.isEmpty())
    {
        u.gfactor = 1.f; // default, will be overwritten if found

        ust_note currentNote;
        currentNote.clear();
        int currentNoteNum = -1;

        bool processingSettings = false;
        bool processingNotes = false;

        QRegExp settingHeader("^\\[#SETTING\\]");
        QRegExp endHeader    ("^\\[#TRACKEND\\]");
        QRegExp noteHeader   ("^\\[#\\d{4}\\]");
        QRegExp nonLyric     ("[\\s\\d]+");       // remove space and digit symbols if any

        QStringList splitted;

        foreach (QString s, sl)
        {
            if (s.contains(settingHeader))
            {
                processingNotes = false;
                processingSettings = true;
                continue;
            }

            if (s.contains(noteHeader))
            {
                processingSettings = false;
                processingNotes = true;

                if (currentNoteNum >= 0)
                    u.notes.append(currentNote);

                currentNote.clear();
                currentNoteNum = u.notes.size(); // starts from 0
                continue;
            }

            if (s.contains(endHeader))
            {
                if (currentNoteNum >= 0)
                    u.notes.append(currentNote);

                break;
            }

            if (processingSettings)
            {
                splitted = s.split('=');

                if (splitted.size() >= 2)
                {
                    if (splitted.first().contains("GFactor"))
                        u.gfactor = splitted.at(1).toFloat();
                    else if (splitted.first().contains("Tempo"))
                        u.tempo = splitted.at(1).toInt();
                    else if (!s.isEmpty())
                        u.userData.append(s);

                    //else if (splitted.first().contains("Tracks"))
                        // TODO: parse all tracks
                }
            }

            if (processingNotes)
            {
                splitted = s.split('=');

                if (splitted.size() >= 2)
                {
                    if (splitted.first().contains("Length"))
                        currentNote.pulseLength = splitted.at(1).toInt();
                    else if (splitted.first().contains("NoteNum"))
                        currentNote.keyNumber = splitted.at(1).toInt();
                    else if (splitted.first().contains("Intensity"))
                        currentNote.velocity = splitted.at(1).toFloat();
                    else if (splitted.first().contains("Lyric"))
                        currentNote.lyric = splitted.at(1);
                    else if (!s.isEmpty())
                        currentNote.userData.append(s);

                    // TODO: UTF decoding? (textstream loading in session)
                }
            }
        }
    }
    else
        qDebug() << "UST: empty string list";

    qint64 offset = 0;
    int nLen = 0;

    // calc horizontal offsets (in pulses)
    for (int i = 0; i < u.notes.size(); ++i)
    {
        ust_note &n = u.notes[i];
        nLen = n.pulseLength;

        if (n.lyric == "R") // "Rest" note - gap
        {
            u.notes.remove(i);
            i--;
        }
        else
            n.pulseOffset = offset;

        offset += nLen;
    }

    return u;
}

QStringList ustToStrings(const ust &u)
{
    QStringList result;

    result << "[#SETTING]";
    result << QString("GFactor=%1").arg(u.gfactor);
    result << QString("Tempo=%1")  .arg(u.tempo);
    result << "Tracks=1";           // TODO: support multiple tracks?

    if (!u.userData.isEmpty())
        foreach (const QString &uds, u.userData)
            result << uds;

    result << "";

    int i = 0;
    int offset = 0;

    foreach (const ust_note &n, u.notes)
    {
        // if there is a gap between ending of previous note and start of this one, insert R (rest) note
        // just because UTAU cannot into gaps between notes
        if (n.pulseOffset > offset)
        {
            result << QString("[#%1]").arg(QString("%1").arg(i), 4, '0');
            result << QString("Length=%1").arg(n.pulseOffset - offset);
            result << QString("NoteNum=38"); // got 38 by rolling 10d6
            result << QString("Lyric=R");
            result << "";

            offset += n.pulseOffset - offset;
            i++;
        }

        result << QString("[#%1]").arg(QString("%1").arg(i), 4, '0');
        result << QString("Length=%1")   .arg(n.pulseLength);
        result << QString("NoteNum=%1")  .arg(n.keyNumber);
        result << QString("Intensity=%1").arg(n.velocity);
        result << QString("Lyric=%1")    .arg(n.lyric);

        if (!n.userData.isEmpty())
            foreach (const QString &uds, n.userData)
                result << uds;

        result << "";

        offset += n.pulseLength;
        i++;
    }

    result << "[#TRACKEND]";

    return result;
}

QByteArray ustToBytes(const ust &u)
{
    QByteArray result;

    foreach (QString s, ustToStrings(u))
        result.append(s.append('\n').toUtf8());

    return result;
}
