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
    void playEffect(const qtauAudioSource &e);
    void playTrack (const qtauAudioSource &t);

    void playStart();
    void playPause();
    void playStop();

public slots:
    void onAppMessage(const QString& msg);

    void onLoadUST(QString fileName);
    void onSaveUST(QString fileName, bool rewrite);

    void onLoadAudio(QString fileName);
    void onSaveAudio(QString fileName, bool rewrite);
    void onAudioPlaybackEnded();

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
    qtmmPlayer *player = nullptr;
    MainWindow *mw     = nullptr;

    QMap<QString, qtauSession*> sessions;
    qtauSession *activeSession = nullptr;

    typedef enum {
        Playing = 0,
        Paused,
        Stopped,
        Repeating
    } EPlayerState;

    typedef struct {
        EPlayerState     state;
        qtauAudioSource *audio;
        qtauSession     *session;
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
