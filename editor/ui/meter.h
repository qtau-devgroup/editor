/* meter.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef METER_H
#define METER_H

#include "Utils.h"
#include <QWidget>

class QPixmap;


class qtauMeterBar : public QWidget
{
    Q_OBJECT

public:
    qtauMeterBar(QWidget *parent = 0);
    ~qtauMeterBar();

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

    int offset;
    SNoteSetup ns;

    QPixmap *bgCache;
    QPixmap *labelCache;

    void updateCache();

};

#endif // METER_H
