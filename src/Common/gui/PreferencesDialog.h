#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>
#include "persistence/Settings.h"

namespace Ui {
class IODialog;
}

// This class is the base class for StandalonePreferencesDialog and VstPreferencesDialog

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    PreferencesDialog(QWidget *parent);
    virtual ~PreferencesDialog();

    virtual void initialize(int initialTab, const Persistence::Settings *settings);

    enum TAB {
        TAB_AUDIO, TAB_MIDI, TAB_VST, TAB_RECORDING, TAB_METRONOME
    };

signals:
    void recordingPathSelected(const QString &newRecordingPath);
    void metronomePrimaryBeatAudioFileSelected(const QString &filePath);
    void metronomeSecondaryBeatAudioFileSelected(const QString &filePath);
    void multiTrackRecordingStatusChanged(bool recording);

protected slots:
    virtual void selectTab(int index) = 0;

private slots:
    void selectRecordingPath();
    void selectPrimaryBeatAudioFile();
    void selectSecondaryBeatAudioFile();

    void refreshCustomMetronomeControlsStyleSheet();
private:
    QString selectAudioFile(QString caption, QString initialDir);

protected:
    Ui::IODialog *ui;

    // recording
    void populateRecordingTab();
    void selectRecordingTab();

    virtual void setupSignals();
    virtual void populateAllTabs();

    const Persistence::Settings *settings;

};

#endif
