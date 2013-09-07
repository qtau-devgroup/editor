/* noteEditorHandlers.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef NOTEEDITORHANDLERS_H
#define NOTEEDITORHANDLERS_H

#include "Utils.h"
#include "ui/Config.h"
#include <qevent.h>

class qtauEvent;
class qtauNoteEditor;

class qtauEvent_NoteAddition;
class qtauEvent_NoteMove;
class qtauEvent_NoteResize;
class qtauEvent_NoteText;
class qtauEvent_NoteEffect;

class QLineEdit;


/// default controller for note editor
class qtauEdController : public QObject
{
    Q_OBJECT
    friend class qtauNoteEditor;

public:
    qtauEdController(qtauNoteEditor &ne, noteSetup &ns, qne::editorNotes &nts, qne::editorState &st) :
        owner(&ne), setup(&ns), notes(&nts), state(&st), edit(0), pointedNote(0), absFirstClickPos(-1,-1),
        rmbDragging(false), idOffset(0) {}

    qtauEdController(qtauEdController *c) :
        owner(c->owner), setup(c->setup), notes(c->notes), state(c->state), edit(c->edit),
        pointedNote(c->pointedNote), absFirstClickPos(c->absFirstClickPos), rmbDragging(false),
        idOffset(c->idOffset) {}

    virtual ~qtauEdController() {}

    void onEvent(qtauEvent *e);
    void deleteSelected();

protected:
    qtauNoteEditor   *owner;
    noteSetup        *setup;
    qne::editorNotes *notes;
    qne::editorState *state;

    QLineEdit        *edit; // should be alive after deleting text controller - may receive buffered event
    qne::editorNote  *pointedNote;

    QPoint            absFirstClickPos;

    qne::editorNote  *noteInPoint(const QPoint &p);

    virtual void init()    {}
    virtual void reset()   {} // called when event happens - current editing activity should be halted
    virtual void cleanup() {} // called when changing controllers - should check if stopped its activity

    inline void addToGrid(int noteCoord, quint64 id)
    {
        int bar = noteCoord / setup->barWidth;

        if (bar >= notes->grid.size())
            notes->grid.resize(bar + 10);

        if (notes->grid[bar].indexOf(id) == -1)
            notes->grid[bar].append(id);
    }

    inline void removeFromGrid(int noteCoord, quint64 id)
    {
        int bar = noteCoord / setup->barWidth;

        if (bar < notes->grid.size())
        {
            int i = notes->grid[bar].indexOf(id);

            if (i >= 0)
                notes->grid[bar].remove(i);
        }
    }

    inline void unselectAll()
    {
        foreach (const quint64 &id, notes->selected)
            notes->idMap[id].selected = false;

        notes->selected.clear();
    }

    inline void updateModelData(qne::editorNote &n)
    {
        float pixelToPulses = 480.f / setup->note.width();
        n.pulseLength = n.r.width() * pixelToPulses + 0.001;
        n.pulseOffset = n.r.left()  * pixelToPulses + 0.001;

        int totalKeys = (setup->baseOctave + setup->numOctaves - 1) * 12;
        int screenKeyInd = n.r.y() / setup->note.height();
        n.keyNumber = totalKeys - screenKeyInd;
    }

    // just because friends aren't inherited ---------
    void changeController(qtauEdController *c);
    void eventHappened   (qtauEvent        *e);

    void recalcNoteRects();
    void lazyUpdate();
    //------------------------------------------------

    virtual void mouseDoubleClickEvent(QMouseEvent  *event);
    virtual void mouseMoveEvent       (QMouseEvent  *event);
    virtual void mousePressEvent      (QMouseEvent  *event);
    virtual void mouseReleaseEvent    (QMouseEvent  *event);

    // what happens in session and needs to be reflected in editor
    void onNoteAdd   (qtauEvent_NoteAddition *event);
    void onNoteResize(qtauEvent_NoteResize   *event);
    void onNoteMove  (qtauEvent_NoteMove     *event);
    void onNoteText  (qtauEvent_NoteText     *event);
    void onNoteEffect(qtauEvent_NoteEffect   *event);

    QPoint rmbScrollStartPos;
    QPoint rmbStartVpOffset;
    bool   rmbDragging;

    quint64 idOffset;

};


/// mouse handler for changing phoneme/lyrics in note(s)
class qtauEd_TextInput : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_TextInput(qtauNoteEditor &ne, noteSetup &ns, qne::editorNotes &nts, qne::editorState &st);
    qtauEd_TextInput(qtauEdController *c);
    ~qtauEd_TextInput();

protected:
    void reset();
    void cleanup();
    void init();

    bool managedOnEdited; // if onEdited() is called manually and need a different controller next
    bool editingNote;
    qne::editorNote *editedNote;

protected slots:
    void onEdited();
    void unfocus();
    bool eventFilter(QObject *obj, QEvent *event);

};


/// mouse handler for note selection with rectangle
class qtauEd_SelectRect : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_SelectRect(qtauNoteEditor &ne, noteSetup &ns, qne::editorNotes &nts, qne::editorState &st);
    qtauEd_SelectRect(qtauEdController *c);
    ~qtauEd_SelectRect();

protected:
    void mouseMoveEvent   (QMouseEvent  *event);
    void mouseReleaseEvent(QMouseEvent  *event);

    void mouseDoubleClickEvent(QMouseEvent*) { changeController(new qtauEdController(this)); }
    void mousePressEvent      (QMouseEvent*) { changeController(new qtauEdController(this)); }

    void reset();

};


/// mouse handler for dragging note(s) when pressing lmb
class qtauEd_DragNotes : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_DragNotes(qtauNoteEditor &ne, noteSetup &ns, qne::editorNotes &nts, qne::editorState &st);
    qtauEd_DragNotes(qtauEdController *c);
    ~qtauEd_DragNotes();

protected:
    void mouseMoveEvent   (QMouseEvent  *event);
    void mouseReleaseEvent(QMouseEvent  *event);

    void mouseDoubleClickEvent(QMouseEvent*) { changeController(new qtauEdController(this)); }
    void mousePressEvent      (QMouseEvent*) { changeController(new qtauEdController(this)); }

    void init();
    void reset();
    qne::editorNote *mainMovedNote;

    QVector<QRect> selRects;
    QRect selBounds;

};


/// mouse handler for resizing notes by dragging their left/right borders
class qtauEd_ResizeNote : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_ResizeNote(qtauNoteEditor &ne, noteSetup &ns, qne::editorNotes &nts, qne::editorState &st, bool left);
    qtauEd_ResizeNote(qtauEdController *c, bool left);
    ~qtauEd_ResizeNote();

protected:
    void mouseMoveEvent   (QMouseEvent  *event);
    void mouseReleaseEvent(QMouseEvent  *event);

    void mouseDoubleClickEvent(QMouseEvent*) { changeController(new qtauEdController(this)); }
    void mousePressEvent      (QMouseEvent*) { changeController(new qtauEdController(this)); }

    void init();
    void reset();

    qne::editorNote *editedNote;
    bool  toLeft; // if should resize to left direction (move beginning) or to right (move end)
    int   minNoteWidth;
    QRect originalRect;
};


/// mouse handler for adding notes with mouse drag
class qtauEd_AddNote : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_AddNote(qtauNoteEditor &ne, noteSetup &ns, qne::editorNotes &nts, qne::editorState &st);
    qtauEd_AddNote(qtauEdController *c);
    ~qtauEd_AddNote();

protected:
    void mouseMoveEvent   (QMouseEvent  *event);
    void mouseReleaseEvent(QMouseEvent  *event);

    void mouseDoubleClickEvent(QMouseEvent*) { changeController(new qtauEdController(this)); }
    void mousePressEvent      (QMouseEvent*) { changeController(new qtauEdController(this)); }

    void init();
    void reset();

    qne::editorNote *editedNote;
    int minOffset;

};

#endif // NOTEEDITORHANDLERS_H
