//
//  DataAsAudio.cpp
//  SenselVapbi
//
//  Created by punkass on 10/12/2019.
//

#include "DataAsAudio.hpp"

//If I'm using vectors, might as well do vector math...
#include <functional>    // std::plus/minus/mulitplies/etc.
#include <algorithm>

//  outgoing channel packet:
//    1,    ( signals start) (1.2)
//    2-3   timestamp   (32*2 = 64 bits)
//    4     sigma0Plate
//    5     sigma1Plate
//    6     fundFreqPlate
//    7     xr1_excitation
//    8     yr1_excitation
//    9     audioDelayLength
//    10     (checking end)  (-1.2)
//  Fill with zero to

// Read will look for the start and end bits (which should be max and min).  Because of auto muting, need to make sure all data ends up under 1.0.  Hence data is scaled down.

//For reasons I don't yet understand, these don't add well to the vapbi project.  These are copies

// The various max values for the different parameters.
const Sensel_Vapbi_Data DataAsAudio::mMaxValues(30.0f, 1.0f, 160.0f, 1.0f, 1.0f);
const Sensel_Vapbi_Data DataAsAudio::mMinValues(.01f, .0032f, 1.0f, 0.0f, 0.0f);

DataAsAudio::DataAsAudio(const uint32 dataBlockSize)
{
    mDataBlock.assign(dataBlockSize, 0.0f);
}; //This is essentially a static class, with

//This would work well using deque but it was being really wierd...

//A bit of useful knowledge about timing... the playhead points to the timeInSamples for the beginnging of the buffer, not the end.
//One per audio channel.
void DataAsAudio::writeDataToAudio(AudioSampleBuffer& buffer, int64 inTime, uint32 channel, std::vector<Sensel_Vapbi_Data>& data){

    const int numSamples = buffer.getNumSamples();          // How many samples in the buffer for this block?
    
    //want data block size to be a multiple of the total number of samples within the buffer.
    assert(numSamples % mDataBlock.size() == 0);
    int numHops = (int) floor(numSamples/mDataBlock.size());
    
    Sensel_Vapbi_Data newData;
    
    int count = 0;
    for (int i = 0; i < numSamples; i = i + mDataBlock.size()){
        
        if (data.size() > count) {
            newData = data[count];
            //usually going to only be one
            if (newData.timestamp <= (inTime + i + mDataBlock.size())) { //the plus one prob not needed
                mDefaultSendData = newData;
                count++;
            }
        }
        
        //build hop buffer
        
        mDataBlock.assign(mDataBlock.size(), 0.0f); //set everything to zero
        //let's get data between the two working first, then we'll deal with any changes in data
        float timestampLowBits = 0.f;
        float timestampHighBits = 0.f;
        if (mDefaultSendData.timestamp != 0){
            uint32 uitmp = static_cast<uint32>(mDefaultSendData.timestamp & 0x00000000ffffffff); //bottom 32 bits
            timestampLowBits = 1.0f/((float) uitmp); //may be better ways of doing this
            uitmp = static_cast<uint32>((mDefaultSendData.timestamp >> 32) & 0x00000000ffffffff);
            timestampHighBits = 1.0f/((float) uitmp); //may be better ways of doing this
        }
        mDataBlock[0] = DATA_AS_AUDIO_PACKET_START;
        mDataBlock[kTimestampL] = timestampLowBits;
        mDataBlock[kTimestampH] = timestampHighBits;
        mDataBlock[kSigma0Plate] = mDefaultSendData.sigma0Plate;
        mDataBlock[kSigma1Plate] = mDefaultSendData.sigma1Plate;
        mDataBlock[kFundFreqPlate] = mDefaultSendData.fundFreqPlate;
        mDataBlock[kXr1_excitation] = mDefaultSendData.xr1_excitation;
        mDataBlock[kYr1_excitation] = mDefaultSendData.yr1_excitation;
        mDataBlock[kNumParameters] = DATA_AS_AUDIO_PACKET_END;

        //need to normalize the parameter data so everything is under 1.0
        const float * maxDataBlock = (const float*) (&mMaxValues.timestamp); //data should be stored consecutively in memory.  Let's take advantage
        const float * minDataBlock = (const float*) (&mMinValues.timestamp);
        for (int j = kSigma0Plate; j < kNumParameters; j++){
            if (mDataBlock[j] < minDataBlock[j-1]) {
                mDataBlock[j] = minDataBlock[j-1];
            }
            mDataBlock[j] = mDataBlock[j]/maxDataBlock[j-1];
        }
        
        const float volRedux = 0.8;  //If this is changed, need to alter the start and end values.
        // rescale everything so we don't accidentally get the channel muted for being too loud
        std::transform(mDataBlock.begin(), mDataBlock.end(), mDataBlock.begin(), std::bind2nd(std::multiplies<float>(), volRedux));
        
        //buffer is a Juce thing and this copies from the handed buffer into juce
        buffer.copyFrom(channel, i, mDataBlock.data(), mDataBlock.size());
    }
    data.clear();
}

