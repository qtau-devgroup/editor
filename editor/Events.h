#ifndef EVENTS_H
#define EVENTS_H

#include <QObject>
#include <QStack>


class qtauEvent
{
    friend class qtauEventManager;

public:
    typedef enum {    // linking is sorta shortcut because I don't feel like making composite events
        single = 0,   // separate event
        linkedToNext, // requires applying next after this one
        linkedToPrev, // should be applied right after previous one
        linkedToBoth  // is in the middle of chain of events
    } EEventLink;

    qtauEvent(int type = 0, bool forward = true, EEventLink link = single) :
        _type(type), _forward(forward), _linkType(link), _isSavePoint(false) {}
    virtual ~qtauEvent()   {}

    int type()            const { return _type;     }
    bool isForward()      const { return _forward;  } // is event used to apply or to revert its changeset
    EEventLink isLinked() const { return _linkType; } // should event be applied with other(s)

    bool isSavePoint()                  { return _isSavePoint; }
    void setSavePoint(bool isSP = true) { _isSavePoint = isSP; }

protected:
    int  _type;
    bool _forward;
    EEventLink _linkType;

    bool _isSavePoint;

    virtual qtauEvent *allocCopy() const = 0; // should create exact copy of event object on heap

};


/// logic for any class that wants to use events
class qtauEventManager : public QObject
{
    Q_OBJECT

public:
    explicit qtauEventManager(QObject *parent = 0) : QObject(parent) {}
    virtual ~qtauEventManager() { clearHistory(); }

    virtual void storeEvent(const qtauEvent *e)
    {
        clearFuture();
        events.push(e->allocCopy());
        stackChanged();
    }

    virtual void undo()
    {
        if (canUndo())
        {
            futureEvents.push(events.pop());
            futureEvents.top()->_forward = false;
            stackChanged();
            emit onEvent(futureEvents.top());
        }
    }

    virtual void redo()
    {
        if (canRedo())
        {
            events.push(futureEvents.pop());
            events.top()->_forward = true;
            stackChanged();
            emit onEvent(events.top());
        }
    }

    virtual void clearHistory()   { clearPast(); clearFuture(); stackChanged(); }
    virtual int  historyDepth()   { return events.size();           }
    virtual bool isHistoryEmpty() { return events.isEmpty();        }
    virtual bool canUndo()        { return !events.isEmpty();       }
    virtual bool canRedo()        { return !futureEvents.isEmpty(); }

signals:
    void onEvent(qtauEvent*);

protected:
    QStack<qtauEvent*> events;
    QStack<qtauEvent*> futureEvents; // what was undo'ed

    // can be (should be?) overloaded to send undo/redo availability status to UI
    virtual void stackChanged() {}

    void clearPast()
    {
        while (!events.isEmpty())
            delete events.pop();
    }

    void clearFuture()
    {
        while (!futureEvents.isEmpty())
            delete futureEvents.pop();
    }

};

#endif // EVENTS_H
