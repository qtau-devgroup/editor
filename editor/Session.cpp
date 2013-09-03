#include "Session.h"
#include "Utils.h"
#include "audio/Source.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringList>

#include <QtAlgorithms>


qtauSession::qtauSession(QObject *parent) :
    qtauEventManager(parent), docName(tr("Untitled")), isModified(false), hadSavePoint(false),
    playSt(qtauSessionPlayback::NothingToPlay)
{
    //
}

qtauSession::~qtauSession()
{
    if (vocal.vocalWave)
        delete vocal.vocalWave;

    if (music.musicWave)
        delete music.musicWave;
}

qtauEvent_NoteAddition *util_makeAddNotesEvent(const ust &u)
{
    qtauEvent_NoteAddition::noteAddVector changeset;

    for (int i = 0; i < u.notes.size(); ++i)
    {
        const ust_note &n = u.notes[i];
        qtauEvent_NoteAddition::noteAddData d;
        d.id     = i+1;
        d.lyrics = n.lyric;

        d.pulseLength = n.pulseLength;
        d.pulseOffset = n.pulseOffset;
        d.keyNumber   = n.keyNumber;

        changeset.append(d);
    }

    return new qtauEvent_NoteAddition(changeset);
}

//-------------------------------------------

bool qtauSession::loadUST(QString fileName)
{
    bool result = false;
    QFile ustFile(fileName);

    if (ustFile.open(QFile::ReadOnly))
    {
        QStringList ustStrings;
        QTextStream reader(&ustFile);

        while (!reader.atEnd())
            ustStrings << reader.readLine();

        if (!ustStrings.isEmpty())
        {
            ust tmp_u = ustFromStrings(ustStrings);

            if (tmp_u.notes.size() > 0)
            {
                vsLog::s("Successfully loaded " + fileName);

                clearHistory(); // or make a delete event + settings change event + filepath change event
                data = tmp_u;
                docName  = QFileInfo(fileName).baseName();
                filePath = fileName;
                qtauEvent_NoteAddition *loadNotesChangeset = util_makeAddNotesEvent(data);
                applyEvent_NoteAdded(*loadNotesChangeset);

                emit dataReloaded();
                emit onEvent(loadNotesChangeset);

                delete loadNotesChangeset;
                result = true;
            }
            else
                vsLog::e("Could not get any notes from " + fileName);
        }
        else
            vsLog::e("Could not read text lines from " + fileName);

        ustFile.close();
    }
    else
        vsLog::e("Could not open " + fileName);

    return result;
}

QStringList qtauSession::ustStrings(bool) { return ustToStrings(ustRef()); }
QByteArray  qtauSession::ustBinary()      { return ustToBytes  (ustRef()); }

inline bool dataNotesComparison(const ust_note &n1, const ust_note &n2)
{
    return n1.pulseOffset < n2.pulseOffset;
}

const ust&  qtauSession::ustRef()
{
    data.notes.clear();

    foreach (const quint64 &key, noteMap.keys())
        data.notes.append(noteMap[key]);

    qStableSort(data.notes.begin(), data.notes.end(), dataNotesComparison);

    return data;
}

void qtauSession::setDocName(const QString &name)
{
    if (name.isEmpty())
        vsLog::e("Shouldn't set empty doc name for session! Ignoring...");
    else
        docName = name;
}

void qtauSession::setFilePath(const QString &fp)
{
    if (fp.isEmpty())
        vsLog::e("Shouldn't set empty filepath for session! Ignoring...");
    else
    {
        filePath = fp;
        docName = QFileInfo(fp).baseName();
    }
}

//----- inner data functions -----------------------------
void qtauSession::applyEvent_NoteAdded(const qtauEvent_NoteAddition &event)
{
    const qtauEvent_NoteAddition::noteAddVector &changeset = event.getAdded();

    // delete event has reversed transformations
    bool reallyForward = (event.isForward() && !event.isDeleteEvent()) ||
                        (!event.isForward() &&  event.isDeleteEvent());

    if (reallyForward)
    {
        foreach (const qtauEvent_NoteAddition::noteAddData &change, changeset)
            noteMap[change.id] = ust_note(change.id, change.lyrics, change.pulseOffset, change.pulseLength, change.keyNumber);
    }
    else
        foreach (const qtauEvent_NoteAddition::noteAddData &change, changeset)
            noteMap.remove(change.id);
}


void qtauSession::applyEvent_NoteResized(const qtauEvent_NoteResize &event)
{
    const qtauEvent_NoteResize::noteResizeVector &changeset = event.getResized();

    foreach (const qtauEvent_NoteResize::noteResizeData &change, changeset)
    {
        ust_note &n = noteMap[change.id];

        if (event.isForward())
        {
            n.pulseOffset = change.offset;
            n.pulseLength = change.length;
        }
        else
        {
            n.pulseOffset = change.prevOffset;
            n.pulseLength = change.prevLength;
        }
    }
}


void qtauSession::applyEvent_NoteMoved(const qtauEvent_NoteMove &event)
{
    const qtauEvent_NoteMove::noteMoveVector &changeset = event.getMoved();

    foreach (const qtauEvent_NoteMove::noteMoveData &change, changeset)
    {
        ust_note &n = noteMap[change.id];

        if (event.isForward())
        {
            n.pulseOffset += change.pulseOffDelta;
            n.keyNumber   =  change.keyNumber;
        }
        else
        {
            n.pulseOffset -= change.pulseOffDelta;
            n.keyNumber   =  change.prevKeyNumber;
        }
    }
}


