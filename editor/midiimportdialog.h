#ifndef MIDIIMPORTDIALOG_H
#define MIDIIMPORTDIALOG_H

#include <QDialog>

#include <smf.h>

namespace Ui {
class MidiImportDialog;
}

class MidiImportDialog : public QDialog
{
    Q_OBJECT
    smf_t* smf;
    
public:
    explicit MidiImportDialog(QWidget *parent,QString filename);
    ~MidiImportDialog();
    
private slots:
    void on_buttonBox_accepted();

signals:
    void midiImported();


private:
    Ui::MidiImportDialog *ui;
};

#endif // MIDIIMPORTDIALOG_H
