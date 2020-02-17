//
//  SenselWrapper.cpp
//  SenselVapbiSend
//
//  Created by punkass on 07/01/2020.
//

#include "SenselWrapper.h"
#include <stdio.h>
#include <math.h>

Sensel::Sensel()
{
    
    senselGetDeviceList(&list);
    
    // Get a list of avaialble Sensel devices
    //cout << "Devices available: " << list.num_devices << "\n";
    //cout << "name of device: " << list.devices << "\n";
    //cout << "Device 0: " << list.devices[0].idx << "\n";
    //cout << "Device 1: " << list.devices[1].idx << "\n";
    
    if (list.num_devices == 0)
    {
        fprintf(stdout, "No Sensel device found.\n");
        return;
    }
    else
    {
        fprintf(stdout, "Sensel device detected.\n");
        senselDetected = true;
    }
    
    //Open a Sensel device by the id in the SenselDeviceList, handle initialized
    senselOpenDeviceByID(&handle, list.devices[senselIndex].idx);
    //Set the frame content to scan contact data
    senselSetFrameContent(handle, FRAME_CONTENT_CONTACTS_MASK);
    //Allocate a frame of data, must be done before reading frame data
    senselAllocateFrameData(handle, &frame);
    //        senselSetBufferControl(handle, 10);
    
    senselSetContactsMask(handle, CONTACT_MASK_DELTAS);
    
    SenselScanDetail scanDetail;
    senselGetScanDetail(handle, &scanDetail);
    switch (scanDetail)  {
        case SCAN_DETAIL_HIGH :
            mSampleRate = 125.0;
            break;
        case SCAN_DETAIL_LOW :
            mSampleRate = 500;
            break;
        case SCAN_DETAIL_MEDIUM:
        case SCAN_DETAIL_UNKNOWN:
            mSampleRate = 250.f; //I've just made this up because I have no idea what it is...
            break;
    }
    
    //Start scanning the Sensel device
    senselStartScanning(handle);    
}

// ID contructor for when you want more sensels
Sensel::Sensel(unsigned int senselID)
{
    senselIndex = senselID;
    
    senselGetDeviceList(&list);
    
    // Get a list of avaialble Sensel devices
    //cout << "Devices available: " << list.num_devices << "\n";
    //cout << "name of device: " << list.devices << "\n";
    //cout << "Device 0: " << list.devices[0].idx << "\n";
    //cout << "Device 1: " << list.devices[1].idx << "\n";
    
    if (list.num_devices == 0)
    {
        fprintf(stdout, "No Sensel device found.\n");
        return;
    }
    else
    {
        fprintf(stdout, "Sensel device detected.\n");
        senselDetected = true;
    }
    
    //Open a Sensel device by the id in the SenselDeviceList, handle initialized
    senselOpenDeviceByID(&handle, list.devices[senselIndex].idx);
    //Set the frame content to scan contact data
    senselSetFrameContent(handle, FRAME_CONTENT_CONTACTS_MASK);
    //Allocate a frame of data, must be done before reading frame data
    senselAllocateFrameData(handle, &frame);
    //        senselSetBufferControl(handle, 2);  //incase I miss one...
    
    senselSetContactsMask(handle, CONTACT_MASK_DELTAS);
    
    //Start scanning the Sensel device
    senselStartScanning(handle);
}

void Sensel::shutDown()
{
    senselClose(handle);
}

void Sensel::setLPCoefs(const std::array<float, 5>& inCoefs, const float inXStart, const bool inYStart){
    for (int i = 0; i < SENSEL_MAX_NUM_CONTACTS; i++){
        fingers[i].lp.setCoefs(inCoefs, inXStart, inYStart);
    }
};

void Sensel::setHPCoefs(const std::array<float, 5>& inCoefs, const float inXStart, const bool inYStart){
    for (int i = 0; i < SENSEL_MAX_NUM_CONTACTS; i++){
        fingers[i].hp.setCoefs(inCoefs, inXStart, inYStart);
    }
};

