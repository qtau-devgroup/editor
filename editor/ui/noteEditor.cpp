#include "ui/noteEditor.h"
#include "ui/noteEditorHandlers.h"
#include "NoteEvents.h"

#include <QTime>
#include <QTimer>
#include <qpainter.h>
#include <QPainterPath>
#include <QMimeData>

#include <QLineEdit>

const int CONST_CACHE_DEFNUM_LABELS = 1000;
const int CONST_CACHE_LINE_HEIGHT   = 12;
const int CONST_CACHE_LINE_WIDTH    = 100;
const int CONST_LABEL_DRAW_MINWIDTH = 20; // minimal note width on screen in pixels to draw phoneme in it

const int CONST_SCROLL_MARGIN       = 20; // px of space between edge of widget and target rect

const double F_ROUNDER = 0.001f; // because float is floor'ed to int by default, so 3.9999999 becomes 3

//QTime t;


qtauNoteEditor::qtauNoteEditor(QWidget *parent) :
    QWidget(parent), bgCache(0), delayingUpdate(false), updateCalled(false), lastCtrl(0)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    labelCache = new QPixmap(CONST_CACHE_LINE_WIDTH, CONST_CACHE_LINE_HEIGHT * CONST_CACHE_DEFNUM_LABELS);
    labelCache->fill(Qt::transparent);

    ctrl = new qtauEdController(*this, setup, notes, state);

    //t.start();
}

qtauNoteEditor::~qtauNoteEditor()
{
    if (labelCache)
        delete labelCache;

    if (bgCache)
        delete bgCache;

    if (lastCtrl)
        delete lastCtrl;

    if (ctrl)
        delete ctrl;
}

void qtauNoteEditor::configure(const noteSetup &newSetup)
{
    ctrl->reset();
    bool gridChanged = newSetup.note != setup.note || newSetup.notesInBar != setup.notesInBar;
    setup = newSetup;

    if (gridChanged)
    {
        recalcNoteRects();
        updateBGCache();
        lazyUpdate();
    }
}

void qtauNoteEditor::deleteSelected()
{
    if (!notes.selected.isEmpty())
    {
        ctrl->reset();
        ctrl->deleteSelected();
    }
}

void qtauNoteEditor::onEvent(qtauEvent *e)
{
    ctrl->onEvent(e);
}

void qtauNoteEditor::lazyUpdate()
{
//    if (!updateCalled)
//    {
//        int now = t.elapsed();

//        if (now - lastUpdate < 15)
//        {
//            delayingUpdate = true;
//            QTimer::singleShot(10, this, SLOT(lazyUpdate()));
//        }
//        else
//        {
//            delayingUpdate = false;
//            updateCalled = true;
//            update();
//        }
//    }
    /* delayed update desynchronizes drawing of piano/meter and note editor...
        Either to delay drawing of all custom widgets, or just stop being lazy here. */
    update();
}

//--------------------------------------------------

void qtauNoteEditor::recalcNoteRects()
{
    notes.grid.clear();

    setup.barWidth  = setup.note.width() * setup.notesInBar;
    setup.octHeight = setup.note.height() * 12;

    double pulsesToPixels = (double)setup.note.width() / MIDI_PPQ;
    int startBar = 0, endBar = 0;

    foreach (quint64 key, notes.idMap.keys())
    {
        qne::editorNote &n = notes.idMap[key];

        n.r.setRect((double)n.pulseOffset * pulsesToPixels + F_ROUNDER,
                   ((setup.baseOctave + setup.numOctaves - 1) * 12 - n.keyNumber) * setup.note.height(),
                   (double)n.pulseLength * pulsesToPixels + F_ROUNDER, setup.note.height());

        // determine bar(s) that have that note fully or partially
        startBar = n.r.left()  / setup.barWidth;
        endBar   = n.r.right() / setup.barWidth;

        if (endBar >= notes.grid.size())
            notes.grid.resize(endBar + 10);

        notes.grid[startBar].append(n.id);

        if (endBar != startBar)
            notes.grid[endBar].append(n.id);
    }
}

