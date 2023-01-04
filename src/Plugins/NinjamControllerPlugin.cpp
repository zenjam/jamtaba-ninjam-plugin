#include "NinjamControllerPlugin.h"
#include "MainControllerPlugin.h"

#include "audio/MetronomeTrackNode.h"
#include "audio/NinjamTrackNode.h"

NinjamControllerPlugin::NinjamControllerPlugin(MainControllerPlugin *controller) :
    NinjamController(controller),
    controller(controller),
    waitingForHostSync(false)
{
    //
}

void NinjamControllerPlugin::stopAndWaitForHostSync()
{
    if (!waitingForHostSync) {
        waitingForHostSync = true;

        reset(); // discard the intervals but keep the most recent

        deactivateAudioNodes(); // metronome and ninjam audio nodes will not be rendered
    }
}

void NinjamControllerPlugin::deactivateAudioNodes()
{
    metronomeTrackNode->deactivate();

    for (auto node : trackNodes)
        node->deactivate();

    mainController->setAllLoopersStatus(false); // deactivate all loopers
}

void NinjamControllerPlugin::activateAudioNodes()
{
    metronomeTrackNode->activate();

    for (auto node : trackNodes)
        node->activate();

    mainController->setAllLoopersStatus(true); // activate all loopers
}

void NinjamControllerPlugin::disableHostSync()
{
    waitingForHostSync = false;
    activateAudioNodes();
}

void NinjamControllerPlugin::startSynchronizedWithHost(qint32 startPosition)
{
    if (waitingForHostSync){
        waitingForHostSync = false;
        if (startPosition >= 0)
            intervalPosition = startPosition % samplesInInterval;
        else
            intervalPosition = samplesInInterval - qAbs(startPosition % samplesInInterval);

        activateAudioNodes();
    }
}

void NinjamControllerPlugin::process(const audio::SamplesBuffer &in, audio::SamplesBuffer &out, int sampleRate)
{
    if (!waitingForHostSync)
        NinjamController::process(in, out, sampleRate);
    else
        controller->doAudioProcess(in, out, sampleRate); // will process input only, ninjam related audio nodes will not be rendered, interval will not progress, etc.
}
