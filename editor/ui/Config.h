#ifndef QTAUCONFIG_H
#define QTAUCONFIG_H

#include <QPoint>
#include <QSize>
#include <QRect>
#include <QVector>
#include <QMap>

#include <QString>


// default config for custom ui widgets
const unsigned int DEFCOLOR_BLACK_KEY_BG      = 0xff666666;
const unsigned int DEFCOLOR_BLACK_NOTELINE_BG = 0xffb8b8b8; // aarrggbb
const unsigned int DEFCOLOR_INNER_LINE        = 0xffbcbcbc;
const unsigned int DEFCOLOR_OUTER_LINE        = 0xff0095c6;

const unsigned int DEFCOLOR_NOTE_BORDER       = 0xff000000;
const unsigned int DEFCOLOR_NOTE_BG           = 0xffffffff;
const unsigned int DEFCOLOR_NOTE_SEL          = 0xff0095c6;
const unsigned int DEFCOLOR_NOTE_SEL_BG       = 0xffe3eeff;

const unsigned int DEFCOLOR_SELRECT           = 0xff00857d;
const unsigned int DEFCOLOR_SELRECT_BG        = 0x2200857d;

const unsigned int DEFCOLOR_SNAP_LINE         = 0xaa3effab;

const unsigned int DEFCOLOR_PIANO_LBL_WH      = 0xff000000; // colors for white and black key lablels
const unsigned int DEFCOLOR_PIANO_LBL_WH_ON   = 0xff00857d;
const unsigned int DEFCOLOR_PIANO_LBL_BL      = 0xffffffff;
const unsigned int DEFCOLOR_PIANO_LBL_BL_ON   = 0xffeafffe;

const QString      DEFCOLOR_DYNBTN_OFF        = "#b7b7b7"; // CSS color
const QString      DEFCOLOR_DYNBTN_BG         = "#77ded8"; // background graph button color
const QString      DEFCOLOR_DYNBTN_ON         = "#00857d"; // foreground graph button color
const QString      DEFCOLOR_DYNBTN_ON_BG      = "#eafffe";

const QPoint CONST_SLIDECLICK_LIMIT = QPoint(3, 3);


// editor config
namespace qne {
    typedef struct _editorNote {
        quint64 id;
        int     keyNumber;
        int     pulseLength;
        int     pulseOffset;
        QString txt;

        bool    cached;
        bool    selected;

        QRect   r; // rectangle in pixels
        QPoint  dragSt; // used to store start topleft pos in pixels at start of dragging
        // settings
        // effects

        _editorNote() : id(0), keyNumber(0), pulseLength(0), pulseOffset(0), txt("TEXT_HERE"),
            cached(false), selected(false), r(0,0,0,0) {}
    } editorNote;

    typedef struct _editorNotes {
        QMap<quint64, editorNote>  idMap;
        QVector<QVector<quint64> > grid;
        QVector<quint64>           selected;
    } editorNotes;

    typedef struct _editorState {
        QRect viewport;

        bool rmbScrollEnabled;
        bool editingEnabled;
        bool gridSnapEnabled;

        QRect selectionRect;
        int snapLine;

        _editorState() : rmbScrollEnabled(true), editingEnabled(false), gridSnapEnabled(true), snapLine(-1) {}
    } editorState;
}


#endif // QTAUCONFIG_H
