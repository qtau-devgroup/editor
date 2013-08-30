#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "editor/Utils.h"
#include <QWidget>

class QPixmap;
class qtauAudioSource;

class qtauWaveform : public QWidget
{
    Q_OBJECT
public:
    explicit qtauWaveform(QWidget *parent = 0);
    ~qtauWaveform();

    void setOffset(int off);
    void configure(int tempo, int noteWidth);
    void setAudio(qtauAudioSource *pcm); // setting 0 means just remove current and show nothing

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
    qtauAudioSource *wave;

    QPixmap *bgCache;
    void updateCache();
    void calcSetup(); // calculate how many samples can fit into geometry.width

    int bpm;
    int beatWidth; // in pixels

    float framesVisible;
};

#endif // WAVEFORM_H
