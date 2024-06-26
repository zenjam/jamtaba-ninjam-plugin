#ifndef JAMTABA_VST_PLUGIN_H
#define JAMTABA_VST_PLUGIN_H

#define uniqueIDEffect CCONST('V', 'b', 'P', 'l')
#define uniqueIDInstrument CCONST('V', 'b', 'I', 's')
#define VST_EVENT_BUFFER_SIZE 1000
#define DEFAULT_INPUTS 2
#define DEFAULT_OUTPUTS 1

#include "JamTabaPlugin.h" // base plugin class

#include "audioeffectx.h"
#include "aeffectx.h"

AudioEffect *createEffectInstance(audioMasterCallback audioMaster);

class JamTabaVSTPlugin : public JamTabaPlugin, public AudioEffectX
{

public:
    explicit JamTabaVSTPlugin (audioMasterCallback audioMaster);
    ~JamTabaVSTPlugin();

    //void initialize();    // called first time editor is opened
    VstInt32 canDo(char *text) override;
    void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) override;

    VstInt32 fxIdle() override;
    bool needIdle() override;

    QString getHostName() override;

    bool getEffectName(char *name) override;
    bool getVendorString(char *text) override;
    bool getProductString(char *text) override;
    VstInt32 getVendorVersion() override;

    VstInt32 getNumMidiInputChannels() override;
    VstInt32 getNumMidiOutputChannels() override;

    void open() override;
    void close() override;
    void setSampleRate(float sampleRate) override;
    float getSampleRate() const override;

    inline VstPlugCategory getPlugCategory() override;

    void suspend() override;
    void resume() override;

    int getHostBpm() const override;

protected:
    char programName[kVstMaxProgNameLen + 1];
    VstEvents *listEvnts;

    VstTimeInfo *timeInfo;

    qint32 getStartPositionForHostSync() const override;
    bool hostIsPlaying() const override;

    MainControllerPlugin *createPluginMainController(const persistence::Settings &settings, JamTabaPlugin *plugin) const override;
};


VstPlugCategory JamTabaVSTPlugin::getPlugCategory()
{
    return kPlugCategEffect;
}

#endif
