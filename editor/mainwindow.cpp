/* mainwindow.cpp from QTau http://github.com/qtau-devgroup/editor by digited, BSD license */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui>
#include <QtCore>
#include <QIcon>

#include "Controller.h"
#include "Session.h"

#include <QGridLayout>
#include <QScrollBar>
#include <QToolBar>
#include <QTabWidget>
#include <QSplitter>
#include <QGroupBox>
#include <QScrollArea>
#include <QTextEdit>
#include <QComboBox>
#include <QDial>

#include <QFileDialog>

#include "ui/Config.h"
#include "ui/piano.h"
#include "ui/dynDrawer.h"
#include "ui/noteEditor.h"
#include "ui/meter.h"
#include "ui/waveform.h"

#include "audio/Codec.h"

const int cdef_bars             = 128; // 128 bars "is enough for everyone" // TODO: make dynamic

const int c_piano_min_width     = 110;
const int c_piano_min_height    = 40;
const int c_meter_min_height    = 20;
const int c_waveform_min_height = 50;
const int c_drawzone_min_height = 100;
const int c_dynbuttons_num      = 10;

const QString c_dynlbl_css_off = QString("QLabel { color : %1; }").arg(cdef_color_dynbtn_off);
const QString c_dynlbl_css_bg  = QString("QLabel { color : %1; }").arg(cdef_color_dynbtn_bg);
const QString c_dynlbl_css_fg  = QString("QLabel { color : %1; background-color : %2; }")
        .arg(cdef_color_dynbtn_on).arg(cdef_color_dynbtn_on_bg);


QSettings settings("QTau_Devgroup", c_qtau_name);

const QString c_key_dir_score   = QStringLiteral("last_score_dir");
const QString c_key_dir_audio   = QStringLiteral("last_audio_dir");
const QString c_key_win_size    = QStringLiteral("window_size");
const QString c_key_win_max     = QStringLiteral("window_fullscreen");
const QString c_key_show_lognum = QStringLiteral("show_new_log_number");
const QString c_key_dynpanel_on = QStringLiteral("dynamics_panel_visible");
const QString c_key_sound       = QStringLiteral("sould_level");
const QString c_key_audio_codec = QStringLiteral("save_audio_codec");

