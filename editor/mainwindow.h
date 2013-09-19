/* mainwindow.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>

#include "Utils.h"
#include "utauloid/ust.h"

class qtauController;
class qtauSession;
class qtauEvent;
class qtauPiano;
class qtauNoteEditor;
class qtauMeterBar;
class qtauDynDrawer;
class qtauDynLabel;
class qtauWaveform;

class QAction;
class QScrollBar;
class QSlider;
class QToolBar;
class QTabWidget;
class QTextEdit;
class QToolBar;
class QSplitter;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool setController(qtauController &c, qtauSession &s);

private:
    Ui::MainWindow *ui;

signals:
    void loadUST  (QString fileName);
    void saveUST  (QString fileName, bool rewrite);
    void saveAudio(QString fileName, bool rewrite);

    void loadAudio(QString fileName);
    void setVolume(int);

public slots:
    void onLog(const QString&, ELog);
    void onOpenUST();
    void onSaveUST();
    void onSaveUSTAs();

    void onVocalAudioChanged();
    void onMusicAudioChanged();

    void notesVScrolled(int);
    void notesHScrolled(int);
    void vertScrolled(int);
    void horzScrolled(int);

    void onEditorRMBScrolled(QPoint, QPoint);
    void onEditorRequestOffset(QPoint);

    void onPianoHeightChanged    (int newHeight);
    void onNoteEditorWidthChanged(int newWidth);

    void onPlaybackState(EAudioPlayback);
    void onMute(bool m);

    void onUndo();
    void onRedo();
    void onDelete();

    void onEditMode(bool);
    void onGridSnap(bool);
    void onQuantizeSelected(int);
    void onNotelengthSelected(int);

    void dynBtnLClicked();
    void dynBtnRClicked();

    void onTabSelected(int);
    void onZoomed(int);
    void onEditorZoomed(int);

    void onDocReloaded();
    void onDocStatus(bool);
    void onUndoStatus(bool);
    void onRedoStatus(bool);
    void onDocEvent(qtauEvent*);

    void onSaveAudioAs();

protected:
    qtauSession    *doc;
    SNoteSetup      ns;
    QTabWidget     *tabs;

    qtauPiano      *piano;
    qtauNoteEditor *noteEditor;
    qtauDynDrawer  *drawZone;
    qtauMeterBar   *meter;

    qtauWaveform   *vocalWave;
    qtauWaveform   *musicWave;
    QWidget        *wavePanel; // used to switch its visibility, hidden by default
    QWidget        *drawZonePanel;
    QSplitter      *editorSplitter;

    qtauDynLabel   *fgDynLbl;
    qtauDynLabel   *bgDynLbl;

    QScrollBar     *hscr;
    QScrollBar     *vscr;
    QSlider        *zoom;
    QSlider        *volume;
    QAction        *muteBtn;

    QTextEdit      *logpad;

    QList<QToolBar*> toolbars;
    void enableToolbars(bool enable = true);

    EPlayer playState;

    QColor  logTabTextColor;
    int     logNewMessages;
    bool    logHasErrors;
    bool    showNewLogNumber;

    QString docName;
    QString lastScoreDir;
    QString lastAudioDir;
    QString audioExt;

    void dragEnterEvent (QDragEnterEvent*);
    void dragMoveEvent  (QDragMoveEvent *);
    void dropEvent      (QDropEvent     *);
    void closeEvent     (QCloseEvent    *);

};

#endif // MAINWINDOW_H
