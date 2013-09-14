/* piano.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "ui/piano.h"
#include <qevent.h>
#include <qpainter.h>
#include <QDebug>

#include "ui/Config.h"

const int CONST_PIANO_LABELHEIGHT = 14;
const int CONST_PIANO_LABELWIDTH  = 50;

qtauPiano::qtauPiano(QWidget *parent) :
    QWidget(parent), offset(0,0), labelCache(nullptr), pressedKey(nullptr), hoveredKey(nullptr)
{
    // setup widget
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    installEventFilter(this);

    initPiano(ns.baseOctave, ns.numOctaves);
}

bool qtauPiano::eventFilter(QObject *object, QEvent *event)
{
    if(object==this && (event->type()==QEvent::Enter || event->type()==QEvent::Leave))
        onHover(event->type()==QEvent::Enter);

    return false;
}

//----------------------------------------------------------------
inline void cacheLbl(QPainter &p, QRect &r, const QString &txt, int &vOff, const QColor &offClr, const QColor &onClr)
{
    r.moveTopLeft(QPoint(0, vOff));
    p.setPen(offClr);
    p.drawText(r, txt);

    r.moveLeft(CONST_PIANO_LABELWIDTH);
    p.setPen(onClr);
    p.drawText(r, txt);

    vOff += CONST_PIANO_LABELHEIGHT;
}

inline void whiteLbl(QPainter &p, QRect &r, const QString &txt, int &vOff)
{
    cacheLbl(p, r, txt, vOff, QColor(cdef_color_piano_lbl_wh), QColor(cdef_color_piano_lbl_wh_on));
}

inline void blackLbl(QPainter &p, QRect &r, const QString &txt, int &vOff)
{
    cacheLbl(p, r, txt, vOff, QColor(cdef_color_piano_lbl_bl), QColor(cdef_color_piano_lbl_bl_on));
}
//----------------------------------------------------------------

void qtauPiano::initPiano(int baseOctave, int octavesNum)
{
    octaves.clear();
    pressedKey = 0;

    // make octave keyblocks from up to bottom, from last block to first
    QRect thisR(this->geometry());
    QRect r(0, 0, thisR.width()-2, ns.note.height() * 12); // magical -2, because magic
    int octNum = baseOctave + octavesNum - 1; // including base num

    for (int i = 0; i < octavesNum; ++i, --octNum)
    {
        r.moveTop(i * r.height());
        octaves.append(octave(r, octNum));
    }

    // cache labels
    if (labelCache)
        delete labelCache;

    labelCache = new QPixmap(CONST_PIANO_LABELWIDTH * 2 + 10, CONST_PIANO_LABELHEIGHT * ns.numOctaves * 12 + 100);
    labelCache->fill(Qt::transparent);

    QPainter p(labelCache);
    QFont f("Arial", 10);
    p.setFont(f);

    QRect lblR(0,0,CONST_PIANO_LABELWIDTH, CONST_PIANO_LABELHEIGHT);
    int vOff = 0;
    int lastOct = ns.baseOctave + ns.numOctaves - 1;
    int octN = ns.baseOctave;

    do
    {
        whiteLbl(p, lblR, QString("C%1") .arg(octN), vOff);
        blackLbl(p, lblR, QString("C#%1").arg(octN), vOff);

        whiteLbl(p, lblR, QString("D%1") .arg(octN), vOff);
        blackLbl(p, lblR, QString("D#%1").arg(octN), vOff);

        whiteLbl(p, lblR, QString("E%1") .arg(octN), vOff);

        whiteLbl(p, lblR, QString("F%1") .arg(octN), vOff);
        blackLbl(p, lblR, QString("F#%1").arg(octN), vOff);

        whiteLbl(p, lblR, QString("G%1") .arg(octN), vOff);
        blackLbl(p, lblR, QString("G#%1").arg(octN), vOff);

        whiteLbl(p, lblR, QString("A%1") .arg(octN), vOff);
        blackLbl(p, lblR, QString("A#%1").arg(octN), vOff);

        whiteLbl(p, lblR, QString("B%1") .arg(octN), vOff);

        ++octN;
    } while (octN <= lastOct);
}

qtauPiano::~qtauPiano()
{
    //
}

void qtauPiano::setOffset(int voff)
{
    if (offset.y() != voff)
    {
        offset.setY(voff);
        update();
    }
}

void qtauPiano::configure(const SNoteSetup &newSetup)
{
    ns = newSetup;
    initPiano(ns.baseOctave, ns.numOctaves);
    update();
}

//--------------------------------------------

void qtauPiano::paintEvent(QPaintEvent *event)
{
    QVector<QRect> whites;
    QVector<QRect> blacks;
    QVector<QRect> whLit;
    QVector<QRect> blLit;

    QVector<QPainter::PixmapFragment> labels;

    QRect trR(event->rect()); // translated rect
    trR.moveTo(trR.topLeft() + offset);

    QRect  drR; // rect with drawing coordinates, translated from virtual keys space to widget area
    QRectF lblR(0, 0, CONST_PIANO_LABELWIDTH, CONST_PIANO_LABELHEIGHT);

    QPoint lblOff(2, -6);

    // determine what octave keyblocks are in repaint rect
    foreach (const octave &o, octaves)
    {
        if (trR.intersects(o.c))  // render whatever keys in octave are visible
            for (int i = 0; i < 12; ++i)
                if (o.keys[i].c.intersects(trR))
                {
                    const key &k = o.keys[i];
                    drR = k.c;

                    if (k.state == Pressed)
                    {
                        if (k.isBlack) blLit.append(drR);
                        else                   whLit.append(drR);

                        lblR.moveTo(CONST_PIANO_LABELWIDTH, (k.number - 1) * CONST_PIANO_LABELHEIGHT);
                        labels.append(QPainter::PixmapFragment::create(QPointF(drR.bottomRight() + lblOff), lblR));
                    }
                    else
                    {
                        if (k.isBlack) blacks.append(drR);
                        else                   whites.append(drR);

                        if (k.octIndex == 11 || (hoveredKey && k.number == hoveredKey->number))
                        {
                            lblR.moveTo(0, (k.number - 1) * CONST_PIANO_LABELHEIGHT);
                            labels.append(QPainter::PixmapFragment::create(QPointF(drR.bottomRight() + lblOff), lblR));
                        }
                    }
                }
    }

    if (!(whites.isEmpty() && blacks.isEmpty()))
    {
        QPainter p(this);
        p.translate(-offset);

        QPen pen = p.pen();
        pen.setStyle(Qt::SolidLine);
        pen.setColor(Qt::black);
        pen.setWidth(1);
        p.setPen(pen);

        p.setBrush(Qt::SolidPattern);

        if (!whites.isEmpty())
        {
            p.setBrush(Qt::white);
            p.drawRects(whites);
        }

        if (!whLit.isEmpty())
        {
            p.setBrush(Qt::lightGray);
            p.drawRects(whLit);
        }

        if (!blacks.isEmpty())
        {
            p.setBrush(QColor(cdef_color_black_key_bg));
            p.drawRects(blacks);
        }

        if (!blLit.isEmpty())
        {
            p.setBrush(Qt::gray);
            p.drawRects(blLit);
        }

        if (!labels.isEmpty())
            p.drawPixmapFragments(labels.data(), labels.size(), *labelCache);
    }
}

void qtauPiano::resizeEvent(QResizeEvent * event)
{
    for (int i = 0; i < octaves.size(); ++i)
        octaves[i].setWidth(event->size().width());

    emit heightChanged(event->size().height());
}


//------------ input handling ---------------

void qtauPiano::mouseDoubleClickEvent(QMouseEvent *event)
{
    QPoint pos(event->pos() + offset);

    for (int i = 0; i < octaves.size(); ++i)
        if (octaves[i].c.contains(pos))
        {
            key *k = octaves[i].keyAt(pos);

            if (k != 0) // found pressed key
            {
                emit keyPressed (k->number);
                emit keyReleased(k->number);
                break;
            }
        }
}

void qtauPiano::mouseMoveEvent(QMouseEvent *event)
{
    key *newHovered = 0;
    QPoint pos(event->pos() + offset);

    for (int i = 0; i < octaves.size(); ++i)
        if (octaves[i].c.contains(pos))
        {
            newHovered = octaves[i].keyAt(pos);

            if (newHovered != 0) // found pressed key
                break;
        }

    if (hoveredKey && newHovered != hoveredKey) // last key changed
    {
        QRect keyR(hoveredKey->c);
        keyR.moveTo(keyR.topLeft() - offset);
        update(keyR);
    }

    if (newHovered)
    {
        hoveredKey = newHovered;
        QRect keyR(hoveredKey->c);
        keyR.moveTo(keyR.topLeft() - offset);
        update(keyR);
    }
}

void qtauPiano::mousePressEvent(QMouseEvent *event)
{
    QPoint pos(event->pos() + offset);

    for (int i = 0; i < octaves.size(); ++i)
        if (octaves[i].c.contains(pos))
        {
            key *k = octaves[i].keyAt(pos);

            if (k != 0) // found pressed key
            {
                if (pressedKey != 0)
                {
                    pressedKey->state = Passive;
                    QRect lastR(pressedKey->c);
                    lastR.moveTo(lastR.topLeft() - offset);
                    update(lastR);

                    emit keyReleased(pressedKey->number);
                }

                // drawing it pressed
                k->state = Pressed;
                QRect keyR(k->c);
                keyR.moveTo(keyR.topLeft() - offset);
                update(keyR);

                pressedKey = k;

                emit keyPressed(k->number);
                break;
            }
        }
}

void qtauPiano::mouseReleaseEvent(QMouseEvent *event)
{
    key *k = 0;

    if (pressedKey != 0)
    {
        k = pressedKey;
        pressedKey = 0;
    }
    else
    {
        QPoint pos(event->pos() + offset);

        for (int i = 0; i < octaves.size(); ++i)
            if (octaves[i].c.contains(pos))
                k = octaves[i].keyAt(pos);
    }

    if (k != 0)
    {
        // unhold, drawing default
        k->state = Passive;
        QRect keyR(k->c);
        keyR.moveTo(keyR.topLeft() - offset);
        update(keyR);

        emit keyReleased(k->number);
    }
}

void qtauPiano::wheelEvent(QWheelEvent *event)
{
    emit scrolled(event->delta());
}

void qtauPiano::onHover(bool inside)
{
    if (!inside && hoveredKey)
    {
        QRect keyR(hoveredKey->c);
        keyR.moveTo(keyR.topLeft() - offset);
        hoveredKey = 0;
        update(keyR);
    }
}
