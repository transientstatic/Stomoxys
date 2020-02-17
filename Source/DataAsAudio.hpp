//
//  DataAsAudio.hpp
//  SenselVapbi
//
//  Created by punkass on 10/12/2019.
//

#ifndef DataAsAudio_hpp
#define DataAsAudio_hpp

//#include "PluginProcessor.h"
#include "../JuceLibraryCode/JuceHeader.h" //for int types and audio buffer

#include <stdio.h>
#include <deque>
#include <vector>

struct Sensel_Vapbi_Data{
    uint64 timestamp = 0;
    float sigma0Plate;
    float sigma1Plate;
    float fundFreqPlate;
    float xr1_excitation;
    float yr1_excitation;
    Sensel_Vapbi_Data() : timestamp(0), sigma0Plate(0.15f), sigma1Plate(0.01f), fundFreqPlate(17.7f), xr1_excitation(.5f), yr1_excitation(0.5f) {} //.15
    Sensel_Vapbi_Data(float s0, float s1, float ffp, float x, float y) : timestamp(1), sigma0Plate(s0), sigma1Plate(s1), fundFreqPlate(ffp), xr1_excitation(x), yr1_excitation(y) {}
};

class DataAsAudio {
public:

    enum Sensel_Vapbi_Parameters{
        kTimestampL = 1,
        kTimestampH = 2,
        kSigma0Plate = 3,
        kSigma1Plate = 4,
        kFundFreqPlate = 5,
        kXr1_excitation = 6,
        kYr1_excitation = 7,
        kNumParameters
    };

    // Data block size is how many samples per data update.  The audio processing block size should be a multiple of the data block size.  Each data block contains the required parameters, bookended by 1.0, with unused space filled with zeros.
    DataAsAudio(const uint32 dataBlockSize=64); //This is essentially a static class, with the exception that it allocates the processing buffer at instantiation which should avoid timely memory allocations.
    
    //buffer is the JUCE supplied audio buffer
    //channel is the target audio channel within the audio buffer to write to
    //data is the set of data to be added to the full audio block
    void writeDataToAudio(AudioSampleBuffer& buffer, int64 inTime, uint32 channel, std::vector<Sensel_Vapbi_Data>& data);

    //buffer is the JUCE supplied audio buffer
    //channel is the target audio channel within the audio buffer to read from
    //returns vector of data (this method allows it to be allocated outside real-time operations)
    void readDataFromAudio(AudioSampleBuffer& buffer, uint32 channel, std::vector<Sensel_Vapbi_Data>& parameters);

    
    //These should be greater than one but less than 1.25
    const float DATA_AS_AUDIO_PACKET_START = 1.2f;
    const float DATA_AS_AUDIO_PACKET_END = -1.2f;

    static const Sensel_Vapbi_Data mMaxValues;
    static const Sensel_Vapbi_Data mMinValues;
    
private:
    std::vector<float>  mDataBlock;
    Sensel_Vapbi_Data   mDefaultSendData;
};


#endif /* DataAsAudio_hpp */
