#include <iostream>
#include "sensel.h"
#include "sensel_device.h"
#include <array>
#include <atomic>
#include <vector>
#include <math.h>

#define SENSEL_MAX_NUM_CONTACTS 16

//the biquad filtering implements the following difference equation:
//y(n) = a0 x(n) + a1 x(n-1) + a2 x(n-2) - b1 y(n-1) - b2 y(n-2)
class Biquad {    //I'd like to make this friends with Sensel and then private all but current...  When I am on-line
public:
    enum FilterType {
        kLowPass,
        kHighPass
    };
    
    Biquad() : coefs({0., 0., 0., 0., 0.}){};
    
    Biquad(const std::array<float, 5> inCoefs) : coefs(inCoefs){
    };
    
    void setCoefs(const std::array<float, 5>& inCoefs, const float inXStart=0.f, const bool inYStart=0.f){
        coefs = inCoefs;
        reset(inXStart, inYStart); //clear values
    };
    
    //For high pass, only need to give starting x of expected low freq., for low pass need both
    void reset(const float inXStart=0.f, const float inYStart=0.f){
        x1 = inXStart;
        x2 = inXStart;
        y1 = inYStart;
        y2 = inYStart;
    }
    
    const float update(const float unfiltered) {
        float y = coefs[0]*unfiltered + coefs[1]*x1 + coefs[2]*x2 - coefs[3]*y1 - coefs[4]*y2;
        x2 = x1;
        x1 = unfiltered;
        y2 = y1;
        y1 = y;
        return y;
    };
    
    void setF0(const float inFreqR, const FilterType isLowPass){
        w0 = 2.0*M_PI*inFreqR;
        cosOmega = cos(w0);
        float alpha = sin(w0)/(2.0*mQ);
        float a0 = (1.0f+alpha); // need to normalize by this
        float a1 = (-2.0*cosOmega)/a0;
        float a2 = (1.0f - alpha)/a0;
        float b1 = 0.f;
        if (isLowPass == kLowPass){
            b1 = (1.0-cosOmega)/a0; //b0 and b2 are b1/2
            setCoefs({static_cast<float>(b1/2.0), b1, static_cast<float>(b1/2.0), a1, a2});
        } else {
            b1 = (1.0+cos(w0))/a0;
            setCoefs({ static_cast<float>(b1/2.0),  static_cast<float>(-1.0*b1), static_cast<float>(b1/2.0),  a1,  a2});
        }
        mF0 = inFreqR;
    }
    
    void setQ(const float inQ, const FilterType isLowPass){
        mQ = sqrt(.5);
    }
    
    const float current(void){ return y1;}

    std::array<float, 5> coefs;

private:
//    std::array<float, 5> coefs;
    float x1 = 0.f;
    float x2 = 0.f;
    float y1 = 0.f;
    float y2 = 0.f;
    
    float mF0 = .025;
    float mQ = .01;
    double cosOmega;
    double w0;
};

struct Contact
{
    int state = false;
    float force = 0.0;
    float area = 0.0;
    float x = 0.0;
    float y = 0.0;
    float delta_x = 0.0;
    float delta_y = 0.0;
    int fingerID = -1;
    bool isDamp = false; //if this low passes, it becomes true
    Biquad lp;
    Biquad hp;
};

class Sensel
{
  public:
    Sensel();
    Sensel(unsigned int senselID);
    
    void setLPCoefs(const std::array<float, 5>& inCoefs, const float inXStart=0.f, const bool inYStart=0.f);
    void setHPCoefs(const std::array<float, 5>& inCoefs, const float inXStart=0.f, const bool inYStart=0.f);
    void setLP_F0(const float inFreqR);
    void setHP_F0(const float inFreqR);
    void setLP_Q(const float inQ);
    void setHP_Q(const float inQ);
    
    void shutDown();
    //LEDs slows things down a bunch.  (About 8ms per contact)
    void turnLedsOn(void);

    void turnLedsOff(void);

    bool check();

    float getSampleRate(void) { return mSampleRate; };
    
    std::array<Contact, SENSEL_MAX_NUM_CONTACTS> fingers;
    std::vector<float> activeContacts;
    unsigned int senselIndex = 0;
    int idx = -1;
    bool senselDetected = false;
    unsigned int contactAmount = 0;
    unsigned int numFrames = 0;
    unsigned int droppedFrames = 0;
    float mHPDampThreshold = .001;
    float mLPDampThreshold = .015;
    
  private:
    
    SENSEL_HANDLE handle = NULL;
    //List of all available Sensel devices
    SenselDeviceList list;
    //SenselFrame data that will hold the contacts
    SenselFrameData *frame = NULL;
    
    float mSampleRate = 125.0;

};