const QString c_doc_txt         = QStringLiteral(":/tr/documentation_en.txt");
const QString c_icon_app        = QStringLiteral(":/images/appicon_ouka_alice.png");
const QString c_icon_sound      = QStringLiteral(":/images/speaker.png");
const QString c_icon_mute       = QStringLiteral(":/images/speaker_mute.png");
const QString c_icon_editor     = QStringLiteral(":/images/b_notes.png");
const QString c_icon_voices     = QStringLiteral(":/images/b_mic.png");
const QString c_icon_plugins    = QStringLiteral(":/images/b_plug.png");
const QString c_icon_settings   = QStringLiteral(":/images/b_gear.png");
const QString c_icon_doc        = QStringLiteral(":/images/b_manual.png");
const QString c_icon_log        = QStringLiteral(":/images/b_envelope.png");
const QString c_icon_play       = QStringLiteral(":/images/b_play.png");
const QString c_icon_pause      = QStringLiteral(":/images/b_pause.png");


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    logNewMessages(0), logHasErrors(false), showNewLogNumber(true)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(c_icon_app));
    setWindowTitle(c_qtau_name);
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::NoContextMenu);

    //-----------------------------------------

    QLabel *meterLabel = new QLabel(QString("%1/%2") .arg(ns.notesInBar).arg(ns.noteLength), this);
    QLabel *tempoLabel = new QLabel(QString("%1 %2").arg(ns.tempo).arg(tr("bpm")),           this);

    QHBoxLayout *bpmHBL = new QHBoxLayout();
    bpmHBL->setContentsMargins(0,0,0,0);
    bpmHBL->addSpacing(5);
    bpmHBL->addWidget(meterLabel);
    bpmHBL->addWidget(tempoLabel);
    bpmHBL->addSpacing(5);

    QFrame *tempoPanel = new QFrame(this);
    tempoPanel->setMinimumSize(c_piano_min_width, c_meter_min_height);
    tempoPanel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    tempoPanel->setContentsMargins(1,0,1,1);
    tempoPanel->setFrameStyle(QFrame::Panel | QFrame::Raised);

    tempoPanel->setLayout(bpmHBL);

    meter = new qtauMeterBar(this);
    meter->setMinimumHeight(c_meter_min_height);
    meter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    meter->setContentsMargins(0,0,0,0);

    piano = new qtauPiano(ui->centralWidget);
    piano->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    piano->setMinimumSize(c_piano_min_width, c_piano_min_height);
    piano->setContentsMargins(0,0,0,0);

    zoom = new QSlider(Qt::Horizontal, ui->centralWidget);
    zoom->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    zoom->setRange(0, c_zoom_num - 1);
    zoom->setSingleStep(1);
    zoom->setPageStep(1);
    zoom->setValue(cdef_zoom_index);
    zoom->setMinimumWidth(c_piano_min_width);
    zoom->setGeometry(0,0,c_piano_min_width,10);
    zoom->setContentsMargins(0,0,0,0);

    noteEditor = new qtauNoteEditor(ui->centralWidget);
    noteEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    noteEditor->setContentsMargins(0,0,0,0);

    hscr = new QScrollBar(Qt::Horizontal, ui->centralWidget);
    vscr = new QScrollBar(Qt::Vertical,   ui->centralWidget);

    hscr->setContentsMargins(0,0,0,0);
    vscr->setContentsMargins(0,0,0,0);
    hscr->setRange(0, ns.note.width() * ns.notesInBar * cdef_bars);
    vscr->setRange(0, ns.note.height() * 12 * ns.numOctaves);
    hscr->setSingleStep(ns.note.width());
    vscr->setSingleStep(ns.note.height());
    hscr->setContextMenuPolicy(Qt::NoContextMenu);
    vscr->setContextMenuPolicy(Qt::NoContextMenu);

    //---- vocal and music waveform panels, hidden until synthesized (vocal wave) and/or loaded (music wave)

    QScrollBar *dummySB = new QScrollBar(this);
    dummySB->setOrientation(Qt::Vertical);
    dummySB->setRange(0,0);
    dummySB->setEnabled(false);

    QFrame *vocalControls = new QFrame(this);
    vocalControls->setContentsMargins(0,0,0,0);
    vocalControls->setMinimumSize(c_piano_min_width, c_waveform_min_height);
    vocalControls->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    vocalControls->setFrameStyle(QFrame::Panel | QFrame::Raised);

    vocalWave = new qtauWaveform(this);
    vocalWave->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    vocalWave->setMinimumHeight(c_waveform_min_height);
    vocalWave->setContentsMargins(0,0,0,0);

    QHBoxLayout *vocalWaveL = new QHBoxLayout();
    vocalWaveL->setContentsMargins(0,0,0,0);
    vocalWaveL->setSpacing(0);
    vocalWaveL->addWidget(vocalControls);
    vocalWaveL->addWidget(vocalWave);
    vocalWaveL->addWidget(dummySB);

    vocalWavePanel = new QWidget(this);
    vocalWavePanel->setContentsMargins(0,0,0,0);
    vocalWavePanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    vocalWavePanel->setLayout(vocalWaveL);
    vocalWavePanel->setVisible(false);

    //---------

    QScrollBar *dummySB2 = new QScrollBar(this);
    dummySB2->setOrientation(Qt::Vertical);
    dummySB2->setRange(0,0);
    dummySB2->setEnabled(false);

    QFrame *musicControls = new QFrame(this);
    musicControls->setContentsMargins(0,0,0,0);
    musicControls->setMinimumSize(c_piano_min_width, c_waveform_min_height);
    musicControls->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    musicControls->setFrameStyle(QFrame::Panel | QFrame::Raised);

    musicWave = new qtauWaveform(this);
    musicWave->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    musicWave->setMinimumHeight(c_waveform_min_height);
    musicWave->setContentsMargins(0,0,0,0);

    QHBoxLayout *musicWaveL = new QHBoxLayout();
    musicWaveL->setContentsMargins(0,0,0,0);
    musicWaveL->setSpacing(0);
    musicWaveL->addWidget(musicControls);
    musicWaveL->addWidget(musicWave);
    musicWaveL->addWidget(dummySB2);

    musicWavePanel = new QWidget(this);
    musicWavePanel->setContentsMargins(0,0,0,0);
    musicWavePanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    musicWavePanel->setLayout(musicWaveL);
    musicWavePanel->setVisible(false);

    //---- notes' dynamics setup area --------------

    QGridLayout *dynBtnL = new QGridLayout();

    QString btnNames[c_dynbuttons_num] = {"VEL", "DYN", "BRE", "BRI", "CLE", "OPE", "GEN", "POR", "PIT", "PBS"};

    for (int i = 0; i < c_dynbuttons_num; ++i)
    {
        qtauDynLabel *l = new qtauDynLabel(btnNames[i], this);
        l->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        dynBtnL->addWidget(l, i / 2, i % 2, 1, 1);

        l->setStyleSheet(c_dynlbl_css_off);
        l->setFrameStyle(QFrame::Box);
        l->setLineWidth(1);

        connect(l, SIGNAL(leftClicked()),  SLOT(dynBtnLClicked()));
        connect(l, SIGNAL(rightClicked()), SLOT(dynBtnRClicked()));
    }

    dynBtnL->setRowStretch(c_dynbuttons_num / 2, 100);

    QFrame *dynBtnPanel = new QFrame(this);
    dynBtnPanel->setContentsMargins(0,0,0,0);
    dynBtnPanel->setMinimumSize(c_piano_min_width, c_drawzone_min_height);
    dynBtnPanel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    dynBtnPanel->setFrameStyle(QFrame::Panel | QFrame::Raised);

    dynBtnPanel->setLayout(dynBtnL);

    drawZone = new qtauDynDrawer(ui->centralWidget);
    drawZone->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    drawZone->setMinimumHeight(c_drawzone_min_height);
    drawZone->setContentsMargins(0,0,0,0);

    QScrollBar *dummySB3 = new QScrollBar(this);
    dummySB3->setOrientation(Qt::Vertical);
    dummySB3->setRange(0,0);
    dummySB3->setEnabled(false);

    QHBoxLayout *singParamsL = new QHBoxLayout();
    singParamsL->setContentsMargins(0,0,0,0);
    singParamsL->setSpacing(0);
    singParamsL->addWidget(dynBtnPanel);
    singParamsL->addWidget(drawZone);
    singParamsL->addWidget(dummySB3);

    drawZonePanel = new QWidget(this);
    drawZonePanel->setContentsMargins(0,0,0,0);
    drawZonePanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    drawZonePanel->setLayout(singParamsL);

    //---- Combining editor panels into hi-level layout ------

    QGridLayout *gl = new QGridLayout();
    gl->setContentsMargins(0,0,0,0);
    gl->setSpacing(0);

    gl->addWidget(tempoPanel, 0, 0, 1, 1);
    gl->addWidget(meter,      0, 1, 1, 1);
    gl->addWidget(piano,      1, 0, 1, 1);
    gl->addWidget(zoom,       2, 0, 1, 1);
    gl->addWidget(noteEditor, 1, 1, 1, 1);
    gl->addWidget(hscr,       2, 1, 1, 1);
    gl->addWidget(vscr,       1, 2, 1, 1);

    QWidget *editorUpperPanel = new QWidget(this);
    editorUpperPanel->setContentsMargins(0,0,0,0);
    editorUpperPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    editorUpperPanel->setMaximumSize(9000,9000);

    editorUpperPanel->setLayout(gl);

    editorSplitter = new QSplitter(Qt::Vertical, this);
    editorSplitter->setContentsMargins(0,0,0,0);
    editorSplitter->addWidget(editorUpperPanel);
    editorSplitter->addWidget(vocalWavePanel);
    editorSplitter->addWidget(musicWavePanel);
    editorSplitter->addWidget(drawZonePanel);
    editorSplitter->setStretchFactor(0, 1);
    editorSplitter->setStretchFactor(1, 0);
    editorSplitter->setStretchFactor(2, 0);
    editorSplitter->setStretchFactor(3, 0);

    QVBoxLayout *edVBL = new QVBoxLayout();
    edVBL->setContentsMargins(0,0,0,0);
    edVBL->addWidget(editorSplitter);

    QWidget *editorPanel = new QWidget(this);
    editorPanel->setContentsMargins(0,0,0,0);
    editorPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    editorPanel->setMaximumSize(9000,9000);

    editorPanel->setLayout(edVBL);

    //---- Voicebank setup tab ---------------------

    QWidget *voicesPanel = new QWidget(this);
    voicesPanel->setContentsMargins(0,0,0,0);
    voicesPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    voicesPanel->setMaximumSize(9000,9000);

    //---- Plugins setup tab -----------------------

    QWidget *pluginsPanel = new QWidget(this);
    pluginsPanel->setContentsMargins(0,0,0,0);
    pluginsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pluginsPanel->setMaximumSize(9000,9000);

    //---- Settings tab ----------------------------

    QWidget *settingsPanel = new QWidget(this);
    settingsPanel->setContentsMargins(0,0,0,0);
    settingsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    settingsPanel->setMaximumSize(9000,9000);

    //---- Documentation tab -----------------------

    QWidget *docsPanel = new QWidget(this);
    docsPanel->setContentsMargins(0,0,0,0);
    docsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    docsPanel->setMaximumSize(9000,9000);

    QTextEdit *docpad = new QTextEdit(this);
    docpad->setReadOnly(true);
    docpad->setUndoRedoEnabled(false);
    docpad->setContextMenuPolicy(Qt::NoContextMenu);

    QFile embeddedDocTxt(c_doc_txt);

    if (embeddedDocTxt.open(QFile::ReadOnly))
    {
        QTextStream ts(&embeddedDocTxt);
        ts.setAutoDetectUnicode(true);
        ts.setCodec("UTF-8");

        docpad->setText(ts.readAll());
        embeddedDocTxt.close();
    }

    QGridLayout *docL = new QGridLayout();
    docL->setContentsMargins(0,0,0,0);
    docL->addWidget(docpad, 0, 0, 1, 1);

    docsPanel->setLayout(docL);

    //---- Log tab ---------------------------------

    QWidget *logPanel = new QWidget(this);
    logPanel->setContentsMargins(0,0,0,0);
    logPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    logPanel->setMaximumSize(9000,9000);

    logpad = new QTextEdit(this);
    logpad->setReadOnly(true);
    logpad->setUndoRedoEnabled(false);
    logpad->setContextMenuPolicy(Qt::NoContextMenu);
    logpad->setStyleSheet("p, pre { white-space: 1.2; }");

    QGridLayout *logL = new QGridLayout();
    logL->setContentsMargins(0,0,0,0);
    logL->addWidget(logpad, 0, 0, 1, 1);

    logPanel->setLayout(logL);

    //---- Combining tabs togeter ------------------

    tabs = new QTabWidget(this);
    tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tabs->setContentsMargins(0,0,0,0);
    tabs->setMaximumSize(9000, 9000);
    tabs->setTabPosition(QTabWidget::South);
    tabs->setMovable(false); // just to be sure

    tabs->addTab(editorPanel,   QIcon(c_icon_editor),   tr("Editor"));
    tabs->addTab(voicesPanel,   QIcon(c_icon_voices),   tr("Voices"));
    tabs->addTab(pluginsPanel,  QIcon(c_icon_plugins),  tr("Plugins"));
    tabs->addTab(settingsPanel, QIcon(c_icon_settings), tr("Settings"));
    tabs->addTab(docsPanel,     QIcon(c_icon_doc),      tr("Documentation"));
    tabs->addTab(logPanel,      QIcon(c_icon_log),      tr("Log"));

    tabs->widget(0)->setContentsMargins(0,0,0,0);
    tabs->widget(1)->setContentsMargins(0,0,0,0);
    tabs->widget(2)->setContentsMargins(0,0,0,0);
    tabs->widget(3)->setContentsMargins(0,0,0,0);
    tabs->widget(4)->setContentsMargins(0,0,0,0);
    tabs->widget(5)->setContentsMargins(0,0,0,0);

    logTabTextColor = tabs->tabBar()->tabTextColor(5);

    connect(tabs, SIGNAL(currentChanged(int)), SLOT(onTabSelected(int)));

    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->setContentsMargins(0,0,0,0);
    vbl->addWidget(tabs);
    ui->centralWidget->setContentsMargins(0,0,0,0);
    ui->centralWidget->setLayout(vbl);

    //---- Toolbars --------------------------------

    QToolBar *fileTB   = new QToolBar("Fileops",  this);
    QToolBar *playerTB = new QToolBar("Playback", this);
    QToolBar *toolsTB  = new QToolBar("Toolset",  this);

    fileTB  ->setFloatable(false);
    playerTB->setFloatable(false);
    toolsTB ->setFloatable(false);

    fileTB->addAction(ui->actionSave);
    fileTB->addAction(ui->actionSave_audio_as);
    fileTB->addAction(ui->actionUndo);
    fileTB->addAction(ui->actionRedo);

    playerTB->addAction(ui->actionPlay);
    playerTB->addAction(ui->actionStop);
    playerTB->addAction(ui->actionBack);
    playerTB->addAction(ui->actionRepeat);

    volume = new QSlider(Qt::Horizontal, this);
    volume->setMaximum(100);
    volume->setSingleStep(1);
    volume->setPageStep(1);
    volume->setValue(settings.value(c_key_sound, 50).toInt());
    volume->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    connect(playerTB, SIGNAL(orientationChanged(Qt::Orientation)), volume, SLOT(setOrientation(Qt::Orientation)));

    muteBtn = new QAction(QIcon(c_icon_sound), "", this);
    muteBtn->setCheckable(true);
    connect(muteBtn, SIGNAL(toggled(bool)), SLOT(onMute(bool)));

    playerTB->addWidget(volume);
    playerTB->addAction(muteBtn);

    QComboBox *quantizeCombo = new QComboBox(this);
    QComboBox *lengthCombo   = new QComboBox(this);
    quantizeCombo->addItems(QStringList() << "Q/4" << "Q/8" << "Q/16" << "Q/32" << "Q/64");
    lengthCombo  ->addItems(QStringList() << "♪/4" << "♪/8" << "♪/16" << "♪/32" << "♪/64");
    quantizeCombo->setCurrentIndex(3);
    lengthCombo  ->setCurrentIndex(3);

    toolsTB->addAction(ui->actionEdit_Mode);
    toolsTB->addAction(ui->actionGrid_Snap);
    toolsTB->addSeparator();
    toolsTB->addWidget(quantizeCombo);
    toolsTB->addWidget(lengthCombo);

    addToolBar(fileTB);
    addToolBar(playerTB);
    addToolBar(toolsTB);

    toolbars.append(fileTB);
    toolbars.append(playerTB);
    toolbars.append(toolsTB);

    //----------------------------------------------
    connect(quantizeCombo, SIGNAL(currentIndexChanged(int)), SLOT(onQuantizeSelected(int)));
    connect(lengthCombo,   SIGNAL(currentIndexChanged(int)), SLOT(onNotelengthSelected(int)));

    connect(vsLog::instance(), &vsLog::message,             this, &MainWindow::onLog);

    connect(piano,      &qtauPiano      ::heightChanged,    this, &MainWindow::onPianoHeightChanged);
    connect(noteEditor, &qtauNoteEditor ::widthChanged,     this, &MainWindow::onNoteEditorWidthChanged);

    connect(meter,      &qtauMeterBar   ::scrolled,         this, &MainWindow::notesHScrolled);
    connect(piano,      &qtauPiano      ::scrolled,         this, &MainWindow::notesVScrolled);
    connect(drawZone,   &qtauDynDrawer  ::scrolled,         this, &MainWindow::notesHScrolled);
    connect(noteEditor, &qtauNoteEditor ::vscrolled,        this, &MainWindow::notesVScrolled);
    connect(noteEditor, &qtauNoteEditor ::hscrolled,        this, &MainWindow::notesHScrolled);
    connect(vocalWave,  &qtauWaveform   ::scrolled,         this, &MainWindow::notesHScrolled);
    connect(musicWave,  &qtauWaveform   ::scrolled,         this, &MainWindow::notesHScrolled);
    connect(vscr,       &QScrollBar     ::valueChanged,     this, &MainWindow::vertScrolled);
    connect(hscr,       &QScrollBar     ::valueChanged,     this, &MainWindow::horzScrolled);

    connect(noteEditor, &qtauNoteEditor ::rmbScrolled,      this, &MainWindow::onEditorRMBScrolled);
    connect(noteEditor, &qtauNoteEditor ::requestsOffset,   this, &MainWindow::onEditorRequestOffset);

    connect(zoom,       &QSlider        ::valueChanged,     this, &MainWindow::onZoomed);
    connect(meter,      &qtauMeterBar   ::zoomed,           this, &MainWindow::onEditorZoomed);
    connect(noteEditor, &qtauNoteEditor ::zoomed,           this, &MainWindow::onEditorZoomed);
    connect(drawZone,   &qtauDynDrawer  ::zoomed,           this, &MainWindow::onEditorZoomed);
    connect(vocalWave,  &qtauWaveform   ::zoomed,           this, &MainWindow::onEditorZoomed);
    connect(musicWave,  &qtauWaveform   ::zoomed,           this, &MainWindow::onEditorZoomed);

    connect(ui->actionQuit,      &QAction::triggered, [=]() { this->close(); });
    connect(ui->actionOpen,      &QAction::triggered, this, &MainWindow::onOpenUST);
    connect(ui->actionSave,      &QAction::triggered, this, &MainWindow::onSaveUST);
    connect(ui->actionSave_as,   &QAction::triggered, this, &MainWindow::onSaveUSTAs);

    connect(ui->actionUndo,      &QAction::triggered, this, &MainWindow::onUndo);
    connect(ui->actionRedo,      &QAction::triggered, this, &MainWindow::onRedo);
    connect(ui->actionDelete,    &QAction::triggered, this, &MainWindow::onDelete);

    connect(ui->actionEdit_Mode, &QAction::triggered, this, &MainWindow::onEditMode);
    connect(ui->actionGrid_Snap, &QAction::triggered, this, &MainWindow::onGridSnap);

    connect(ui->actionSave_audio_as, &QAction::triggered, this, &MainWindow::onSaveAudioAs);

    //----------------------------------------------

    lastScoreDir     = settings.value(c_key_dir_score,   "").toString();
    lastAudioDir     = settings.value(c_key_dir_audio,   "").toString();
    audioExt         = settings.value(c_key_audio_codec, "").toString();
    showNewLogNumber = settings.value(c_key_show_lognum, true).toBool();

    if (!settings.value(c_key_dynpanel_on, true).toBool())
    {
        QList<int> panelSizes = editorSplitter->sizes();
        panelSizes.last() = 0;
        editorSplitter->setSizes(panelSizes);
    }

    if (settings.value(c_key_win_max, false).toBool())
        showMaximized();
    else
    {
        QRect winGeom = geometry();
        QRect setGeom = settings.value(c_key_win_size, QRect(winGeom.topLeft(), minimumSize())).value<QRect>();

        if (setGeom.width() >= winGeom.width() && setGeom.height() >= setGeom.height())
            setGeometry(setGeom);
    }

    //----------------------------------------------

    vsLog::instance()->enableHistory(false);
    onLog(QString("\t%1 %2 @ %3").arg(tr("Launching QTau")).arg(c_qtau_ver).arg(__DATE__), ELog::success);

    onLog("\t---------------------------------------------", ELog::info);
    vsLog::r(); // print stored messages from program startup
    onLog("\t---------------------------------------------", ELog::info);
    vsLog::n();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // store settings
    settings.setValue(c_key_dir_score,   lastScoreDir);
    settings.setValue(c_key_dir_audio,   lastAudioDir);
    settings.setValue(c_key_win_size,    geometry());
    settings.setValue(c_key_win_max,     isMaximized());
    settings.setValue(c_key_show_lognum, showNewLogNumber);
    settings.setValue(c_key_sound,       volume->value());
    settings.setValue(c_key_audio_codec, audioExt);
    settings.setValue(c_key_dynpanel_on, editorSplitter->sizes().last() > 0);

    event->accept();
}

