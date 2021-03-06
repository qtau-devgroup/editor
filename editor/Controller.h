/* Controller.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QMap>
#include <QDir>
#include <QThread>

class MainWindow;
class qtauSynth;
class qtmmPlayer;
class qtauAudioSource;
class qtauSession;
class ISynth;


// main class of QTau that ties everything together
class qtauController : public QObject
{
    Q_OBJECT
    QThread audioThread;

public:
    explicit qtauController(QObject *parent = 0);
    ~qtauController();

    bool run(); // app startup & setup, window creation

signals:
    void setEffect(qtauAudioSource *e, bool replace, bool smoothly, bool copy);
    void setTrack (qtauAudioSource *t, bool replace, bool smoothly, bool copy);

    void playStart();
    void playPause();
    void playStop();

    void playerSetVolume(int level);

public slots:
    void onAppMessage(const QString& msg);

    void onLoadUST(QString fileName);
    void onSaveUST(QString fileName, bool rewrite);

    void onLoadAudio(QString fileName);
    void onSaveAudio(QString fileName, bool rewrite);

    void onAudioPlaybackEnded();
    void onAudioPlaybackTick(qint64 mcsecElapsed);

    void onRequestSynthesis();
    void onRequestStartPlayback();
    void onRequestPausePlayback();
    void onRequestStopPlayback();
    void onRequestResetPlayback();
    void onRequestRepeatPlayback();

    void onVolumeChanged(int);

    void pianoKeyPressed(int);
    void pianoKeyReleased(int);

protected:
    qtmmPlayer *player;
    MainWindow *mw;

    QMap<QString, qtauSession*> sessions;
    qtauSession *activeSession;

    typedef enum {
        Playing = 0,
        Paused,
        Stopped,
        Repeating
    } EPlayerState;

    typedef struct _PlayState {
        EPlayerState     state;
        qtauAudioSource *audio;
        qtauSession     *session;

        _PlayState() : state(Stopped), audio(nullptr), session(nullptr) {}
    } SPlayState;

    SPlayState playState;

    bool setupTranslations();
    bool setupPlugins();
    bool setupVoicebanks();

    void initSynth(ISynth *s);
    QMap<QString, ISynth*> synths;

    QDir pluginsDir;

    void newEmptySession();
};

#endif // CONTROLLER_H
