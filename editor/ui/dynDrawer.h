/* dynDrawer.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef DYNDRAWER_H
#define DYNDRAWER_H

#include "Utils.h"

#include <QLabel>

class QPixmap;


class qtauDynLabel : public QLabel
{
    Q_OBJECT

public:
    explicit qtauDynLabel(const QString& txt = "", QWidget *parent = 0);
    ~qtauDynLabel();

    typedef enum {
        off = 0,
        front,
        back
    } EState;

    EState state();
    void setState(EState s);

signals:
    void leftClicked();
    void rightClicked();

protected:
    void mousePressEvent(QMouseEvent * event);

    EState _state;

};

//----------------------------------------------

class qtauDynDrawer : public QWidget
{
    Q_OBJECT

public:
    qtauDynDrawer(QWidget *parent = 0);
    ~qtauDynDrawer();

    void setOffset(int off);
    void configure(const SNoteSetup &newSetup);

signals:
    void scrolled(int delta);
    void zoomed  (int delta);

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

    int offset = 0;

    SNoteSetup ns;

    QPixmap *bgCache = nullptr;
    void updateCache();

};

#endif // DYNDRAWER_H
