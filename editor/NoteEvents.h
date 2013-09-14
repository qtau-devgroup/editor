/* NoteEvents.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef NOTEEVENTS_H
#define NOTEEVENTS_H

#include <QVector>
#include <QPoint>
#include <QSize>
#include "Events.h"


namespace ENoteEvents
{
    enum {
        add = 100,
        resize,
        move,
        text,
        effect
    };
}


// add/remove note(s)
class qtauEvent_NoteAddition : public qtauEvent
{
public:
    typedef struct {
        quint64 id;
        QString lyrics;

        int pulseOffset;
        int pulseLength;
        int keyNumber;

        QString toString() const { return QString("id: %1 offset: %2 length: %3 key number: %4 lyrics: %5")
                    .arg(id).arg(pulseOffset).arg(pulseLength).arg(keyNumber).arg(lyrics); }
    } noteAddData;

    typedef QVector<noteAddData> noteAddVector;

    qtauEvent_NoteAddition(const noteAddVector &changeset, bool forward = true, bool deleteEvent = false) :
        qtauEvent(ENoteEvents::add, forward), added(changeset), deleteInstead(deleteEvent) {}

    const noteAddVector& getAdded() const { return added; }

    bool isDeleteEvent() const { return deleteInstead; }

protected:
    noteAddVector added;
    bool deleteInstead; // if it really is a delete event, so its transformation should be reversed

    qtauEvent_NoteAddition* allocCopy() const override { return new qtauEvent_NoteAddition(*this); }

};


class qtauEvent_NoteResize : public qtauEvent
{
public:
    typedef struct {
        quint64 id;
        int offset; // offset is delta of start of note
        int length;

        int prevOffset;
        int prevLength;

        QString toString() const { return QString("id: %1 offset: %2 offset was: %3 length: %4 length was: %5")
                    .arg(id).arg(offset).arg(prevOffset).arg(length).arg(prevLength); }
    } noteResizeData;

    typedef QVector<noteResizeData> noteResizeVector;

    qtauEvent_NoteResize(const noteResizeVector &changeset, bool forward = true) :
        qtauEvent(ENoteEvents::resize, forward), resized(changeset) {}

    const noteResizeVector& getResized() const { return resized; }

protected:
    noteResizeVector resized;

    qtauEvent_NoteResize* allocCopy() const override { return new qtauEvent_NoteResize(*this); }

};


class qtauEvent_NoteMove : public qtauEvent
{
public:
    typedef struct {
        quint64 id;
        int pulseOffDelta;
        int keyNumber;
        int prevKeyNumber;

        QString toString() const { return QString("id: %1 pulseOffDelta: %2 key number: %3 key number was: %4")
                    .arg(id).arg(pulseOffDelta).arg(keyNumber).arg(prevKeyNumber); }
    } noteMoveData;

    typedef QVector<noteMoveData> noteMoveVector;

    qtauEvent_NoteMove(const noteMoveVector &changeset, bool forward = true) :
        qtauEvent(ENoteEvents::move, forward), moved(changeset) {}

    const noteMoveVector& getMoved() const { return moved; }

protected:
    noteMoveVector moved;

    qtauEvent_NoteMove* allocCopy() const override { return new qtauEvent_NoteMove(*this); }
};


class qtauEvent_NoteText : public qtauEvent
{
public:
    typedef struct {
        quint64 id;
        QString txt;
        QString prevTxt;

        QString toString() const { return QString("id: %1 text: %2 text was: %3").arg(id).arg(txt).arg(prevTxt); }
    } noteTextData;

    typedef QVector<noteTextData> noteTextVector;

    qtauEvent_NoteText(const noteTextVector &changeset, bool forward = true) :
        qtauEvent(ENoteEvents::text, forward), text(changeset) {}

    const noteTextVector& getText() const { return text; }

protected:
    noteTextVector text;

    qtauEvent_NoteText* allocCopy() const override { return new qtauEvent_NoteText(*this); }
};


class qtauEvent_NoteEffect : public qtauEvent
{
public:
    typedef struct {
        quint64 id;
        // old data, new data

        QString toString() const { return QString("id: %1").arg(id); }
    } noteEffectData;

    typedef QVector<noteEffectData> noteEffectVector;

    qtauEvent_NoteEffect(const noteEffectVector &changeset, bool forward = true) :
        qtauEvent(ENoteEvents::effect, forward), effect(changeset) {}

    const noteEffectVector& getEffect() const { return effect; }

protected:
    noteEffectVector effect;

    qtauEvent_NoteEffect* allocCopy() const override { return new qtauEvent_NoteEffect(*this); }

};


#endif // NOTEEVENTS_H
