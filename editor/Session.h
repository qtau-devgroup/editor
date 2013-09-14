/* Session.h from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#ifndef SESSION_H
#define SESSION_H

#include "Utils.h"
#include "NoteEvents.h"
#include "utauloid/ust.h"

#include <QMap>

class qtauAudioSource;


/** Work session that contains one voice setup (notes/lyrics/effects)
 and voicebank selection+setup to synthesize one song.
 */
class qtauSession : public qtauEventManager
{
    Q_OBJECT

public:
    explicit qtauSession(QObject *parent = 0);
    ~qtauSession();

    bool loadUST(QString fileName);

    void setSynthesizedVocal(qtauAudioSource &s);
    void setBackgroundAudio (qtauAudioSource &s);

    QStringList ustStrings(bool selectionOnly = false);
    QByteArray  ustBinary();
    const ust&  ustRef();

    QString documentName() { return docName; }
    QString documentFile() { return filePath; }

    void setDocName(const QString &name);
    void setFilePath(const QString &fp);

    bool isSessionEmpty()    const { return noteMap.isEmpty(); } /// returns true if doesn't contain any data
    bool isSessionModified() const { return isModified; }        /// if has changes from last save/load

    void setModified(bool m);
    void setSaved(); // if doc was saved at this point

    EAudioPlayback playbackState() const { return playSt; }
    void setPlaybackState(EAudioPlayback state);

    typedef struct {
        qtauAudioSource *vocalWave = nullptr;

        bool  needsSynthesis = true;
        float volume         = 1;
    } VocalWaveSetup;

    typedef struct {
        qtauAudioSource *musicWave = nullptr;

        qint64 offset = 0;
        int    tempo  = 120;
        float  volume = 1;
    } MusicWaveSetup;

    VocalWaveSetup& getVocal() { return vocal; }
    MusicWaveSetup& getMusic() { return music; }

signals:
    void modifiedStatus(bool); /// if document is modified
    void undoStatus    (bool); /// if can undo last stored action
    void redoStatus    (bool); /// if can apply previously reverted action

    void dataReloaded();       /// when data is changed completely
    void playbackStateChanged(EAudioPlayback);

    void vocalSet(); // when session gets synthesized audio from score
    void musicSet(); // when user adds bg (off-vocal?) music to play with synthesized vocals

    // signals to controller
    void requestSynthesis(); // means synth & play
    void requestStartPlayback();
    void requestPausePlayback();
    void requestStopPlayback();
    void requestResetPlayback();
    void requestRepeatPlayback();

public slots:
    void onUIEvent(qtauEvent *);

    // received from UI
    void startPlayback();
    void stopPlayback();
    void resetPlayback();
    void repeatPlayback();

    void vocalWaveWasModified();
    void musicWaveWasModified();

protected:
    bool    parseUSTStrings(QStringList ustStrings);
    QString filePath;
    QString docName      = QStringLiteral("Untitled");
    bool    isModified   = false;
    bool    hadSavePoint = false; // if was saved having a non-empty event stack

    VocalWaveSetup vocal;
    MusicWaveSetup music;

    QMap<qint64, ust_note> noteMap; // need to store copies until changing data structure to something better
    ust data; // TODO: nite vector inside is obviously unsuitable, needs changing to something else

    void applyEvent_NoteAdded  (const qtauEvent_NoteAddition &event);
    void applyEvent_NoteMoved  (const qtauEvent_NoteMove     &event);
    void applyEvent_NoteResized(const qtauEvent_NoteResize   &event);
    void applyEvent_NoteLyrics (const qtauEvent_NoteText     &event);
    void applyEvent_NoteEffects(const qtauEvent_NoteEffect   &event);

    bool processEvent(qtauEvent *) override;
    void stackChanged()            override;

    EAudioPlayback playSt = EAudioPlayback::noAudio;
};

#endif // SESSION_H