void qtauNoteEditor::updateBGCache()
{
    setup.barWidth  = setup.note.width()  * setup.notesInBar;
    setup.octHeight = setup.note.height() * 12;

    int requiredCacheWidth  = (geometry().width()  / setup.barWidth  + 2) * setup.barWidth;
    int requiredCacheHeight = (geometry().height() / setup.octHeight + 2) * setup.octHeight;

    if (!bgCache || (bgCache->width() < requiredCacheWidth || bgCache->height() < requiredCacheHeight))
    {
        if (bgCache)
            delete bgCache;

        bgCache = new QPixmap(requiredCacheWidth, requiredCacheHeight);
    }

    // prepare bg/grid data =========================================
    QPainterPath blacks;   // rects for black keys
    QPainterPath outerLines;
    QPainterPath innerLines;

    // calculating indexes of visible notes of grid ----------------------
    int hSt  = 0;
    int vSt  = 0;
    int hEnd = requiredCacheWidth  - 1;
    int vEnd = requiredCacheHeight - 1;

    int pxHOff = 0;
    int pxVOff = 0;

    int barCounter = 0;
    int octCounter = 0;

    // horizontal pass to calc note & bar vertical delimiter lines
    do {
        if (barCounter == setup.notesInBar)
            barCounter = 0;

        if (barCounter == 0)
        {
            outerLines.moveTo(QPoint(pxHOff, vSt ));
            outerLines.lineTo(QPoint(pxHOff, vEnd));
        }
        else {
            innerLines.moveTo(QPoint(pxHOff, vSt ));
            innerLines.lineTo(QPoint(pxHOff, vEnd));
        }

        barCounter++;
        pxHOff += setup.note.width();

    } while (pxHOff <= hEnd);

    QRect noteBG(hSt, 0, hEnd - hSt, setup.note.height());

    // vertical pass to calc note backgrounds, note & octave delimiter lines
    do {
        if (octCounter == 12)
            octCounter = 0;

        if (octCounter == 0)
        {
            outerLines.moveTo(QPoint(hSt,  pxVOff));
            outerLines.lineTo(QPoint(hEnd, pxVOff));
        }
        else {
            innerLines.moveTo(QPoint(hSt,  pxVOff));
            innerLines.lineTo(QPoint(hEnd, pxVOff));
        }

        //--- note bg's --------------
        noteBG.moveTop(pxVOff);

        if (octCounter == 1 || octCounter == 3 || octCounter == 5 || octCounter == 8 || octCounter == 10)
            blacks.addRect(noteBG);
        //----------------------------

        octCounter++;
        pxVOff += setup.note.height();

    } while (pxVOff <= vEnd);

    // paint 'em! ======================
    bgCache->fill (Qt::white);
    QPainter p    (bgCache);
    QBrush   brush(p.brush());

    p.setPen(Qt::NoPen);

    // background -------------
    if (!blacks.isEmpty())
    {
        brush.setStyle(Qt::Dense6Pattern);
        brush.setColor(DEFCOLOR_BLACK_NOTELINE_BG);
        p.setBrush(brush);

        p.drawPath(blacks);
    }

    p.setPen(Qt::SolidLine);

    // lines ------------------
    if (!innerLines.isEmpty())
    {
        p.setPen(QColor(DEFCOLOR_INNER_LINE));
        p.drawPath(innerLines);
    }

    if (!outerLines.isEmpty())
    {
        p.setPen(QColor(DEFCOLOR_OUTER_LINE));
        p.drawPath(outerLines);
    }
}

void qtauNoteEditor::setVOffset(int voff)
{
    if (voff != state.viewport.y())
    {
        ctrl->reset();
        state.viewport.moveTop(voff);
        lazyUpdate();
    }
}

void qtauNoteEditor::setHOffset(int hoff)
{
    if (hoff != state.viewport.x())
    {
        ctrl->reset();
        state.viewport.moveLeft(hoff);
        lazyUpdate();
    }
}

QPoint qtauNoteEditor::scrollTo(const QRect &r)
{
    QPoint result = state.viewport.topLeft();

    if (r.x() < state.viewport.x())
        result.setX(r.x() - CONST_SCROLL_MARGIN);
    else
        if (r.x() > state.viewport.x() + geometry().width() - CONST_CACHE_LINE_WIDTH - CONST_SCROLL_MARGIN)
            result.setX(r.x() - geometry().width() / 2);

    if (r.y() < state.viewport.y())
        result.setY(r.y() - CONST_SCROLL_MARGIN);
    else
        if (r.y() > state.viewport.y() + geometry().height() - CONST_SCROLL_MARGIN)
            result.setY(r.y() - geometry().height() / 2);

    if (result != state.viewport.topLeft())
        emit requestsOffset(result);

    return result;
}

//----------------------------------------------------

