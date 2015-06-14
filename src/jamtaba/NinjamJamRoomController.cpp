#include "NinjamJamRoomController.h"
#include "MainController.h"
#include "audio/core/AudioDriver.h"
#include "ninjam/Service.h"
#include "ninjam/User.h"
#include "ninjam/Server.h"
#include "../NinjamRoomWindow.h"
#include "../audio/NinjamTrackNode.h"

#include <QDebug>

using namespace Controller;

NinjamJamRoomController::NinjamJamRoomController(Controller::MainController* mainController)
    :mainController(mainController),
    metronomeTrackNode(new Audio::MetronomeTrackNode(":/click.wav", mainController->getAudioDriver()->getSampleRate()))
{
    running = false;


}

NinjamJamRoomController::~NinjamJamRoomController()
{
    delete metronomeTrackNode;
}

void NinjamJamRoomController::start(const Ninjam::Server& server){
    //schedule an update in internal attributes
    newBpi = server.getBpi();
    newBpm = server.getBpm();
    processScheduledChanges();

    if(!running){
        mainController->addTrack(-1, this->metronomeTrackNode);

        this->intervalPosition  = 0;
        this->running = true;

        Ninjam::Service* ninjamService = Ninjam::Service::getInstance();
        QObject::connect(ninjamService, SIGNAL(serverBpmChanged(short)), this, SLOT(ninjamServerBpmChanged(short)));
        QObject::connect(ninjamService, SIGNAL(serverBpiChanged(short,short)), this, SLOT(ninjamServerBpiChanged(short,short)));
        QObject::connect(ninjamService, SIGNAL(audioIntervalPartAvailable(Ninjam::User,int,QByteArray,bool)), this, SLOT(ninjamAudioAvailable(Ninjam::User,int,QByteArray,bool)));

        //desconectar esses no destrutor
        QObject::connect(ninjamService, SIGNAL(userChannelCreated(Ninjam::User, Ninjam::UserChannel)), this, SLOT(ninjamUserChannelCreated(Ninjam::User, Ninjam::UserChannel)));
        QObject::connect(ninjamService, SIGNAL(userChannelRemoved(Ninjam::User, Ninjam::UserChannel)), this, SLOT(ninjamUserChannelRemoved(Ninjam::User, Ninjam::UserChannel)));
        QObject::connect(ninjamService, SIGNAL(userChannelUpdated(Ninjam::User, Ninjam::UserChannel)), this, SLOT(ninjamUserChannelUpdated(Ninjam::User, Ninjam::UserChannel)));

        //add server users
        QList<Ninjam::User*> users = server.getUsers();
        foreach (Ninjam::User* user, users) {
            foreach (Ninjam::UserChannel* channel, user->getChannels()) {
                addTrack(*user, *channel);
            }
        }

    }
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
long NinjamJamRoomController::generateNewTrackID(){
    static long TRACK_IDS = 0;
    long newID = TRACK_IDS;
    TRACK_IDS++;
    return newID;
}

QString NinjamJamRoomController::getUniqueKey(Ninjam::UserChannel channel){
    return channel.getUserFullName() + QString::number(channel.getIndex());
}

void NinjamJamRoomController::addTrack(Ninjam::User user, Ninjam::UserChannel channel){
    NinjamTrackNode* trackNode = new NinjamTrackNode(generateNewTrackID());
    trackNodes.insert(getUniqueKey(channel), trackNode);
    mainController->addTrack(trackNode->getID(), trackNode);
    emit channelAdded(user,  channel, trackNode->getID());
}

void NinjamJamRoomController::removeTrack(Ninjam::User user, Ninjam::UserChannel channel){
    QString uniqueKey = getUniqueKey(channel);
    if(trackNodes.contains(uniqueKey)){
        NinjamTrackNode* trackNode = trackNodes[uniqueKey];
        mainController->removeTrack(trackNode->getID());
        trackNodes.remove(uniqueKey);
        emit channelRemoved(user, channel, trackNode->getID());
    }
    else{
        qDebug() << " não encontrou o channel em tracNodes";

    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void NinjamJamRoomController::voteBpi(int bpi){
    Ninjam::Service::getInstance()->voteToChangeBPI(bpi);
}

void NinjamJamRoomController::voteBpm(int bpm){
    Ninjam::Service::getInstance()->voteToChangeBPM(bpm);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void NinjamJamRoomController::setMetronomeBeatsPerAccent(int beatsPerAccent){
    metronomeTrackNode->setBeatsPerAccent(beatsPerAccent);
}

void NinjamJamRoomController::stop(){
    if(running){
        this->running = false;
        mainController->removeTrack(-1);//remove metronome
        Ninjam::Service* ninjamService = Ninjam::Service::getInstance();
        QObject::disconnect(ninjamService, SIGNAL(serverBpmChanged(short)), this, SLOT(ninjamServerBpmChanged(short)));
        QObject::disconnect(ninjamService, SIGNAL(serverBpiChanged(short,short)), this, SLOT(ninjamServerBpiChanged(short,short)));
        QObject::disconnect(ninjamService, SIGNAL(audioIntervalPartAvailable(Ninjam::User,int,QByteArray,bool)), this, SLOT(ninjamAudioAvailable(Ninjam::User,int,QByteArray,bool)));
    }
}


void NinjamJamRoomController::process(Audio::SamplesBuffer &in, Audio::SamplesBuffer &out){
    int totalSamplesToProcess = out.getFrameLenght();
    int samplesProcessed = 0;
    static int lastBeat = 0;
    int offset = 0;
    int outLenght = out.getFrameLenght();
    do{
        int samplesToProcessInThisStep = std::min((int)(samplesInInterval - intervalPosition), outLenght - offset);
        out.setOffset(offset);
        out.setFrameLenght(samplesToProcessInThisStep);

        if(this->intervalPosition == 0){//starting new interval
            if(hasScheduledChanges()){
                processScheduledChanges();
            }
            foreach (NinjamTrackNode* track, trackNodes.values()) {
                track->startNewInterval();
            }
        }
        metronomeTrackNode->setIntervalPosition(this->intervalPosition);

        int currentBeat = intervalPosition / getSamplesPerBeat();
        if(currentBeat != lastBeat){
            lastBeat = currentBeat;
            emit intervalBeatChanged(currentBeat);
        }
        mainController->doAudioProcess(in, out);
        samplesProcessed += samplesToProcessInThisStep;
        offset += samplesToProcessInThisStep;
        out.setFrameLenght(outLenght);//restore
        this->intervalPosition = (this->intervalPosition + samplesToProcessInThisStep) % samplesInInterval;
    }
    while( samplesProcessed < totalSamplesToProcess);
    out.resetOffset();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void NinjamJamRoomController::processScheduledChanges(){
    if(newBpi > 0){
        currentBpi = newBpi;
        newBpi = -1;
        this->samplesInInterval = computeTotalSamplesInInterval();
        emit currentBpiChanged(currentBpi); //ui->topPanel->setBpi(currentBpi);

    }
    if(newBpm > 0){
        currentBpm = newBpm;
        newBpm = -1;
        this->samplesInInterval = computeTotalSamplesInInterval();
        this->metronomeTrackNode->setSamplesPerBeat(getSamplesPerBeat());
        emit currentBpmChanged((currentBpm)); //ui->topPanel->setBpm(currentBpm);
    }
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
long NinjamJamRoomController::getSamplesPerBeat(){
    return computeTotalSamplesInInterval()/currentBpi;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
long NinjamJamRoomController::computeTotalSamplesInInterval(){
    double intervalPeriod =  60000.0 / currentBpm * currentBpi;
    int sampleRate = mainController->getAudioDriver()->getSampleRate();
    return (long)(sampleRate * intervalPeriod / 1000.0);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//ninjam events

void NinjamJamRoomController::ninjamUserChannelCreated(Ninjam::User user, Ninjam::UserChannel channel){
    addTrack(user, channel);
}

void NinjamJamRoomController::ninjamUserChannelRemoved(Ninjam::User user, Ninjam::UserChannel channel){
    removeTrack(user, channel);
}

void NinjamJamRoomController::ninjamUserChannelUpdated(Ninjam::User user, Ninjam::UserChannel channel){
    QString uniqueKey = getUniqueKey(channel);
    if(trackNodes.contains(uniqueKey)){
        NinjamTrackNode* trackNode = trackNodes[uniqueKey];
        emit channelChanged(user, channel, trackNode->getID());
    }

}

void NinjamJamRoomController::ninjamServerBpiChanged(short newBpi, short /*oldBpi*/){
    //this->samplesInInterval = computeTotalSamplesInInterval();
    this->newBpi = newBpi;
}

void NinjamJamRoomController::ninjamServerBpmChanged(short newBpm){
    //this->metronomeTrackNode->setSamplesPerBeat(getSamplesPerBeat());
    this->newBpm = newBpm;
}

void NinjamJamRoomController::ninjamAudioAvailable(Ninjam::User user, int channelIndex, QByteArray encodedAudioData, bool lastPartOfInterval){

    Ninjam::UserChannel channel = user.getChannel(channelIndex);
    QString channelKey = getUniqueKey(channel);
    if(trackNodes.contains(channelKey)){
        NinjamTrackNode* trackNode = trackNodes[channelKey];
        trackNode->addEncodedBytes(encodedAudioData, lastPartOfInterval);
    }
    else{
        qWarning() << "o canal " << channelIndex << " do usuário " << user.getName() << " não foi encontrado no mapa!";
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
