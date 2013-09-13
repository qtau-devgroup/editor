/* piano.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef PIANO_H
#define PIANO_H

#include <QVector>
#include "Utils.h"

class QPixmap;


class qtauPiano : public QWidget
{
    Q_OBJECT

public:
    qtauPiano(QWidget *parent = 0);
    ~qtauPiano();

    void setOffset(int voff);
    void configure(const SNoteSetup &newSetup);

signals:
    void keyPressed (int keyNum);
    void keyReleased(int keyNum);

    void scrolled(int delta);
    void heightChanged(int newHeight);

public slots:
    //

protected:
    void paintEvent           (QPaintEvent  *event);
    void resizeEvent          (QResizeEvent *event);

    void mouseDoubleClickEvent(QMouseEvent  *event);
    void mouseMoveEvent       (QMouseEvent  *event);
    void mousePressEvent      (QMouseEvent  *event);
    void mouseReleaseEvent    (QMouseEvent  *event);
    void wheelEvent           (QWheelEvent  *event);

    void onHover(bool inside);
    bool eventFilter(QObject *object, QEvent *event);

    void initPiano(int baseOctave, int octavesNum);

    QPoint   offset; // graphical offset of keys, used for virtual scrolling
    QPixmap *labelCache;

public:
    //-------------------------------------------------------------
    typedef enum {
        Passive = 0,
        Pressed,
        Highlighted
    } pianoKeyState;

    typedef struct _key {
        int   octIndex; // 0..11
        int   number;   // absolute number (from first key of first octave)
        bool  isBlack;
        QRect c;        // coordinates

        pianoKeyState state;

        void init(const QRect &r, int ind, int absInd, bool bl) {
            octIndex = ind;
            number = absInd;
            isBlack = bl;
            c = r;
            state = Passive;
        }
    } key;

    //-------------------------------------------------------------

    typedef struct _octave {
        QRect c;        // coordinates
        key   keys[12];

        _octave() {}

        _octave(const QRect &r, int octN)
        {
            c = r;

            int octOff = (octN - 1) * 12; // octN starting from 1 minimum, not an index

            int whH = r.height() / 7;   // white keys height
            int blH = whH * 0.66;       // black keys height
            int bHi = blH * 0.66;       // offset for higher black key from nect white key
            int bMd = blH * 0.5;        // offset for middle black key
            int bLw = blH * 0.33;       // offset for lower black key
            int ww  = r.width() - 2;    // width of white keys
            int bw  = ww * 0.66;        // width of black keys

            // gen 7 whites
            keys[0] .init(QRect(r.x(), r.y(),                ww, whH), 0,  octOff + 12, false);
            keys[2] .init(QRect(r.x(), r.y() + whH,          ww, whH), 2,  octOff + 10, false);
            keys[4] .init(QRect(r.x(), r.y() + whH*2,        ww, whH), 4,  octOff + 8,  false);
            keys[6] .init(QRect(r.x(), r.y() + whH*3,        ww, whH), 6,  octOff + 6,  false);
            keys[7] .init(QRect(r.x(), r.y() + whH*4,        ww, whH), 7,  octOff + 5,  false);
            keys[9] .init(QRect(r.x(), r.y() + whH*5,        ww, whH), 9,  octOff + 3,  false);
            keys[11].init(QRect(r.x(), r.y() + whH*6,        ww, whH), 11, octOff + 1,  false);

            // gen 5 blacks
            keys[1] .init(QRect(r.x(), keys[2] .c.y() - bHi, bw, blH), 1,  octOff + 11, true);
            keys[3] .init(QRect(r.x(), keys[4] .c.y() - bMd, bw, blH), 3,  octOff + 9,  true);
            keys[5] .init(QRect(r.x(), keys[6] .c.y() - bLw, bw, blH), 5,  octOff + 7,  true);
            keys[8] .init(QRect(r.x(), keys[9] .c.y() - bHi, bw, blH), 8,  octOff + 4,  true);
            keys[10].init(QRect(r.x(), keys[11].c.y() - bLw, bw, blH), 10, octOff + 2,  true);
        }

        int keyIndex (QPoint p) // returns 0..11, -1 if not found
        {
            int result = -1;

            if (c.contains(p))
                for (int i = 0; i < 12; ++i)     // TODO: something simpler, based on rect height?
                    if (keys[i].c.contains(p))
                    {
                        result = i; // set any key to result, immediately break if isBlack

                        if (keys[i].isBlack)
                            break;
                    }
                    else if (result > -1) // if cycling after single white is found
                        break;

            return result;
        }

        key* keyAt(const QPoint &p)   // returns null if not found
        {
            key *result = 0;
            int  ind    = keyIndex(p);

            assert(ind < 12);

            if (ind > -1)
                result = &keys[ind];

            return result;
        }

        void setWidth(int newWidth)
        {
            newWidth -= 2;
            c.setWidth(newWidth);

            for (int i = 0; i < 12; ++i)
                if (keys[i].isBlack) keys[i].c.setWidth(newWidth * 0.66);
                else                 keys[i].c.setWidth(newWidth);
        }
    } octave;
    //-------------------------------------------------------------

protected:
    QVector<octave> octaves;
    key *pressedKey = nullptr;
    key *hoveredKey = nullptr;
    SNoteSetup ns;

};

#endif // PIANO_H