void qtauNoteEditor::paintEvent(QPaintEvent *event)
{
    //lastUpdate = t.elapsed();

    // draw bg
    QRect r(event->rect());

    int hSt          = r.x() + state.viewport.x();
    int firstBar     = hSt / setup.barWidth;
    int cacheHOffset = hSt - firstBar * setup.barWidth;

    int vSt          = r.y() + state.viewport.y();
    int firstOct     = vSt / setup.octHeight;
    int cacheVOffset = vSt - firstOct * setup.octHeight;

    QRect cacheRect(r);
    cacheRect.moveTo(cacheRect.x() + cacheHOffset, cacheRect.y() + cacheVOffset);

    QPainter p(this);
    p.drawPixmap(r, *bgCache, cacheRect);

    // singing notes with phoneme labels -------
    int barSt    = firstBar;
    int barEnd   = (hSt + r.width()) / setup.barWidth;

    QPainterPath noteRects;
    QPainterPath selNoteRects;

    QMap<quint64, bool>               processedIDMap;
    QVector<QPainter::PixmapFragment> cachedLabels;

    QPainter cacheP(labelCache);
    cacheP.setBrush(Qt::white); // to clear pixmap completely

    p.translate(-state.viewport.topLeft());

    if (barSt < notes.grid.size())
    {
        if (barEnd >= notes.grid.size())
            barEnd = notes.grid.size() - 1;

        bool hasVisibleNotes = false;
        r.moveTo(state.viewport.topLeft());

        for (int i = barSt; i < barEnd + 1; ++i)
            for (int k = 0; k < notes.grid[i].size(); ++k)
            {
                qne::editorNote &n = notes.idMap[notes.grid[i][k]];

                if (!processedIDMap.contains(n.id) && r.intersects(n.r))
                {
                    hasVisibleNotes = true;

                    if (n.selected) selNoteRects.addRect(n.r);
                    else            noteRects   .addRect(n.r);

                    if (n.r.width() > CONST_LABEL_DRAW_MINWIDTH) // don't draw labels for too narrow rects
                    {
                        QRectF fR(0, n.id * CONST_CACHE_LINE_HEIGHT,
                                  CONST_CACHE_LINE_WIDTH, CONST_CACHE_LINE_HEIGHT);

                        if (!n.cached)
                        {
                            // clearing cache line
                            cacheP.setCompositionMode(QPainter::CompositionMode_Clear);
                            cacheP.drawRect(fR);
                            cacheP.setCompositionMode(QPainter::CompositionMode_SourceOver);

                            cacheP.drawText(fR, Qt::AlignLeft | Qt::AlignVCenter, n.txt);
                            n.cached = true;
                        }

                        cachedLabels.append(QPainter::PixmapFragment::create(
                                                QPointF(n.r.x() + 55, n.r.y() + 7), fR)); // wtf is with pos?..
                    }
                }

                processedIDMap[n.id] = true; // to avoid processing notes that go through 2 or more bars
            }

        if (hasVisibleNotes)
        {
            p.setPen  (QColor(DEFCOLOR_NOTE_BORDER));
            p.setBrush(QColor(DEFCOLOR_NOTE_BG));

            p.drawPath(noteRects);
            p.setBrush(QColor(DEFCOLOR_NOTE_SEL_BG));
            p.setPen(QColor(DEFCOLOR_NOTE_SEL));
            p.drawPath(selNoteRects);

            p.drawPixmapFragments(cachedLabels.data(), cachedLabels.size(), *labelCache);
        }
    }

    if (state.selectionRect.x() > -1 && state.selectionRect.y() > -1)
    {
        QPen pen = p.pen();
        pen.setWidth(1);
        pen.setColor(QColor(100,100,100,80));
        p.setPen(pen);
        p.setBrush(QBrush(QColor(128,128,128,50))); // TODO: unhardcode colors?
        p.drawRect(state.selectionRect);
    }

    if (state.snapLine > -1)
    {
        QPen pen = p.pen();
        pen.setWidth(1);
        pen.setColor(QColor(DEFCOLOR_SNAP_LINE));
        p.setPen(pen);
        p.drawLine(state.snapLine, vSt, state.snapLine, vSt + state.viewport.height());
    }

    updateCalled = false;
}

void qtauNoteEditor::resizeEvent(QResizeEvent *event)
{
    state.viewport.setSize(event->size());
    updateBGCache();
    emit heightChanged(state.viewport.height());
    emit widthChanged (state.viewport.width ());
}

void qtauNoteEditor::mouseDoubleClickEvent(QMouseEvent *event) { ctrl->mouseDoubleClickEvent(event); }
void qtauNoteEditor::mouseMoveEvent       (QMouseEvent *event) { ctrl->mouseMoveEvent(event);        }
void qtauNoteEditor::mousePressEvent      (QMouseEvent *event) { ctrl->mousePressEvent(event);       }
void qtauNoteEditor::mouseReleaseEvent    (QMouseEvent *event) { ctrl->mouseReleaseEvent(event);     }

void qtauNoteEditor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier)
        emit hscrolled(event->delta());
    else if (event->modifiers() & Qt::ControlModifier)
        emit zoomed(event->delta());
    else
        emit vscrolled(event->delta());
}

void qtauNoteEditor::changeController(qtauEdController *c)
{
    if (c)
    {
        if (lastCtrl)
            delete lastCtrl;

        if (ctrl)
            ctrl->cleanup(); // since we're not deleting last one (it's dangerous), need to stop it if it isn't

        lastCtrl = ctrl;
        ctrl = c;
        ctrl->init();
    }
}

void qtauNoteEditor::rmbScrollHappened(const QPoint &delta, const QPoint &offset)
{
    emit rmbScrolled(delta, offset);
}

void qtauNoteEditor::eventHappened(qtauEvent *e)
{
    emit editorEvent(e);
}