void DataAsAudio::readDataFromAudio(AudioSampleBuffer& buffer, uint32 channel, std::vector<Sensel_Vapbi_Data>& parameters){
    const int numSamples = buffer.getNumSamples();          // How many samples in the buffer for this block?
    
    assert(numSamples % mDataBlock.size() == 0);

    float* channelData = buffer.getWritePointer(channel);
    
    Sensel_Vapbi_Data readData;

    for (int i = 0; i < numSamples; i += mDataBlock.size()){
        
        //look for a 1, keep track of max in case I don't find it (which suggests gain mod.)  Also count zeros
        float maxFound, minFound = 0.0f;
        int maxi, mini = 0;
        int zeroCnt = 0;

        mDataBlock.assign(mDataBlock.size(), 0.0f);
        
        for (int k = 0; k < mDataBlock.size(); k++){
            mDataBlock[k] = channelData[i+k];
            float sigdat = channelData[i+k];
            if (sigdat > maxFound){
                maxFound = sigdat;
                maxi = k;
            } else if (sigdat < minFound){
                minFound = sigdat;
                mini = k;
            }
            
            if (sigdat == 0.0){
                zeroCnt++;
            } else {
                zeroCnt = 0;
            }
        }
        
        //if this is failing, double check not getting audio into same channel
        if ((maxi == 0) && ((mini - maxi) == kNumParameters) && (maxFound == -minFound)){

            float scaleFactor = 1.0;
            if ((maxFound != DATA_AS_AUDIO_PACKET_START) && (maxFound != 0.f)) { //this is the normal state since we've intentionally scaled down
                scaleFactor = DATA_AS_AUDIO_PACKET_START/maxFound;
            }

            //audio buffer only has copy to it, not bulk read so do everything in one go:
            float timeStampLow = 0.f;
            if (channelData[i + kTimestampL] != 0.f) timeStampLow = 1.0f/(channelData[i + kTimestampL] * scaleFactor);
            float timeStampHigh = 0.f;
            if (channelData[i + kTimestampH] != 0.f) timeStampHigh = 1.0f/(channelData[i + kTimestampH] * scaleFactor);
            readData.timestamp = static_cast<int64>(timeStampLow) + (static_cast<uint64>(timeStampHigh) << 32);
        
            //need to normalize the parameter data so everything is under 1.0
            const float * maxDataBlock = (const float*) (&mMaxValues.timestamp);
            float * params = (float *) (&readData.timestamp);
            for (int j = kSigma0Plate; j < kNumParameters; j++){
                params[j-1] = channelData[j]*maxDataBlock[j-1]*scaleFactor;
            }
            
            parameters.push_back(readData); //copies the found parameters into return vector
        }
    }
}


