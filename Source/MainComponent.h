#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent, public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent() : state(Stopped)
    {
        //Window size

        setSize(600, 400);
        //Open button
        addAndMakeVisible(&openButton);
        openButton.setButtonText("Open...");
        openButton.onClick = [this] { openButtonClicked(); };

        //Play Button
        addAndMakeVisible(&playButton);
        playButton.setButtonText("Play");
        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
        playButton.setEnabled(false);

        //Stop Button
        addAndMakeVisible(&stopButton);
        stopButton.setButtonText("Stop");
        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
        stopButton.setEnabled(false);

        formatManager.registerBasicFormats();
        transportSource.addChangeListener(this);

        setAudioChannels(0, 2);
    }

    ~MainComponent() override {
        shutdownAudio();
    };

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        if (readerSource.get() == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        transportSource.getNextAudioBlock(bufferToFill);
    }

    void releaseResources() override
    {
        transportSource.releaseResources();
    }

    //==============================================================================
    void resized() override {
        openButton.setBounds(10, 10, getWidth() - 20, 20);
        playButton.setBounds(10, 40, getWidth() - 20, 20);
        stopButton.setBounds(10, 70, getWidth() - 20, 20);
    };

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == &transportSource)
        {
            if (transportSource.isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
        }
    }

private:
    //==============================================================================
    // Your private member variables go here...

    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };

    void changeState(TransportState newState)
    {
        if (state != newState)
        {
            state = newState;

            switch (state)
            {
            case Stopped:                           // [3]
                stopButton.setEnabled(false);
                playButton.setEnabled(true);
                transportSource.setPosition(0.0);
                break;

            case Starting:                          // [4]
                playButton.setEnabled(false);
                transportSource.start();
                break;

            case Playing:                           // [5]
                stopButton.setEnabled(true);
                break;

            case Stopping:                          // [6]
                transportSource.stop();
                break;
            }
        }
    }

    void openButtonClicked()
    {
        chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...",
            juce::File{},
            "*.wav");                     // [7]
        auto chooserFlags = juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)     // [8]
            {
                auto file = fc.getResult();

                if (file != juce::File{})                                                // [9]
                {
                    auto* reader = formatManager.createReaderFor(file);                 // [10]

                    if (reader != nullptr)
                    {
                        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);   // [11]
                        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);       // [12]
                        playButton.setEnabled(true);                                                      // [13]
                        readerSource.reset(newSource.release());                                          // [14]
                    }
                }
            });
    }

    void playButtonClicked()
    {
        changeState(Starting);
    }

    void stopButtonClicked()
    {
        changeState(Stopping);
    }

    //Audio Player state
    TransportState state;

    //Buttons
    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