MainWindow::~MainWindow() { delete ui; }

//========================================================================================


bool MainWindow::setController(qtauController &c, qtauSession &s)
{
    // NOTE: bind what uses qtauSession only here (menu/toolbar button states etc)
    doc = &s;

    connect(noteEditor, &qtauNoteEditor::editorEvent, doc, &qtauSession::onUIEvent      );

    connect(doc, &qtauSession::dataReloaded,   this, &MainWindow::onDocReloaded         );
    connect(doc, &qtauSession::modifiedStatus, this, &MainWindow::onDocStatus           );
    connect(doc, &qtauSession::undoStatus,     this, &MainWindow::onUndoStatus          );
    connect(doc, &qtauSession::redoStatus,     this, &MainWindow::onRedoStatus          );

    connect(doc, &qtauSession::vocalSet,       this, &MainWindow::onVocalAudioChanged   );
    connect(doc, &qtauSession::musicSet,       this, &MainWindow::onMusicAudioChanged   );

    connect(doc, &qtauSession::onEvent,        this, &MainWindow::onDocEvent            );
    connect(doc, &qtauSession::playbackStateChanged, this, &MainWindow::onPlaybackState );

    connect(ui->actionPlay,   &QAction::triggered, doc, &qtauSession::startPlayback );
    connect(ui->actionStop,   &QAction::triggered, doc, &qtauSession::stopPlayback  );
    connect(ui->actionBack,   &QAction::triggered, doc, &qtauSession::resetPlayback );
    connect(ui->actionRepeat, &QAction::triggered, doc, &qtauSession::repeatPlayback);

    connect(this,   &MainWindow::loadUST,      &c, &qtauController::onLoadUST       );
    connect(this,   &MainWindow::saveUST,      &c, &qtauController::onSaveUST       );
    connect(this,   &MainWindow::saveAudio,    &c, &qtauController::onSaveAudio     );

    connect(this,   &MainWindow::loadAudio,    &c, &qtauController::onLoadAudio     );
    connect(volume, &QSlider   ::valueChanged, &c, &qtauController::onVolumeChanged );
    connect(this,   &MainWindow::setVolume,    &c, &qtauController::onVolumeChanged );

    connect(piano, &qtauPiano::keyPressed,     &c, &qtauController::pianoKeyPressed );
    connect(piano, &qtauPiano::keyReleased,    &c, &qtauController::pianoKeyReleased);
    //-----------------------------------------------------------------------

    c.onVolumeChanged(volume->value());

    // widget configuration - maybe read app settings here?
    noteEditor->setRMBScrollEnabled(!ui->actionEdit_Mode->isChecked());
    noteEditor->setEditingEnabled  ( ui->actionEdit_Mode->isChecked());
    noteEditor->setFocus();

    return true;
}

