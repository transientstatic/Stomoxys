# Stomoxys

This repository was built using JUCE 5.2.0. 
It requires downloading and installing the Sensel libraries:  https://github.com/sensel/sensel-api
Sensel librarys are required both for building and running any resulting VST/AU.

This is a project we used to interact with the Sensel Morph and send data over audio to minimize potential latency and jitter in data reaching the target VST.  While using data as audio does require rebuilding the target VST, it is fairly easy to implement using the code here as an example.  This project is not meant to directly suit your needs, but act as an example how to read and use the Sensel, along with how to use data as audio.  It is posted with some expectation that you know your way around JUCE and C++ coding.  It isn't particularly well commented (yet) and might not ever be (though if a few people are interested, I will comment it better).

This project includes code for opening and using a Sensel Morph at it's default sample rate (125Hz) and passing that through data as audio whenever the next audio block is serviced.  While the receiving VST will only receive updates every audio block, the data as audio preserves when within the processed audio block the Sensel update was received.

It also currently includes things you probably don't care about like randomly setting configurations for study trials, logging data, and handling OSC.  OSC send and receiving largely because we were curious about timing.  While we have found OSC sending to be useful, unless you change the priority on the OSC receive thread, it is currently really slow with high latency and high jitter.  We're using the VST primarily within Reaper which has an inbuilt OSC server which is easy to use and much more reliable (update every 70ms as opposed to 2s).

If you want to build the project included here be aware of the following:
1) If you want to build with a version of JUCE other than 5.2.0, it is strongly recommended to use projucer to create a new project linking to the correct JUCE version.  Confusingly, XCode may appear to successfully build even if you are using a different version of JUCE, but then the plugin won't work.
2) Most of the project settings are included in the Projucer file Stomoxys.jucer.  There are additional header paths that will need to be included within the resulting project.  Make sure to update the header paths in Stomoxys.jucer appropriate for where you have things.
3) Currently the Library Search Path isn't added within projucer which is necessary to link to the Sensel library (you can do this yourself using -L/usr/local/lib).  Otherwise,in Xcode, Library Search Paths you'll need to add  "/usr/local/lib".  
4) If you are getting a link error about cyclic dependencies within Xcode you may also have to set the project setting (File->ProjectSettings Build System) to Legacy Build systems. 

TransientStatic
