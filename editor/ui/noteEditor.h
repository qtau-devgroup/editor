/* noteEditor.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef NOTEEDITOR_H
#define NOTEEDITOR_H

#include "Utils.h"
#include "ui/Config.h"
#include <QWidget>
#include <QUrl>

class qtauEvent_NoteAddition;
class qtauEvent_NoteMove;
class qtauEvent_NoteResize;
class qtauEvent_NoteText;
class qtauEvent_NoteEffect;

class QPixmap;
class QLineEdit;

class qtauEvent;
class qtauEdController;


class qtauNoteEditor : public QWidget
{
    Q_OBJECT
    friend class qtauEdController;

public:
    qtauNoteEditor(QWidget *parent = 0);
    ~qtauNoteEditor();

    void configure(const SNoteSetup &newSetup);
    void deleteSelected();

    void   setVOffset(int voff);
    void   setHOffset(int hoff);
    QPoint scrollTo  (const QRect &r);

    void setRMBScrollEnabled(bool e) { state.rmbScrollEnabled  = e; }
    void setEditingEnabled  (bool e) { state.editingEnabled    = e; }
    void setGridSnapEnabled (bool e) { state.gridSnapEnabled   = e; }

signals:
    void editorEvent(qtauEvent *e);

    void hscrolled(int delta);
    void vscrolled(int delta);
    void zoomed   (int delta);

    void heightChanged(int newHeight);
    void widthChanged (int newWidth );

    void rmbScrolled(QPoint posDelta, QPoint origOffset);
    void requestsOffset(QPoint offset);

    // what happens in editor and is sent to session
    void noteAdd   (qtauEvent_NoteAddition *event);
    void noteResize(qtauEvent_NoteResize   *event);
    void noteMove  (qtauEvent_NoteMove     *event);
    void noteText  (qtauEvent_NoteText     *event);
    void noteEffect(qtauEvent_NoteEffect   *event);

    void urisDropped(QList<QUrl> uris);

public slots:
    void onEvent(qtauEvent *e);
    void lazyUpdate(); // use this instead of update() - Qt is drawing much faster than display updates

protected:
    void paintEvent           (QPaintEvent  *event);
    void resizeEvent          (QResizeEvent *event);

    void mouseDoubleClickEvent(QMouseEvent  *event);
    void mouseMoveEvent       (QMouseEvent  *event);
    void mousePressEvent      (QMouseEvent  *event);
    void mouseReleaseEvent    (QMouseEvent  *event);
    void wheelEvent           (QWheelEvent  *event);

    SNoteSetup       setup;
    qne::editorState state;
    qne::editorNotes notes;

    QPixmap *labelCache;
    QPixmap *bgCache;

    void recalcNoteRects();
    void updateBGCache();

    int  lastUpdate;
    bool delayingUpdate;
    bool updateCalled;

    qtauEdController *ctrl;
    qtauEdController *lastCtrl; // they keep getting input after changing somehow :/

    void changeController(qtauEdController *c);
    void rmbScrollHappened(const QPoint &delta, const QPoint &origOff);
    void eventHappened(qtauEvent *e);

};


#endif // NOTEEDITOR_H