void Sensel::setLP_F0(const float inF0){
    for (int i = 0; i < SENSEL_MAX_NUM_CONTACTS; i++){
        fingers[i].lp.setQ(inF0, Biquad::kLowPass); //setQ doesn't really do anything
        fingers[i].lp.setF0(inF0, Biquad::kLowPass);
    }
};

void Sensel::setHP_F0(const float inF0){
    for (int i = 0; i < SENSEL_MAX_NUM_CONTACTS; i++){
        fingers[i].hp.setQ(inF0, Biquad::kHighPass); //setQ doesn't really do anything but set const mQ
        fingers[i].hp.setF0(inF0, Biquad::kLowPass);
    }
};

void Sensel::setLP_Q(const float inQ){
    for (int i = 0; i < SENSEL_MAX_NUM_CONTACTS; i++){
        fingers[i].lp.setQ(inQ, Biquad::kLowPass);
    }
};

void Sensel::setHP_Q(const float inQ){
    for (int i = 0; i < SENSEL_MAX_NUM_CONTACTS; i++){
        fingers[i].hp.setQ(inQ, Biquad::kHighPass);
    }
};

bool Sensel::check()
{
    //for (int i = 0; i < fingers.size(); i++)
    //    fingers[i].state.store(false);
    
    if (!senselDetected) {
        return false;
    }
    unsigned int num_frames = 0;
    //Read all available data from the Sensel device
    SenselStatus success = senselReadSensor(handle);
    if (success == SENSEL_ERROR){
        return false;
    }
    
    //Get number of frames available in the data read from the sensor
    senselGetNumAvailableFrames(handle, &num_frames);
    numFrames = num_frames;
    
    for (int f = 0; f < num_frames; f++)
    {
        if (f > 1) continue; //this wrapper isn't set up to handle more than one frame at a time
        
        //Read one frame of data
        senselGetFrame(handle, frame);
        
        droppedFrames = frame->lost_frame_count;
        
        contactAmount = frame->n_contacts;
        
        activeContacts.clear();
        
        // Get contact data
        if (contactAmount > 0)
        {
            for (int c = 0; c < contactAmount; c++)
            {
                // mapping
                unsigned int state = frame->contacts[c].state;
                float force = frame->contacts[c].total_force / 8192.0f;
                float x = frame->contacts[c].x_pos / 240.0f;
                float y = frame->contacts[c].y_pos / 139.0f;
                float delta_x = frame->contacts[c].delta_x;
                float delta_y = frame->contacts[c].delta_y;
                float area = frame->contacts[c].area/8192.0f;
                
                int id = frame->contacts[c].id;
                
                if (id < fingers.size()){
                    fingers[id].force = force;
                    fingers[id].x = x;
                    fingers[id].y = y;
                    fingers[id].delta_x = delta_x;
                    fingers[id].delta_y = delta_y;
                    fingers[id].area = area;
                    fingers[id].fingerID = frame->contacts[c].id; //This should be redundant
                    fingers[id].state = (int) state;
                    if (state != CONTACT_END) activeContacts.push_back(id);
                    if (state == CONTACT_START) {
                        //passing (0,force) it does a much smoother settling.
                        fingers[id].hp.reset(0,force);
                        fingers[id].lp.reset(force, 0); //depends whether we want settling or not.
                        fingers[id].lp.update(force);
                    } else{
                        fingers[id].hp.update(force);
                        fingers[id].lp.update(force);
                    }
                    fingers[id].isDamp = (fingers[id].state != CONTACT_START);  //second helps prevent
//                    fingers[id].isDamp = (fingers[id].state != CONTACT_START) && ( (fabs(fingers[id].hp.current()) < mHPDampThreshold) || (fingers[id].lp.current() > mLPDampThreshold));  //second helps prevent agains slips causing false positives in hp
                }
            }
        }
    }
    return true;
    }