void qtauSession::applyEvent_NoteLyrics(const qtauEvent_NoteText &event)
{
    const qtauEvent_NoteText::noteTextVector &changeset = event.getText();

    foreach (const qtauEvent_NoteText::noteTextData &change, changeset)
        if (event.isForward()) noteMap[change.id].lyric = change.txt;
        else                   noteMap[change.id].lyric = change.prevTxt;
}


void qtauSession::applyEvent_NoteEffects(const qtauEvent_NoteEffect &/*event*/)
{
    // TODO: or not to do, that is the question
}

//--------- dispatcher -----------------------------
void qtauSession::onUIEvent(qtauEvent *e)
{
    if (e)
    {
        switch (e->type())
        {
        case ENoteEvents::add:
        {
            qtauEvent_NoteAddition *ne = static_cast<qtauEvent_NoteAddition*>(e);

            if (ne)
            {
                applyEvent_NoteAdded(*ne);
                storeEvent(ne);
            }
            else
                vsLog::e("Session could not convert UI event to noteAdd");

            break;
        }
        case ENoteEvents::move:
        {
            qtauEvent_NoteMove *ne = static_cast<qtauEvent_NoteMove*>(e);

            if (ne)
            {
                applyEvent_NoteMoved(*ne);
                storeEvent(ne);
            }
            else
                vsLog::e("Session could not convert UI event to noteMove");

            break;
        }
        case ENoteEvents::resize:
        {
            qtauEvent_NoteResize *ne = static_cast<qtauEvent_NoteResize*>(e);

            if (e)
            {
                applyEvent_NoteResized(*ne);
                storeEvent(ne);
            }
            else
                vsLog::e("Session could not convert UI event to noteResize");

            break;
        }
        case ENoteEvents::text:
        {
            qtauEvent_NoteText *ne = static_cast<qtauEvent_NoteText*>(e);

            if (ne)
            {
                applyEvent_NoteLyrics(*ne);
                storeEvent(ne);
            }
            else
                vsLog::e("Session could not convert UI event to noteText");

            break;
        }
        case ENoteEvents::effect:
        {
            qtauEvent_NoteEffect *ne = static_cast<qtauEvent_NoteEffect*>(e);

            if (ne)
            {
                applyEvent_NoteEffects(*ne);
                storeEvent(ne);
            }
            else
                vsLog::e("Session could not convert UI event to noteEffect");

            break;
        }
        default:
            vsLog::e(QString("Session received unknown event type from UI").arg(e->type()));
        }

        delete e; // NOTE: it is copied on storing here, so someone somewhere should delete it
    }
    else
        vsLog::e("Session receved a null event from UI");
}

void qtauSession::stackChanged()
{
    if (canUndo())
        isModified = !events.top()->isSavePoint();
    else
        isModified = hadSavePoint;

    emit undoStatus(canUndo());
    emit redoStatus(canRedo());
    emit modifiedStatus(isModified);
}

void qtauSession::setSynthesizedVocal(qtauAudioSource &s)
{
    if (playSt == qtauSessionPlayback::NothingToPlay)
        playSt = qtauSessionPlayback::Stopped;
    else if (playSt != qtauSessionPlayback::Stopped)
        emit requestStopPlayback();

    if (vocal.vocalWave)
        delete vocal.vocalWave;

    vocal.vocalWave = &s;
    emit vocalSet();
}

void qtauSession::setBackgroundAudio(qtauAudioSource &s)
{
    if (playSt == qtauSessionPlayback::NothingToPlay)
        playSt = qtauSessionPlayback::Stopped;
    else if (playSt != qtauSessionPlayback::Stopped)
        emit requestStopPlayback();

    if (music.musicWave)
        delete music.musicWave;

    music.musicWave = &s;
    emit musicSet();
}

void qtauSession::setModified(bool m)
{
    if (m != isModified)
    {
        isModified = m;
        emit modifiedStatus(isModified);
    }
}

void qtauSession::setSaved()
{
    if (canUndo())
    {
        foreach (qtauEvent *e, events)
            e->setSavePoint(false);

        if (!futureEvents.isEmpty())
            foreach (qtauEvent *e, futureEvents)
                e->setSavePoint(false);

        hadSavePoint = true;
        events.top()->setSavePoint();
        setModified(false);
    }
    else
        vsLog::e("Saving an empty session?");
}

void qtauSession::startPlayback()
{
    switch (playSt)
    {
    case qtauSessionPlayback::Playing:
    case qtauSessionPlayback::Repeating:
        emit requestPausePlayback();
        break;

    case qtauSessionPlayback::Paused:
    case qtauSessionPlayback::Stopped:
        emit requestStartPlayback();
        break;

    case qtauSessionPlayback::NeedsSynthesis:
        emit requestSynthesis();
        break;

    case qtauSessionPlayback::NothingToPlay:
    default:
        vsLog::e(QString("Session was asked to start playback when nothing to play! %1").arg(playSt));
    }
}

void qtauSession::stopPlayback()
{
    emit requestStopPlayback();
}

void qtauSession::resetPlayback()
{
    emit requestResetPlayback();
}

void qtauSession::repeatPlayback()
{
    emit requestRepeatPlayback();
}

void qtauSession::setPlaybackState(qtauSessionPlayback::EState state)
{
    if (state != playSt) // may be called with same state on reset (playing -> playing)
    {
        playSt = state;
        emit playbackStateChanged(playSt);
    }
}
