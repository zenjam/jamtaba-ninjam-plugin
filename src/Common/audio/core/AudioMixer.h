#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <QList>
#include <QMutex>
#include <QMap>
#include <QScopedPointer>
#include <QSharedPointer>
#include "audio/SamplesBufferResampler.h"

namespace midi {
class MidiMessage;
}

namespace audio {

class AudioNode;
class SamplesBuffer;
class LocalInputNode;

class AudioMixer
{

private:
    AudioMixer(const AudioMixer &other);

public:
    explicit AudioMixer(int sampleRate);
    ~AudioMixer();
    void process(const SamplesBuffer &in, SamplesBuffer &out, int sampleRate, const std::vector<midi::MidiMessage> &midiBuffer, bool attenuateAfterSumming = false);
    void addNode(QSharedPointer<AudioNode> node);
    void removeNode(QSharedPointer<AudioNode> node);

    void setSampleRate(int newSampleRate);

private:
    QList<QSharedPointer<AudioNode>> nodes;
    int sampleRate;
    QMap<QSharedPointer<AudioNode>, SamplesBufferResampler> resamplers;

};

inline void AudioMixer::setSampleRate(int newSampleRate)
{
    sampleRate = newSampleRate;
}

} // namespace

#endif
