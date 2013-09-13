/* waveform.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "Utils.h"
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

    int offset = 0;
    qtauAudioSource *wave = nullptr;

    QPixmap *bgCache = nullptr;
    void updateCache();
    void calcSetup(); // calculate how many samples can fit into geometry.width

    int bpm       = 120;
    int beatWidth = c_zoom_note_widths[cdef_zoom_index]; // in pixels

    float framesVisible = 0;
};

#endif // WAVEFORM_H
