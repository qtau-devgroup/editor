#include "editor/ui/meter.h"
#include <qevent.h>
#include <qpainter.h>
#include <editor/ui/Config.h>

#include <QTime>
#include <QPixmap>

const int CONST_LBLCACHE_LINE_HEIGHT = 12;
const int CONST_LBLCACHE_LINE_WIDTH  = 20;
const int CONST_LBLCACHE_CACHED_NUM  = 200;
const int CONST_LBLCACHE_FONT_HEIGHT = 8;

const int CONST_LBL_HOFFSET = 15; // drawing offset from top-left of octave line
const int CONST_LBL_VOFFSET = 10;

const QString CONST_LBLCACHE_FONT_NAME = "Arial";


qtauMeterBar::qtauMeterBar(QWidget *parent) :
    QWidget(parent), offset(0), bgCache(0)
{
    // cache bar numbers into pixmap - maybe that's a bit of an overkill, but won't hurt RAM usage much
    labelCache = new QPixmap(CONST_LBLCACHE_LINE_WIDTH, CONST_LBLCACHE_LINE_HEIGHT * CONST_LBLCACHE_CACHED_NUM);
    labelCache->fill(Qt::transparent);

    QPainter p(labelCache);
    p.setFont(QFont(CONST_LBLCACHE_FONT_NAME, CONST_LBLCACHE_FONT_HEIGHT));

    QRect lblR(0, 0, CONST_LBLCACHE_LINE_WIDTH, CONST_LBLCACHE_LINE_HEIGHT);

    for (int i = 0; i < CONST_LBLCACHE_CACHED_NUM; ++i)
    {
        lblR.moveTop(i * CONST_LBLCACHE_LINE_HEIGHT);
        p.drawText(lblR, QString("%1").arg(i + 1));
    }
}

qtauMeterBar::~qtauMeterBar()
{
    delete labelCache;
    delete bgCache;
}

void qtauMeterBar::setOffset(int off)
{
    if (off != offset)
    {
        offset = off;
        update();
    }
}

void qtauMeterBar::configure(const noteSetup &newSetup)
{
    ns = newSetup;
    updateCache();
    update();
}

//---------------------------------------------------------

void qtauMeterBar::paintEvent(QPaintEvent *event)
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

    // draw bar numbers
    if (cacheOffset != 0)
        firstBar++; // now it's first visible bar line

    int lastBar = (hSt + event->rect().width()) / barWidth;
    QVector<QPainter::PixmapFragment> cachedLabels;

    int barScreenOffset = firstBar * barWidth - offset;

    for (int i = firstBar; i <= lastBar; ++i, barScreenOffset += barWidth)
        cachedLabels.append(QPainter::PixmapFragment::create(
            QPointF(barScreenOffset + CONST_LBL_HOFFSET, CONST_LBL_VOFFSET),
            QRectF(0, i * CONST_LBLCACHE_LINE_HEIGHT, CONST_LBLCACHE_LINE_WIDTH, CONST_LBLCACHE_LINE_HEIGHT)));

    if (!cachedLabels.isEmpty())
        p.drawPixmapFragments(cachedLabels.data(), cachedLabels.size(), *labelCache);
}

void qtauMeterBar::resizeEvent(QResizeEvent*)
{
    updateCache();
}

void qtauMeterBar::mouseDoubleClickEvent(QMouseEvent*)
{
    //
}

void qtauMeterBar::mouseMoveEvent(QMouseEvent*)
{
    //
}

void qtauMeterBar::mousePressEvent(QMouseEvent*)
{
    //
}

void qtauMeterBar::mouseReleaseEvent(QMouseEvent*)
{
    //
}

void qtauMeterBar::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
        emit zoomed(event->delta());
    else
        emit scrolled(event->delta());
}

void qtauMeterBar::updateCache()
{
    // bars fully fit in + 2 for partially visible bars
    int barWidth = ns.note.width() * ns.notesInBar;
    int requiredCacheWidth = (geometry().width() / barWidth + 2) * barWidth;

    if (!bgCache || bgCache->width() < requiredCacheWidth)
    {
        if (bgCache)
            delete bgCache;

        bgCache = new QPixmap(requiredCacheWidth, geometry().height());
    }

    // prepare painting data
    int vSt  = 0;
    int vEnd = geometry().height();
    int vMid = vEnd * 0.6;

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
        else // note line (low)
        {
            noteLines.append(QPoint(pxOff, vMid));
            noteLines.append(QPoint(pxOff, vEnd));
        }

        barCounter++;
        pxOff += ns.note.width();
    }

    // paint 'em!
    bgCache->fill(Qt::white);
    QPainter p(bgCache);

    p.setPen(QColor(DEFCOLOR_INNER_LINE));
    p.drawLines(noteLines);

    p.setPen(QColor(DEFCOLOR_OUTER_LINE));
    p.drawLines(barLines);
}