void MainWindow::onOpenUST()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open UST"), lastScoreDir, tr("UTAU Sequence Text Files (*.ust)"));

    if (!fileName.isEmpty())
    {
        lastScoreDir = QFileInfo(fileName).absolutePath();
        emit loadUST(fileName);
    }
}

void MainWindow::onSaveUST()
{
    if (doc->documentFile().isEmpty())
        onSaveUSTAs();
    else
        emit saveUST(doc->documentFile(), true);
}

void MainWindow::onSaveUSTAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save UST"), lastScoreDir, tr("UTAU Sequence Text Files (*.ust)"));

    if (!fileName.isEmpty())
    {
        lastScoreDir = QFileInfo(fileName).absolutePath();
        emit saveUST(fileName, true);
    }
}

void MainWindow::onSaveAudioAs()
{
    QList<QString> codecs = qtauCodecRegistry::instance()->listCodecs(audioExt);

    if (!codecs.isEmpty())
    {
        QString codecStr;

        foreach (const QString &c, codecs)
            codecStr.append(";;" + c);

        codecStr.remove(0,2);

        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save audio"), lastAudioDir, codecStr);

        if (!fileName.isEmpty())
        {
            QFileInfo fi(fileName);
            audioExt     = fi.suffix();
            lastAudioDir = fi.absolutePath();

            emit saveAudio(fileName, true);
        }
    }
    else vsLog::e(tr("Can't save audio because no audio codecs. At all. This shouldn't happen!"));
}

