/* dynDrawer.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "ui/dynDrawer.h"
#include "ui/Config.h"

#include <QPainter>
#include <qevent.h>
#include <QPixmap>


qtauDynLabel::qtauDynLabel(const QString &txt, QWidget *parent) : QLabel(txt, parent), _state(off) {}
qtauDynLabel::~qtauDynLabel() {}

void qtauDynLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit leftClicked();
    else if (event->button() == Qt::RightButton)
        emit rightClicked();
}

qtauDynLabel::EState qtauDynLabel::state() { return _state; }
void qtauDynLabel::setState(EState s)      { _state = s; }

//===========================================================================


qtauDynDrawer::qtauDynDrawer(QWidget *parent) :
    QWidget(parent), offset(0), bgCache(nullptr)
{
    //
}

qtauDynDrawer::~qtauDynDrawer()
{
    //
}

void qtauDynDrawer::setOffset(int off)
{
    if (off != offset)
    {
        offset = off;
        update();
    }
}

void qtauDynDrawer::configure(const SNoteSetup &newSetup)
{
    ns = newSetup;
    updateCache();
    update();
}

//------------------------------------

void qtauDynDrawer::paintEvent(QPaintEvent *event)
{
    // draw bg
    int hSt         = event->rect().x() + offset;
    int barWidth    = ns.note.width() * ns.notesInBar;
    int firstBar    = hSt / barWidth;
    int cacheOffset = hSt - firstBar * barWidth;

    QRect screenRect(event->rect());
    QRect cacheRect(screenRect);
    cacheRect.moveLeft(cacheRect.x() + cacheOffset);

    QPainter p(this);
    p.drawPixmap(screenRect, *bgCache, cacheRect);
}

void qtauDynDrawer::resizeEvent(QResizeEvent*)
{
    updateCache();
}

void qtauDynDrawer::mouseDoubleClickEvent(QMouseEvent*)
{
    //
}

void qtauDynDrawer::mouseMoveEvent(QMouseEvent*)
{
    //
}

void qtauDynDrawer::mousePressEvent(QMouseEvent*)
{
    //
}

void qtauDynDrawer::mouseReleaseEvent(QMouseEvent*)
{
    //
}

void qtauDynDrawer::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
        emit zoomed(event->delta());
    else
        emit scrolled(event->delta());
}

void qtauDynDrawer::updateCache()
{
    if (geometry().height() == 0)
    {
        vsLog::e("Zero height of dynDrawer in updateCache()... This definitely shouldn't happen.");
        return;
    }

    // bars fully fit in + 2 for partially visible bars
    int barWidth = ns.note.width() * ns.notesInBar;
    int requiredCacheWidth = (geometry().width() / barWidth + 2) * barWidth;

    if (!bgCache || (bgCache->width() < requiredCacheWidth || bgCache->height() < geometry().height()))
    {
        if (bgCache)
            delete bgCache;

        bgCache = new QPixmap(requiredCacheWidth, geometry().height());
    }

    // prepare painting data
    int vSt  = 0;
    int vEnd = geometry().height();
    int mSt  = vEnd / 2 - 10;
    int mEnd = mSt + 20;

    int barCounter = 0;
    int pxOff      = 0;

    QVector<QPoint> noteLines;
    QVector<QPoint> barLines;

    while (true)
    {
        if (pxOff >= requiredCacheWidth)
            break;

        if (barCounter == ns.notesInBar)
            barCounter = 0;

        if (barCounter == 0) // bar line
        {
            barLines.append(QPoint(pxOff, vSt ));
            barLines.append(QPoint(pxOff, vEnd));
        }
        else
        {
            noteLines.append(QPoint(pxOff, mSt ));
            noteLines.append(QPoint(pxOff, mEnd));
        }

        barCounter++;
        pxOff += ns.note.width();

    }

    // paint 'em!
    bgCache->fill(Qt::white);
    QPainter p(bgCache);

    p.setPen(QColor(cdef_color_inner_line));
    p.drawLines(noteLines);

    p.setPen(QColor(cdef_color_outer_line));
    p.drawLines(barLines);
}
