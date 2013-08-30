#ifndef SESSION_H
#define SESSION_H

#include "editor/Utils.h"
#include "editor/NoteEvents.h"

#include "tools/utauloid/ust.h"

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

    qtauSessionPlayback::EState playbackState() const { return playSt; }
    void setPlaybackState(qtauSessionPlayback::EState state);

    typedef struct SVocalWaveSetup {
        qtauAudioSource *vocalWave;

        bool  needsSynthesis;
        float volume;

        SVocalWaveSetup() : vocalWave(0), needsSynthesis(true), volume(1) {}
    } VocalWaveSetup;

    typedef struct SMusicWaveSetup {
        qtauAudioSource *musicWave;

        qint64 offset;
        int    tempo;
        float  volume;

        SMusicWaveSetup() : musicWave(0), offset(0), tempo(120), volume(1) {}
    } MusicWaveSetup;

    VocalWaveSetup& getVocal() { return vocal; }
    MusicWaveSetup& getMusic() { return music; }

signals:
    void modifiedStatus(bool); /// if document is modified
    void undoStatus    (bool); /// if can undo last stored action
    void redoStatus    (bool); /// if can apply previously reverted action

    void dataReloaded();       /// when data is changed completely
    void playbackStateChanged(qtauSessionPlayback::State);

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

protected:
    bool    parseUSTStrings(QStringList ustStrings);
    QString filePath;
    QString docName;
    bool    isModified;
    bool    hadSavePoint; // if was saved having a non-empty event stack

    VocalWaveSetup vocal;
    MusicWaveSetup music;

    QMap<qint64, ust_note> noteMap; // need to store copies until changing data structure to something better
    ust data; // TODO: nite vector inside is obviously unsuitable, needs changing to something else

    void applyEvent_NoteAdded  (const qtauEvent_NoteAddition &event);
    void applyEvent_NoteMoved  (const qtauEvent_NoteMove     &event);
    void applyEvent_NoteResized(const qtauEvent_NoteResize   &event);
    void applyEvent_NoteLyrics (const qtauEvent_NoteText     &event);
    void applyEvent_NoteEffects(const qtauEvent_NoteEffect   &event);

    void stackChanged();

    qtauSessionPlayback::EState playSt;
};

#endif // SESSION_H