void MainWindow::notesVScrolled(int delta)
{
    if (delta > 0 && vscr->value() > 0) // scroll up
        delta = -ns.note.height();
    else if (delta < 0 && vscr->value() < vscr->maximum()) // scroll down
        delta = ns.note.height();
    else
        delta = 0;

    if (delta != 0)
        vscr->setValue(vscr->value() + delta);
}

void MainWindow::notesHScrolled(int delta)
{
    if (delta > 0 && hscr->value() > 0) // scroll left
        delta = -ns.note.width();
    else if (delta < 0 && hscr->value() < hscr->maximum()) // scroll right
        delta = ns.note.width();
    else
        delta = 0;

    if (delta != 0)
        hscr->setValue(hscr->value() + delta);
}

void MainWindow::vertScrolled(int delta)
{
    piano->setOffset(delta);
    noteEditor->setVOffset(delta);
}

void MainWindow::horzScrolled(int delta)
{
    noteEditor->setHOffset(delta);
    meter     ->setOffset (delta);
    drawZone  ->setOffset (delta);

    if (vocalWavePanel->isVisible())
        vocalWave->setOffset(delta);

    if (musicWavePanel->isVisible())
        musicWave->setOffset(delta);
}

void MainWindow::onEditorRMBScrolled(QPoint mouseDelta, QPoint origOffset)
{
    // moving editor space in reverse of mouse delta
    int hOff = qMax(qMin(origOffset.x() - mouseDelta.x(), hscr->maximum()), 0);
    int vOff = qMax(qMin(origOffset.y() - mouseDelta.y(), vscr->maximum()), 0);

    hscr->setValue(hOff);
    vscr->setValue(vOff);
}

void MainWindow::onEditorRequestOffset(QPoint off)
{
    off.setX(qMax(qMin(off.x(), hscr->maximum()), 0));
    off.setY(qMax(qMin(off.y(), vscr->maximum()), 0));

    hscr->setValue(off.x());
    vscr->setValue(off.y());
}

void MainWindow::onPianoHeightChanged(int newHeight)
{
    vscr->setMaximum(ns.note.height() * 12 * ns.numOctaves - newHeight + 1);
    vscr->setPageStep(piano->geometry().height());
}

void MainWindow::onNoteEditorWidthChanged(int newWidth)
{
    hscr->setMaximum(ns.note.width() * ns.notesInBar * cdef_bars - newWidth + 1);
    hscr->setPageStep(noteEditor->geometry().width());
}

void MainWindow::onPlaybackState(EAudioPlayback state)
{
    switch (state)
    {
    case EAudioPlayback::noAudio:
    case EAudioPlayback::needsSynth:
        ui->actionPlay->setText(tr("Play"));
        ui->actionPlay->setIcon(QIcon(c_icon_play));
        ui->actionStop->setEnabled(false);
        ui->actionBack->setEnabled(false);
        ui->actionRepeat->setEnabled(false);
        ui->actionSave_audio_as->setEnabled(false);

        ui->actionPlay->setChecked(false);
        ui->actionRepeat->setChecked(false);

        ui->actionPlay->setEnabled(state == EAudioPlayback::needsSynth);
        break;

    case EAudioPlayback::repeating: // this and following states imply that actions were enabled before
    case EAudioPlayback::playing:
        ui->actionPlay->setIcon(QIcon(c_icon_pause));
        ui->actionPlay->setText(tr("Pause"));
        ui->actionStop->setEnabled(true);
        ui->actionBack->setEnabled(true);
        ui->actionRepeat->setEnabled(true);
        ui->actionSave_audio_as->setEnabled(true);
        break;

    case EAudioPlayback::stopped:
        ui->actionPlay->setChecked(false);
        ui->actionRepeat->setChecked(false);
    case EAudioPlayback::paused:
        ui->actionPlay->setIcon(QIcon(c_icon_play));
        ui->actionPlay->setText(tr("Play"));
        ui->actionSave_audio_as->setEnabled(true);
        break;

    default:
        vsLog::e(QString("Window got unknown session playback state %1").arg((char)state));
    }
}

void MainWindow::onUndo()
{
    if (doc->canUndo())
        doc->undo();
    else
        ui->actionUndo->setEnabled(false);
}

void MainWindow::onRedo()
{
    if (doc->canRedo())
        doc->redo();
    else
        ui->actionRedo->setEnabled(false);
}

void MainWindow::onDelete()
{
    noteEditor->deleteSelected();
}

void MainWindow::onEditMode(bool toggled)
{
    noteEditor->setEditingEnabled  ( toggled);
    noteEditor->setRMBScrollEnabled(!toggled);
}

void MainWindow::onGridSnap(bool toggled)
{
    noteEditor->setGridSnapEnabled(toggled);
}

void MainWindow::onQuantizeSelected(int index)
{
    int newQuant = 4 * (int)(pow(2, index) + 0.001);

    if (newQuant != ns.quantize)
    {
        ns.quantize = newQuant;
        noteEditor->configure(ns);
    }
}

void MainWindow::onNotelengthSelected(int index)
{
    int newNoteLength = 4 * (int)(pow(2, index) + 0.001);

    if (newNoteLength != ns.length)
    {
        ns.length = newNoteLength;
        noteEditor->configure(ns);
    }
}

void MainWindow::dynBtnLClicked()
{
    qtauDynLabel* l= qobject_cast<qtauDynLabel*>(sender());

    if (l && (fgDynLbl == 0 || l != fgDynLbl))
    {
        if (fgDynLbl)
        {
            fgDynLbl->setState(qtauDynLabel::off);
            fgDynLbl->setStyleSheet(c_dynlbl_css_off);
        }

        if (l == bgDynLbl)
        {
            bgDynLbl->setState(qtauDynLabel::off);
            bgDynLbl->setStyleSheet(c_dynlbl_css_off);
            bgDynLbl = 0;
        }

        l->setStyleSheet(c_dynlbl_css_fg);
        fgDynLbl = l;
    }
}

void MainWindow::dynBtnRClicked()
{
    qtauDynLabel* l= qobject_cast<qtauDynLabel*>(sender());

    if (l)
    {
        if (bgDynLbl != 0 && l == bgDynLbl)
        {
            // clicking on same dynkey - switch it off
            bgDynLbl->setState(qtauDynLabel::off);
            bgDynLbl->setStyleSheet(c_dynlbl_css_off);
            bgDynLbl = 0;
        }
        else
        {   // clicking on other dynkey
            if (bgDynLbl)
            {   // switch off previous one, if any
                bgDynLbl->setState(qtauDynLabel::off);
                bgDynLbl->setStyleSheet(c_dynlbl_css_off);
                bgDynLbl = 0;
            }

            if (l != fgDynLbl)
            {   // clicking on not-foreground dynkey
                l->setStyleSheet(c_dynlbl_css_bg);
                bgDynLbl = l;
            }
        }
    }
}

void MainWindow::onLog(const QString &msg, ELog type)
{
    QString color = "black";
    bool viewingLog = tabs->currentIndex() == tabs->count() - 1;

    switch(type)
    {
    case ELog::error:
        color = "red";
        break;
    case ELog::success:
        color = "green";
        break;
    default: break;
    }

    if (!viewingLog)
    {
        QTabBar *tb = const_cast<QTabBar *>(tabs->tabBar()); // dirty hack I know, but no other way atm

        if (showNewLogNumber)
        {
            tb->setTabText(tb->count() - 1, tr("Log") + QString(" (%1)").arg(logNewMessages));
            logNewMessages++;
        }

        if (type == ELog::error)
        {
            tb->setTabTextColor(tb->count() - 1, QColor(cdef_color_logtab_err));
            logHasErrors = true;
        }
    }

    logpad->moveCursor(QTextCursor::End);
    logpad->insertHtml(QString("<pre style=\"color: %1;\">%2</pre><p></p>").arg(color).arg(msg));
}

void MainWindow::enableToolbars(bool enable)
{
    foreach (QToolBar *t, toolbars)
        t->setVisible(enable);      //t->setEnabled(enable);
}

void MainWindow::onTabSelected(int index)
{
    enableToolbars(index == 0);

    if (index == tabs->count() - 1)
    {
        QTabBar *tb = const_cast<QTabBar *>(tabs->tabBar());

        if (logNewMessages > 0)
        {
            tb->setTabText(tb->count() - 1, tr("Log"));
            logNewMessages = 0;
        }

        if (logHasErrors)
        {
            tb->setTabTextColor(tb->count() - 1, logTabTextColor); // set default color back
            logHasErrors = false;
        }
    }
}

void MainWindow::onZoomed(int z)
{
    // modify note data and send it to widgets
    ns.note.setWidth(c_zoom_note_widths[z]);

    meter     ->configure(ns);
    piano     ->configure(ns);
    noteEditor->configure(ns);
    drawZone  ->configure(ns);

    if (vocalWavePanel->isVisible())
        vocalWave->configure(ns.tempo, ns.note.width());

    if (musicWavePanel->isVisible())
        musicWave->configure(ns.tempo, ns.note.width());

    // modify scrollbar sizes and position
    double hscr_val = (double)hscr->value() / hscr->maximum();
    hscr->setMaximum(ns.note.width()  * ns.notesInBar * cdef_bars - noteEditor->width() + 1);
    hscr->setValue(hscr->maximum() * hscr_val);
    hscr->setSingleStep(ns.note.width());

    horzScrolled(hscr->value());
}

void MainWindow::onEditorZoomed(int delta)
{
    if (delta != 0)
        if ((delta > 0 && zoom->value() >= 0) ||
            (delta < 0 && zoom->value() < c_zoom_num))
            zoom->setValue(zoom->value() + ((delta > 0) ? 1 : -1));
}

void MainWindow::onDocReloaded()
{
    setWindowTitle(doc->documentName() + " - QTau");
}

void MainWindow::onDocStatus(bool isModified)
{
    QString newDocName = doc->documentName();

    if (docName != newDocName)
        docName = newDocName;

    setWindowTitle((isModified ? "*" : "") + docName + " - QTau");

    ui->actionSave->setEnabled(isModified);
}

void MainWindow::onUndoStatus(bool canUndo)
{
    ui->actionUndo->setEnabled(canUndo);
}

void MainWindow::onRedoStatus(bool canRedo)
{
    ui->actionRedo->setEnabled(canRedo);
}

void MainWindow::onDocEvent(qtauEvent* event)
{
    if (event->type() >= ENoteEvents::add && event->type() <= ENoteEvents::effect)
        noteEditor->onEvent(event);
}

void MainWindow::onVocalAudioChanged()
{
    // show vocal waveform panel and send audioSource to it for generation
    vocalWavePanel->setVisible(true);
    vocalWave->setAudio(doc->getVocal().vocalWave);
}

void MainWindow::onMusicAudioChanged()
{
    // show & fill music waveform panel
    musicWavePanel->setVisible(true);
    musicWave->setAudio(doc->getMusic().musicWave);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // accepting filepaths
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    // accepting filepaths
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> uris;

    foreach (const QByteArray &uriData, event->mimeData()->data("text/uri-list").split('\n'))
        if (!uriData.isEmpty())
            uris << QUrl::fromEncoded(uriData).toLocalFile().remove('\r');

    if (!uris.isEmpty())
    {
        QFileInfo fi(uris.first().toString());

        if (uris.size() > 1)
            vsLog::i("Multiple URIs dropped, currently using only first one");

        if (fi.exists() && !fi.isDir() && !fi.suffix().isEmpty()) // if it's an existing file with some extension
        {
            // maybe it's a note/lyrics file? (ust/vsq/vsqx/midi)
            if (fi.suffix() == "ust") // TODO: support many, or do something like audio codecs registry
            {
                emit loadUST(fi.absoluteFilePath());
            }
            else if (isAudioExtSupported(fi.suffix())) // or maybe it's an audio? (try audio codecs)
            {
                emit loadAudio(fi.absoluteFilePath());
            }
            else
                vsLog::e("File extension not supported: " + fi.suffix());
        }
    }
}

void MainWindow::onMute(bool m)
{
    volume->setEnabled(!m);

    if (m) muteBtn->setIcon(QIcon(c_icon_mute));
    else   muteBtn->setIcon(QIcon(c_icon_sound));

    emit setVolume(m ? 0 : volume->value());
}
