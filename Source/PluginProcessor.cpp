/**************************************************************************
 * Copyright 2018 UdeM - GRIS - Olivier Belanger                          *
 *                                                                        *
 * This file is part of ControlGris, a multi-source spatialization plugin *
 *                                                                        *
 * ControlGris is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * ControlGris is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU Lesser General Public       *
 * License along with ControlGris.  If not, see                           *
 * <http://www.gnu.org/licenses/>.                                        *
 *************************************************************************/

#include "PluginProcessor.h"

#include "PluginEditor.h"

enum FixedPositionType { terminal, initial };

String getFixedPosSourceName(FixedPositionType const fixedPositionType, SourceIndex const index, int const dimension)
{
    String const type{ fixedPositionType == FixedPositionType::terminal ? "_terminal" : "" };
    String result{};
    switch (dimension) {
    case 0:
        result = String("S") + String(index.toInt() + 1) + type + String("_X");
        break;
    case 1:
        result = String("S") + String(index.toInt() + 1) + type + String("_Y");
        break;
    case 2:
        result = String("S") + String(index.toInt() + 1) + type + String("_Z");
        break;
    default:
        jassertfalse; // how did you get there?
    }
    return result;
}

// The parameter Layout creates the automatable mParameters.
AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Parameter = AudioProcessorValueTreeState::Parameter;

    std::vector<std::unique_ptr<Parameter>> mParameters;

    mParameters.push_back(std::make_unique<Parameter>(String("recordingTrajectory_x"),
                                                      String("Recording Trajectory X"),
                                                      String(),
                                                      NormalisableRange<float>(0.0f, 1.0f),
                                                      0.0f,
                                                      nullptr,
                                                      nullptr));
    mParameters.push_back(std::make_unique<Parameter>(String("recordingTrajectory_y"),
                                                      String("Recording Trajectory Y"),
                                                      String(),
                                                      NormalisableRange<float>(0.0f, 1.0f),
                                                      0.0f,
                                                      nullptr,
                                                      nullptr));
    mParameters.push_back(std::make_unique<Parameter>(String("recordingTrajectory_z"),
                                                      String("Recording Trajectory Z"),
                                                      String(),
                                                      NormalisableRange<float>(0.0f, 1.0f),
                                                      0.0f,
                                                      nullptr,
                                                      nullptr));

    mParameters.push_back(std::make_unique<Parameter>(
        String("sourceLink"),
        String("Source Link"),
        String(),
        NormalisableRange<float>(0.0f, static_cast<float>(POSITION_SOURCE_LINK_TYPES.size() - 1), 1.0f),
        0.0f,
        nullptr,
        nullptr,
        false,
        true,
        true));
    mParameters.push_back(std::make_unique<Parameter>(String("sourceLinkAlt"),
                                                      String("Source Link Alt"),
                                                      String(),
                                                      NormalisableRange<float>(0.0f, 4.0f, 1.0f),
                                                      0.0f,
                                                      nullptr,
                                                      nullptr,
                                                      false,
                                                      true,
                                                      true));

    mParameters.push_back(std::make_unique<Parameter>(String("positionPreset"),
                                                      String("Position Preset"),
                                                      String(),
                                                      NormalisableRange<float>(0.0f, 50.0f, 1.0f),
                                                      0.0f,
                                                      nullptr,
                                                      nullptr,
                                                      false,
                                                      true,
                                                      true));

    mParameters.push_back(std::make_unique<Parameter>(String("azimuthSpan"),
                                                      String("Azimuth Span"),
                                                      String(),
                                                      NormalisableRange<float>(0.0f, 1.0f),
                                                      0.0f,
                                                      nullptr,
                                                      nullptr));
    mParameters.push_back(std::make_unique<Parameter>(String("elevationSpan"),
                                                      String("Elevation Span"),
                                                      String(),
                                                      NormalisableRange<float>(0.0f, 1.0f),
                                                      0.0f,
                                                      nullptr,
                                                      nullptr));

    return { mParameters.begin(), mParameters.end() };
}

//==============================================================================
ControlGrisAudioProcessor::ControlGrisAudioProcessor()
    :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor(BusesProperties()
    #if !JucePlugin_IsMidiEffect
        #if !JucePlugin_IsSynth
                       .withInput("Input", AudioChannelSet::stereo(), true)
        #endif // JucePlugin_IsSynth
                       .withOutput("Output", AudioChannelSet::stereo(), true)
    #endif // JucePlugin_IsMidiEffect
                       )
    ,
#endif // JucePlugin_PreferredChannelConfigurations
    mParameters(*this, nullptr, Identifier(JucePlugin_Name), createParameterLayout())
    , mFixPositionData(FIXED_POSITION_DATA_TAG)
{
    // Size of the plugin window.
    mParameters.state.addChild({ "uiState", { { "width", 650 }, { "height", 700 } }, {} }, -1, nullptr);

    // Global setting mParameters.
    mParameters.state.setProperty("oscFormat", 0, nullptr);
    mParameters.state.setProperty("oscPortNumber", 18032, nullptr);
    mParameters.state.setProperty("oscConnected", true, nullptr);
    mParameters.state.setProperty("oscInputPortNumber", 9000, nullptr);
    mParameters.state.setProperty("oscInputConnected", false, nullptr);
    mParameters.state.setProperty("oscOutputAddress", "192.168.1.100", nullptr);
    mParameters.state.setProperty("oscOutputPortNumber", 8000, nullptr);
    mParameters.state.setProperty("oscOutputConnected", false, nullptr);
    mParameters.state.setProperty("numberOfSources", 2, nullptr);
    mParameters.state.setProperty("firstSourceId", 1, nullptr);
    mParameters.state.setProperty("oscOutputPluginId", 1, nullptr);

    // Trajectory box persitent settings.
    mParameters.state.setProperty("trajectoryType", static_cast<int>(PositionTrajectoryType::realtime), nullptr);
    mParameters.state.setProperty("trajectoryTypeAlt", static_cast<int>(ElevationTrajectoryType::realtime), nullptr);
    mParameters.state.setProperty("backAndForth", false, nullptr);
    mParameters.state.setProperty("backAndForthAlt", false, nullptr);
    mParameters.state.setProperty("dampeningCycles", 0, nullptr);
    mParameters.state.setProperty("dampeningCyclesAlt", 0, nullptr);
    mParameters.state.setProperty("deviationPerCycle", 0, nullptr);
    mParameters.state.setProperty("cycleDuration", 5, nullptr);
    mParameters.state.setProperty("durationUnit", 1, nullptr);

    // Per source mParameters. Because there is no attachment to the automatable
    // mParameters, we need to keep track of the current parameter values to be
    // able to reload the last state of the plugin when we close/open the UI.
    for (int i{}; i < MAX_NUMBER_OF_SOURCES; ++i) {
        String oscId(i);
        // Non-automatable, per source, mParameters.
        mParameters.state.setProperty(String("p_azimuth_") + oscId, i % 2 == 0 ? -90.0 : 90.0, nullptr);
        mParameters.state.setProperty(String("p_elevation_") + oscId, 0.0, nullptr);
        mParameters.state.setProperty(String("p_distance_") + oscId, 1.0, nullptr);
        mParameters.state.setProperty(String("p_x_") + oscId, 0.0, nullptr);
        mParameters.state.setProperty(String("p_y_") + oscId, 0.0, nullptr);

        // Gives the source an initial id...
        mSources.get(i).setId(SourceId{ i + mFirstSourceId.toInt() });
        // .. and coordinates.
        auto const azimuth{ i % 2 == 0 ? Degrees{ -45.0f } : Degrees{ 45.0f } };
        mSources.get(i).setCoordinates(azimuth, MAX_ELEVATION, 1.0f, SourceLinkNotification::notify);
    }

    mParameters.getParameter("recordingTrajectory_x")->setValue(mSources.getPrimarySource().getPos().x);
    mParameters.getParameter("recordingTrajectory_y")->setValue(mSources.getPrimarySource().getPos().y);

    // Automation values for the recording trajectory.
    mParameters.addParameterListener(String("recordingTrajectory_x"), this);
    mParameters.addParameterListener(String("recordingTrajectory_y"), this);
    mParameters.addParameterListener(String("recordingTrajectory_z"), this);
    mParameters.addParameterListener(String("sourceLink"), this);
    mParameters.addParameterListener(String("sourceLinkAlt"), this);
    mParameters.addParameterListener(String("positionPreset"), this);
    mParameters.addParameterListener(String("azimuthSpan"), this);
    mParameters.addParameterListener(String("elevationSpan"), this);

    mPositionAutomationManager.addListener(this);
    mElevationAutomationManager.addListener(this);

    // The timer's callback send OSC messages periodically.
    //-----------------------------------------------------
    startTimerHz(50);
}

ControlGrisAudioProcessor::~ControlGrisAudioProcessor()
{
    disconnectOSC();
}

//==============================================================================
void ControlGrisAudioProcessor::parameterChanged(String const & parameterID, float newValue)
{
    if (std::isnan(newValue) || std::isinf(newValue)) {
        return;
    }
    MessageManagerLock const
        messageManagerLock{}; /* Somehow, this callback is not happening on the message thread. This will make some
                               * broadcaster callbacks fail in the Source class. Source-modifiying and painting
                               * functions must be asynchronously forwarded to the message thread. */
    Normalized const normalized{ newValue };
    if (parameterID.compare("recordingTrajectory_x") == 0) {
        mPositionAutomationManager.setPlaybackPositionX(newValue);
        mSources.getPrimarySource().setX(normalized, SourceLinkNotification::notify);
    } else if (parameterID.compare("recordingTrajectory_y") == 0) {
        mPositionAutomationManager.setPlaybackPositionY(newValue);
        mSources.getPrimarySource().setY(normalized, SourceLinkNotification::notify);
    } else if (parameterID.compare("recordingTrajectory_z") == 0 && mSpatMode == SpatMode::cube) {
        mElevationAutomationManager.setPlaybackPositionY(newValue);
        mSources.getPrimarySource().setElevation(MAX_ELEVATION - (MAX_ELEVATION * normalized.toFloat()),
                                                 SourceLinkNotification::notify);
    }

    if (parameterID.compare("sourceLink") == 0) {
        auto const val = static_cast<PositionSourceLink>(static_cast<int>(newValue) + 1);
        setPositionSourceLink(val);
    }

    if (parameterID.compare("sourceLinkAlt") == 0) {
        auto const val = static_cast<ElevationSourceLink>(static_cast<int>(newValue) + 1);
        if (val != mElevationAutomationManager.getSourceLink()) {
            setElevationSourceLink(val);
            auto * ed = dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor());
            if (ed != nullptr) {
                ed->updateElevationSourceLinkCombo(static_cast<ElevationSourceLink>(val));
            }
        }
    }

    if (parameterID.compare("positionPreset") == 0) {
        auto const value{ static_cast<int>(newValue) };
        setPositionPreset(value);
    }

    if (parameterID.startsWith("azimuthSpan")) {
        for (auto & source : mSources) {
            source.setAzimuthSpan(normalized);
        }
    } else if (parameterID.startsWith("elevationSpan")) {
        for (auto & source : mSources) {
            source.setElevationSpan(normalized);
        }
    }
}

//========================================================
void ControlGrisAudioProcessor::setPositionSourceLink(PositionSourceLink const newSourceLink)
{
    if (newSourceLink != mPositionAutomationManager.getSourceLink()) {
        mPositionAutomationManager.setSourceLink(newSourceLink);

        auto const howMany{ static_cast<float>(POSITION_SOURCE_LINK_TYPES.size() - 1) };
        mParameters.getParameter("sourceLink")->beginChangeGesture();
        mParameters.getParameter("sourceLink")
            ->setValueNotifyingHost((static_cast<float>(newSourceLink) - 1.0f) / howMany);
        mParameters.getParameter("sourceLink")->endChangeGesture();

        auto * editor{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
        if (editor != nullptr) {
            editor->updateSourceLinkCombo(newSourceLink);
        }

        mPositionSourceLinkEnforcer.setSourceLink(newSourceLink);
        mPositionSourceLinkEnforcer.enforceSourceLink();
    }
}

void ControlGrisAudioProcessor::setElevationSourceLink(ElevationSourceLink newSourceLink)
{
    if (newSourceLink != mElevationAutomationManager.getSourceLink()) {
        mElevationAutomationManager.setSourceLink(newSourceLink);

        mParameters.getParameter("sourceLinkAlt")->beginChangeGesture();
        mParameters.getParameter("sourceLinkAlt")
            ->setValueNotifyingHost((static_cast<float>(newSourceLink) - 1.0f) / 4.0f);
        mParameters.getParameter("sourceLinkAlt")->endChangeGesture();

        auto * ed{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
        if (ed != nullptr) {
            ed->updateElevationSourceLinkCombo(newSourceLink);
        }

        mElevationSourceLinkEnforcer.setSourceLink(newSourceLink);
        mElevationSourceLinkEnforcer.enforceSourceLink();
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setSpatMode(SpatMode const spatMode)
{
    mSpatMode = spatMode;
    mParameters.state.setProperty("oscFormat", static_cast<int>(mSpatMode), nullptr);
    for (int i{}; i < MAX_NUMBER_OF_SOURCES; ++i) {
        mSources.get(i).setSpatMode(spatMode);
    }

    if (spatMode == SpatMode::dome) {
        // remove cube-specific gadgets
        mElevationSourceLinkEnforcer.setSourceLink(ElevationSourceLink::independent);
    } else {
        jassert(spatMode == SpatMode::cube);
        mElevationSourceLinkEnforcer.setSourceLink(mElevationAutomationManager.getSourceLink());
    }
}

void ControlGrisAudioProcessor::setOscPortNumber(int oscPortNumber)
{
    mCurrentOSCPort = oscPortNumber;
    mParameters.state.setProperty("oscPortNumber", mCurrentOSCPort, nullptr);
}

void ControlGrisAudioProcessor::setFirstSourceId(SourceId const firstSourceId, bool propagate)
{
    mFirstSourceId = firstSourceId;
    mParameters.state.setProperty("firstSourceId", mFirstSourceId.toInt(), nullptr);
    for (int i{}; i < MAX_NUMBER_OF_SOURCES; ++i) {
        mSources.get(i).setId(SourceId{ i + mFirstSourceId.toInt() });
    }

    if (propagate)
        sendOscMessage();
}

void ControlGrisAudioProcessor::setNumberOfSources(int const numOfSources, bool const propagate)
{
    mSources.setSize(numOfSources);
    mParameters.state.setProperty("numberOfSources", mSources.size(), nullptr);

    for (auto & source : mSources) {
        source.setColorFromIndex(numOfSources);
    }

    mPositionSourceLinkEnforcer.numberOfSourcesChanged();
    mElevationSourceLinkEnforcer.numberOfSourcesChanged();

    if (propagate) {
        sendOscMessage();
    }
}

void ControlGrisAudioProcessor::setSelectedSource(SourceIndex const index)
{
    mSelectedSource = index;
}

//==============================================================================
bool ControlGrisAudioProcessor::createOscConnection(int oscPort)
{
    disconnectOSC();

    mOscConnected = mOscSender.connect("127.0.0.1", oscPort);
    if (!mOscConnected)
        std::cout << "Error: could not connect to UDP port " << oscPort << "." << std::endl;
    else
        mLastConnectedOSCPort = oscPort;

    return mOscConnected;
}

bool ControlGrisAudioProcessor::disconnectOSC()
{
    if (mOscConnected) {
        if (mOscSender.disconnect()) {
            mOscConnected = false;
            mLastConnectedOSCPort = -1;
        }
    }
    return !mOscConnected;
}

void ControlGrisAudioProcessor::handleOscConnection(bool state)
{
    if (state) {
        if (mLastConnectedOSCPort != mCurrentOSCPort) {
            createOscConnection(mCurrentOSCPort);
        }
    } else {
        disconnectOSC();
    }
    mParameters.state.setProperty("oscConnected", getOscConnected(), nullptr);
}

void ControlGrisAudioProcessor::sendOscMessage()
{
    if (!mOscConnected)
        return;

    OSCAddressPattern oscPattern("/spat/serv");
    OSCMessage message(oscPattern);

    for (auto const & source : mSources) {
        float const azimuth{ source.getAzimuth().getAsRadians() };
        float const elevation{ source.getElevation().getAsRadians() };
        float const azimuthSpan{ source.getAzimuthSpan() * 2.0f };
        float const elevationSpan{ source.getElevationSpan() * 0.5f };
        float const distance{ mSpatMode == SpatMode::cube ? source.getDistance() / 0.6f : source.getDistance() };

        // std::cout << "Sending osc for source #" << i << ":\n\tazimuth: " << azimuth << "\n\televation: " << elevation
        //           << "\n\tazimuthSpan: " << azimuthSpan << "\n\televationSpan" << elevationSpan
        //           << "\n\tdistance: " << distance << "\n\n";

        message.clear();
        message.addInt32(source.getId().toInt() - 1); // osc id starts at 0
        message.addFloat32(azimuth);
        message.addFloat32(elevation);
        message.addFloat32(azimuthSpan);
        message.addFloat32(elevationSpan);
        message.addFloat32(distance);
        message.addFloat32(0.0);

        auto const success{ mOscSender.send(message) };
        jassert(success);
    }
}

//==============================================================================
bool ControlGrisAudioProcessor::createOscInputConnection(int oscPort)
{
    disconnectOSCInput(oscPort);

    mOscInputConnected = mOscInputReceiver.connect(oscPort);
    if (!mOscInputConnected) {
        std::cout << "Error: could not connect to UDP input port " << oscPort << "." << std::endl;
    } else {
        mOscInputReceiver.addListener(this);
        mCurrentOSCInputPort = oscPort;
        mParameters.state.setProperty("oscInputPortNumber", oscPort, nullptr);
    }

    mParameters.state.setProperty("oscInputConnected", getOscInputConnected(), nullptr);

    return mOscInputConnected;
}

bool ControlGrisAudioProcessor::disconnectOSCInput(int oscPort)
{
    if (mOscInputConnected) {
        if (mOscInputReceiver.disconnect()) {
            mOscInputConnected = false;
        }
    }

    mParameters.state.setProperty("oscInputPortNumber", oscPort, nullptr);
    mParameters.state.setProperty("oscInputConnected", getOscInputConnected(), nullptr);

    return !mOscInputConnected;
}

void ControlGrisAudioProcessor::oscBundleReceived(const OSCBundle & bundle)
{
    for (auto const & element : bundle) {
        if (element.isMessage())
            oscMessageReceived(element.getMessage());
        else if (element.isBundle())
            oscBundleReceived(element.getBundle());
    }
}

void ControlGrisAudioProcessor::oscMessageReceived(const OSCMessage & message)
{
    PositionSourceLink positionSourceLinkToProcess{ PositionSourceLink::undefined };
    ElevationSourceLink elevationSourceLinkToProcess{ ElevationSourceLink::undefined };
    float x{ -1.0f };
    float y{ -1.0f };
    float z{ -1.0f };
    String const address{ message.getAddressPattern().toString() };
    String const pluginInstance{ String("/controlgris/") + String(getOscOutputPluginId()) };
    if ((address == String(pluginInstance + "/traj/1/x") || address == String(pluginInstance + "/traj/1/xyz/1"))
        && mPositionAutomationManager.getTrajectoryType() == PositionTrajectoryType::realtime) {
        x = message[0].getFloat32();
    } else if ((address == String(pluginInstance + "/traj/1/y") || address == String(pluginInstance + "/traj/1/xyz/2"))
               && mPositionAutomationManager.getTrajectoryType() == PositionTrajectoryType::realtime) {
        y = message[0].getFloat32();
    } else if ((address == String(pluginInstance + "/traj/1/z") || address == String(pluginInstance + "/traj/1/xyz/3"))
               && static_cast<ElevationTrajectoryType>(mElevationAutomationManager.getTrajectoryType())
                      == ElevationTrajectoryType::realtime) {
        z = message[0].getFloat32();
    } else if (address == String(pluginInstance + "/traj/1/xy")
               && mPositionAutomationManager.getTrajectoryType() == PositionTrajectoryType::realtime) {
        x = message[0].getFloat32();
        y = message[1].getFloat32();
    } else if (address == String(pluginInstance + "/traj/1/xyz")) {
        if (mPositionAutomationManager.getTrajectoryType() == PositionTrajectoryType::realtime) {
            x = message[0].getFloat32();
            y = message[1].getFloat32();
        }
        if (static_cast<ElevationTrajectoryType>(mElevationAutomationManager.getTrajectoryType())
            == ElevationTrajectoryType::realtime) {
            z = message[2].getFloat32();
        }
    } else if (address == String(pluginInstance + "/azispan")) {
        for (auto & source : mSources)
            source.setAzimuthSpan(Normalized{ message[0].getFloat32() });
        mParameters.getParameter("azimuthSpan")->beginChangeGesture();
        mParameters.getParameter("azimuthSpan")->setValueNotifyingHost(message[0].getFloat32());
        mParameters.getParameter("azimuthSpan")->endChangeGesture();
    } else if (address == String(pluginInstance + "/elespan")) {
        for (auto & source : mSources)
            source.setElevationSpan(Normalized{ message[0].getFloat32() });
        mParameters.getParameter("elevationSpan")->beginChangeGesture();
        mParameters.getParameter("elevationSpan")->setValueNotifyingHost(message[0].getFloat32());
        mParameters.getParameter("elevationSpan")->endChangeGesture();
    } else if (address == String(pluginInstance + "/sourcelink/1/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(1);
    } else if (address == String(pluginInstance + "/sourcelink/2/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(2);
    } else if (address == String(pluginInstance + "/sourcelink/3/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(3);
    } else if (address == String(pluginInstance + "/sourcelink/4/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(4);
    } else if (address == String(pluginInstance + "/sourcelink/5/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(5);
    } else if (address == String(pluginInstance + "/sourcelink/6/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(6);
    } else if (address == String(pluginInstance + "/sourcelink")) {
        positionSourceLinkToProcess = static_cast<PositionSourceLink>(message[0].getFloat32()); // 1 -> 6
    } else if (address == String(pluginInstance + "/sourcelinkalt/1/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(1);
    } else if (address == String(pluginInstance + "/sourcelinkalt/2/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(2);
    } else if (address == String(pluginInstance + "/sourcelinkalt/3/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(3);
    } else if (address == String(pluginInstance + "/sourcelinkalt/4/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(4);
    } else if (address == String(pluginInstance + "/sourcelinkalt/5/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(5);
    } else if (address == String(pluginInstance + "/sourcelinkalt")) {
        elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(message[0].getFloat32()); // 1 -> 5
    } else if (address == String(pluginInstance + "/presets")) {
        int newPreset = (int)message[0].getFloat32(); // 1 -> 50
        setPositionPreset(newPreset);
        ControlGrisAudioProcessorEditor * ed = dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor());
        if (ed != nullptr) {
            ed->updatePositionPreset(newPreset);
        }
    }

    if (x != -1.0f && y != -1.0f) {
        mSources.getPrimarySource().setPos(Point<float>{ x, y }, SourceLinkNotification::notify);
        sourcePositionChanged(SourceIndex{ 0 }, 0);
        setPositionPreset(0);
    } else if (y != -1.0f) {
        mSources.getPrimarySource().setY(y, SourceLinkNotification::notify);
        sourcePositionChanged(SourceIndex{ 0 }, 0);
        setPositionPreset(0);
    } else if (x != -1.0f) {
        mSources.getPrimarySource().setX(x, SourceLinkNotification::notify);
        sourcePositionChanged(SourceIndex{ 0 }, 0);
        setPositionPreset(0);
    }

    if (z != -1.0f) {
        mSources.getPrimarySource().setY(z, SourceLinkNotification::notify);
        mElevationAutomationManager.sendTrajectoryPositionChangedEvent();
        setPositionPreset(0);
    }

    if (static_cast<bool>(positionSourceLinkToProcess)) {
        setPositionSourceLink(positionSourceLinkToProcess);
    }

    if (static_cast<bool>(elevationSourceLinkToProcess)) {
        setElevationSourceLink(elevationSourceLinkToProcess);
    }
}

//==============================================================================
bool ControlGrisAudioProcessor::createOscOutputConnection(String const & oscAddress, int const oscPort)
{
    disconnectOSCOutput(oscAddress, oscPort);

    mOscOutputConnected = mOscOutputSender.connect(oscAddress, oscPort);
    if (!mOscOutputConnected)
        std::cout << "Error: could not connect to UDP output port " << oscPort << " on address " << oscAddress << "."
                  << std::endl;
    else {
        mCurrentOSCOutputPort = oscPort;
        mCurrentOSCOutputAddress = oscAddress;
        mParameters.state.setProperty("oscOutputPortNumber", oscPort, nullptr);
        mParameters.state.setProperty("oscOutputAddress", oscAddress, nullptr);
    }

    mParameters.state.setProperty("oscOutputConnected", getOscOutputConnected(), nullptr);

    return mOscOutputConnected;
}

bool ControlGrisAudioProcessor::disconnectOSCOutput(String oscAddress, int oscPort)
{
    if (mOscOutputConnected) {
        if (mOscOutputSender.disconnect()) {
            mOscOutputConnected = false;
        }
    }

    mParameters.state.setProperty("oscOutputPortNumber", oscPort, nullptr);
    mParameters.state.setProperty("oscOutputAddress", oscAddress, nullptr);
    mParameters.state.setProperty("oscOutputConnected", getOscOutputConnected(), nullptr);

    return !mOscOutputConnected;
}

void ControlGrisAudioProcessor::setOscOutputPluginId(int pluginId)
{
    mParameters.state.setProperty("oscOutputPluginId", pluginId, nullptr);
}

int ControlGrisAudioProcessor::getOscOutputPluginId() const
{
    return mParameters.state.getProperty("oscOutputPluginId", 1);
}

void ControlGrisAudioProcessor::sendOscOutputMessage()
{
    if (!mOscOutputConnected)
        return;

    OSCMessage message(OSCAddressPattern("/tmp"));

    String pluginInstance = String("/controlgris/") + String(getOscOutputPluginId());

    auto const trajectoryHandlePosition{ mSources.getPrimarySource().getPos() };
    float trajectory1x = trajectoryHandlePosition.getX();
    float trajectory1y = trajectoryHandlePosition.getY();
    float trajectory1z = trajectoryHandlePosition.getY();

    if (mLastTrajectory1x != trajectory1x) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/traj/1/x"));
        message.addFloat32(trajectory1x);
        mOscOutputSender.send(message);
        message.clear();

        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/traj/1/xyz/1"));
        message.addFloat32(trajectory1x);
        mOscOutputSender.send(message);
        message.clear();
    }

    if (mLastTrajectory1y != trajectory1y) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/traj/1/y"));
        message.addFloat32(trajectory1y);
        mOscOutputSender.send(message);
        message.clear();

        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/traj/1/xyz/2"));
        message.addFloat32(trajectory1y);
        mOscOutputSender.send(message);
        message.clear();
    }

    if (mLastTrajectory1z != trajectory1z) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/traj/1/z"));
        message.addFloat32(trajectory1z);
        mOscOutputSender.send(message);
        message.clear();

        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/traj/1/xyz/3"));
        message.addFloat32(trajectory1z);
        mOscOutputSender.send(message);
        message.clear();
    }

    if (mLastTrajectory1x != trajectory1x || mLastTrajectory1y != trajectory1y || mLastTrajectory1z != trajectory1z) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/traj/1/xy"));
        message.addFloat32(trajectory1x);
        message.addFloat32(trajectory1y);
        mOscOutputSender.send(message);
        message.clear();

        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/traj/1/xyz"));
        message.addFloat32(trajectory1x);
        message.addFloat32(trajectory1y);
        message.addFloat32(trajectory1z);
        mOscOutputSender.send(message);
        message.clear();
    }

    mLastTrajectory1x = trajectory1x;
    mLastTrajectory1y = trajectory1y;
    mLastTrajectory1z = trajectory1z;

    if (mLastAzispan != mSources.getPrimarySource().getAzimuthSpan()) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/azispan"));
        message.addFloat32(mSources.getPrimarySource().getAzimuthSpan().toFloat());
        mOscOutputSender.send(message);
        message.clear();
        mLastAzispan = mSources.getPrimarySource().getAzimuthSpan();
    }

    if (mLastElespan != mSources.getPrimarySource().getElevationSpan()) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/elespan"));
        message.addFloat32(mSources.getPrimarySource().getElevationSpan().toFloat());
        mOscOutputSender.send(message);
        message.clear();
        mLastElespan = mSources.getPrimarySource().getElevationSpan();
    }

    if (mPositionAutomationManager.getSourceLink() != mLastSourceLink) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/sourcelink"));
        message.addInt32(static_cast<int32>(mPositionAutomationManager.getSourceLink()));
        mOscOutputSender.send(message);
        message.clear();

        String pattern = pluginInstance + String("/sourcelink/")
                         + String(static_cast<int>(mPositionAutomationManager.getSourceLink())) + String("/1");
        message.setAddressPattern(OSCAddressPattern(pattern));
        message.addInt32(1);
        mOscOutputSender.send(message);
        message.clear();

        mLastSourceLink = mPositionAutomationManager.getSourceLink();
    }

    if (static_cast<ElevationSourceLink>(mElevationAutomationManager.getSourceLink()) != mLastElevationSourceLink) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/sourcelinkalt"));
        message.addInt32(static_cast<int32>(mElevationAutomationManager.getSourceLink()));
        mOscOutputSender.send(message);
        message.clear();

        String patternAlt = pluginInstance + String("/sourcelinkalt/")
                            + String(static_cast<int>(mElevationAutomationManager.getSourceLink())) + String("/1");
        message.setAddressPattern(OSCAddressPattern(patternAlt));
        message.addInt32(1);
        mOscOutputSender.send(message);
        message.clear();

        mLastElevationSourceLink = static_cast<ElevationSourceLink>(mElevationAutomationManager.getSourceLink());
    }

    if (mCurrentPositionPreset != mLastPositionPreset) {
        message.setAddressPattern(OSCAddressPattern(pluginInstance + "/presets"));
        message.addInt32(mCurrentPositionPreset);
        mOscOutputSender.send(message);
        message.clear();

        mLastPositionPreset = mCurrentPositionPreset;
    }
}

//==============================================================================
void ControlGrisAudioProcessor::timerCallback()
{
    auto * editor{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };

    // automation
    if (mLastTimerTime != getCurrentTime()) {
        auto const deltaTime{ getCurrentTime() - getInitTimeOnPlay() };
        if (mPositionAutomationManager.getPositionActivateState()) {
            mPositionAutomationManager.setTrajectoryDeltaTime(deltaTime);
        }
        if (mSpatMode == SpatMode::cube && mElevationAutomationManager.getPositionActivateState()) {
            mElevationAutomationManager.setTrajectoryDeltaTime(deltaTime);
        }
    }

    mLastTimerTime = getCurrentTime();

    if (mCanStopActivate && !mIsPlaying) {
        if (mPositionAutomationManager.getPositionActivateState())
            mPositionAutomationManager.setPositionActivateState(false);
        if (mElevationAutomationManager.getPositionActivateState())
            mElevationAutomationManager.setPositionActivateState(false);
        mCanStopActivate = false;

        if (editor != nullptr) {
            editor->updateSpanLinkButton(false);
        }
    }

    if (editor != nullptr) {
        editor->refresh();
    }

    sendOscMessage();
    sendOscOutputMessage();
}

//==============================================================================
void ControlGrisAudioProcessor::setPluginState()
{
    // If no preset is loaded, try to restore the last saved positions.
    if (mCurrentPositionPreset == 0) {
        for (auto & source : mSources) {
            auto const index{ source.getIndex().toString() };
            source.setAzimuth(Normalized{ mParameters.state.getProperty(String("p_azimuth_") + index) },
                              SourceLinkNotification::notify);
            source.setElevation(Normalized{ mParameters.state.getProperty(String("p_elevation_") + index) },
                                SourceLinkNotification::notify);
            source.setDistance(mParameters.state.getProperty(String("p_distance_") + index),
                               SourceLinkNotification::notify);
        }
    }

    auto * editor{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
    if (editor != nullptr) {
        editor->setPluginState();
    }

    sendOscMessage();
}

//==============================================================================
void ControlGrisAudioProcessor::sourcePositionChanged(SourceIndex sourceIndex, int whichField)
{
    auto const & source{ mSources[sourceIndex] };
    if (whichField == 0) {
        if (getSpatMode() == SpatMode::dome) {
            setSourceParameterValue(sourceIndex, SourceParameter::azimuth, source.getNormalizedAzimuth().toFloat());
            setSourceParameterValue(sourceIndex, SourceParameter::elevation, source.getNormalizedElevation().toFloat());
        } else {
            setSourceParameterValue(sourceIndex, SourceParameter::azimuth, source.getNormalizedAzimuth().toFloat());
            setSourceParameterValue(sourceIndex, SourceParameter::distance, source.getDistance());
        }
        if (source.isPrimarySource()) {
            mPositionAutomationManager.setTrajectoryType(mPositionAutomationManager.getTrajectoryType(),
                                                         mSources.getPrimarySource().getPos());
        }
    } else {
        setSourceParameterValue(sourceIndex, SourceParameter::elevation, source.getNormalizedElevation().toFloat());
        mElevationAutomationManager.setTrajectoryType(mElevationAutomationManager.getTrajectoryType());
    }

    //    // any position change invalidates current preset.
    //    setPositionPreset(0);
}

// Called whenever a source has changed.
//--------------------------------------
void ControlGrisAudioProcessor::setSourceParameterValue(SourceIndex const sourceIndex,
                                                        SourceParameter const parameterId,
                                                        float const value)
{
    if (sourceIndex != SourceIndex{ 0 }) {
        return; // TODO : why do we have to do this?
    }
    Normalized const normalized{ static_cast<float>(value) };
    auto const param_id{ sourceIndex.toString() };
    auto & source{ mSources[sourceIndex] };
    switch (parameterId) {
    case SourceParameter::azimuth:
        source.setAzimuth(normalized, SourceLinkNotification::notify);
        mParameters.state.setProperty("p_azimuth_" + param_id, value, nullptr);
        break;
    case SourceParameter::elevation:
        source.setElevation(normalized, SourceLinkNotification::notify);
        mParameters.state.setProperty(String("p_elevation_") + param_id, value, nullptr);
        break;
    case SourceParameter::distance:
        source.setDistance(value, SourceLinkNotification::notify);
        mParameters.state.setProperty(String("p_distance_") + param_id, value, nullptr);
        break;
    case SourceParameter::x:
        source.setX(value, SourceLinkNotification::notify);
        break;
    case SourceParameter::y:
        source.setY(value, SourceLinkNotification::notify);
        break;
    case SourceParameter::azimuthSpan:
        for (auto & source : mSources) {
            source.setAzimuthSpan(normalized);
        }
        mParameters.getParameter("azimuthSpan")->setValueNotifyingHost(value);
        break;
    case SourceParameter::elevationSpan:
        for (auto & source : mSources) {
            source.setElevationSpan(normalized);
        }
        mParameters.getParameter("elevationSpan")->setValueNotifyingHost(value);
        break;
    }
}

void ControlGrisAudioProcessor::beginSourcePositionChangeGesture() const
{
    mParameters.getParameter("recordingTrajectory_x")->beginChangeGesture();
    mParameters.getParameter("recordingTrajectory_y")->beginChangeGesture();
}

void ControlGrisAudioProcessor::endSourcePositionChangeGesture() const
{
    mParameters.getParameter("recordingTrajectory_x")->endChangeGesture();
    mParameters.getParameter("recordingTrajectory_y")->endChangeGesture();
}

void ControlGrisAudioProcessor::beginSourceElevationChangeGesture() const
{
    mParameters.getParameter("recordingTrajectory_z")->beginChangeGesture();
}

void ControlGrisAudioProcessor::endSourceElevationChangeGesture() const
{
    mParameters.getParameter("recordingTrajectory_z")->endChangeGesture();
}

void ControlGrisAudioProcessor::beginAzimuthSpanChangeGesture() const
{
    mParameters.getParameter("azimuthSpan")->beginChangeGesture();
}

void ControlGrisAudioProcessor::endAzimuthSpanChangeGesture() const
{
    mParameters.getParameter("azimuthSpan")->endChangeGesture();
}

void ControlGrisAudioProcessor::beginElevationSpanChangeGesture() const
{
    mParameters.getParameter("elevationSpan")->beginChangeGesture();
}

void ControlGrisAudioProcessor::endElevationSpanChangeGesture() const
{
    mParameters.getParameter("elevationSpan")->endChangeGesture();
}

void ControlGrisAudioProcessor::trajectoryPositionChanged(AutomationManager * manager,
                                                          Point<float> const position,
                                                          Radians const elevation)
{
    // TODO: change gestures might have to be initiated some other way ? (osc input)
    auto const normalizedPosition{ (position + Point<float>{ 1.0f, 1.0f }) / 2.0f };
    if (manager == &mPositionAutomationManager) {
        if (!isPlaying()) {
            mPositionAutomationManager.setPrincipalSourceAndPlaybackPosition(position);
            mParameters.getParameter("recordingTrajectory_x")->setValue(normalizedPosition.getX());
            mParameters.getParameter("recordingTrajectory_y")->setValue(normalizedPosition.getY());
        }

        mParameters.getParameter("recordingTrajectory_x")->setValueNotifyingHost(normalizedPosition.getX());
        mParameters.getParameter("recordingTrajectory_y")->setValueNotifyingHost(normalizedPosition.getY());

    } else if (manager == &mElevationAutomationManager) {
        auto const normalizedElevation{ 1.0f - elevation / MAX_ELEVATION };
        if (!isPlaying()) {
            mParameters.getParameter("recordingTrajectory_z")->setValue(normalizedElevation);
        }

        mParameters.getParameter("recordingTrajectory_z")->setValueNotifyingHost(normalizedElevation);
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setPositionPreset(int const presetNumber)
{
    if (presetNumber != 0 && presetNumber != mCurrentPositionPreset) {
        mCurrentPositionPreset = presetNumber;
        mParameters.getParameter("positionPreset")->beginChangeGesture();
        auto const normalizedPresetNumber{ static_cast<float>(presetNumber)
                                           / static_cast<float>(NUMBER_OF_POSITION_PRESETS) };
        mParameters.getParameter("positionPreset")->setValueNotifyingHost(normalizedPresetNumber);
        mParameters.getParameter("positionPreset")->endChangeGesture();
        recallFixedPosition(presetNumber);
    } else {
        mCurrentPositionPreset = presetNumber;
        mParameters.getParameter("positionPreset")
            ->setValue(static_cast<float>(presetNumber) / static_cast<float>(NUMBER_OF_POSITION_PRESETS));
    }
}

//==============================================================================
void ControlGrisAudioProcessor::addNewFixedPosition(int const id)
{
    // Build a new fixed position element.
    auto * newData = new XmlElement("ITEM");
    newData->setAttribute("ID", id);
    auto const positionSnapshots{ mPositionSourceLinkEnforcer.getSnapshots() };
    auto const elevationSnapshots{ mElevationSourceLinkEnforcer.getSnapshots() };
    SourceIndex const numberOfSources{ positionSnapshots.size() };
    for (SourceIndex sourceIndex{}; sourceIndex < numberOfSources; ++sourceIndex) {
        auto const xName{ getFixedPosSourceName(FixedPositionType::initial, sourceIndex, 0) };
        auto const yName{ getFixedPosSourceName(FixedPositionType::initial, sourceIndex, 1) };
        auto const zName{ getFixedPosSourceName(FixedPositionType::initial, sourceIndex, 2) };

        auto const position{ positionSnapshots[sourceIndex].position };
        auto const zValue{ elevationSnapshots[sourceIndex].z / MAX_ELEVATION };

        newData->setAttribute(xName, position.getX());
        newData->setAttribute(yName, position.getY());
        newData->setAttribute(zName, zValue);
    }

    auto const xName{ getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 0) };
    auto const yName{ getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 1) };
    auto const zName{ getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 2) };

    auto const position{ mSources.getPrimarySource().getPos() };
    auto const zValue{ mSources.getPrimarySource().getElevation() / MAX_ELEVATION };

    newData->setAttribute(xName, position.getX());
    newData->setAttribute(yName, position.getY());
    newData->setAttribute(zName, zValue);

    // Replace an element if the new one has the same ID as one already saved.
    bool found{ false };
    XmlElement * fpos = mFixPositionData.getFirstChildElement();
    while (fpos) {
        if (fpos->getIntAttribute("ID") == id) {
            found = true;
            break;
        }
        fpos = fpos->getNextElement();
    }

    if (found) {
        mFixPositionData.replaceChildElement(fpos, newData);
    } else {
        mFixPositionData.addChildElement(newData);
    }

    XmlElementDataSorter sorter("ID", true);
    mFixPositionData.sortChildElements(sorter);
}

bool ControlGrisAudioProcessor::recallFixedPosition(int id)
{
    // TODO : this is a mess
    bool found = false;
    XmlElement * fpos = mFixPositionData.getFirstChildElement();
    while (fpos) {
        if (fpos->getIntAttribute("ID") == id) {
            found = true;
            break;
        }
        fpos = fpos->getNextElement();
    }

    if (!found) {
        return false;
    }

    mCurrentFixPosition = fpos;
    SourcesSnapshots snapshots{};
    for (auto & source : mSources) {
        SourceSnapshot snapshot{};
        auto const index{ source.getIndex() };
        auto const xPosId{ getFixedPosSourceName(FixedPositionType::initial, index, 0) };
        auto const yPosId{ getFixedPosSourceName(FixedPositionType::initial, index, 1) };
        if (mCurrentFixPosition->hasAttribute(xPosId) && mCurrentFixPosition->hasAttribute(yPosId)) {
            Point<float> const position{ static_cast<float>(mCurrentFixPosition->getDoubleAttribute(xPosId)),
                                         static_cast<float>(mCurrentFixPosition->getDoubleAttribute(yPosId)) };
            snapshot.position = position;
            auto const zPosId{ getFixedPosSourceName(FixedPositionType::initial, index, 2) };
            if (mCurrentFixPosition->hasAttribute(zPosId)) {
                auto const normalizedElevation{ static_cast<float>(mCurrentFixPosition->getDoubleAttribute(
                    getFixedPosSourceName(FixedPositionType::initial, index, 2))) };
                snapshot.z = MAX_ELEVATION * normalizedElevation;
            }
        }
        if (source.isPrimarySource()) {
            snapshots.primary = snapshot;
        } else {
            snapshots.secondaries.add(snapshot);
        }
    }

    mPositionSourceLinkEnforcer.loadSnapshots(snapshots);
    if (mSpatMode == SpatMode::cube) {
        mElevationSourceLinkEnforcer.loadSnapshots(snapshots);
    }

    auto const xTerminalPositionId{ getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 0) };
    auto const yTerminalPositionId{ getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 1) };
    auto const zTerminalPositionId{ getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 2) };

    Point<float> terminalPosition{};
    if (mCurrentFixPosition->hasAttribute(xTerminalPositionId)
        && mCurrentFixPosition->hasAttribute(yTerminalPositionId)) {
        terminalPosition.setXY(static_cast<float>(mCurrentFixPosition->getDoubleAttribute(xTerminalPositionId)),
                               static_cast<float>(mCurrentFixPosition->getDoubleAttribute(yTerminalPositionId)));
    } else {
        terminalPosition = snapshots.primary.position;
    }
    mSources.getPrimarySource().setPos(terminalPosition, SourceLinkNotification::notify);

    if (mSpatMode == SpatMode::cube) {
        Radians elevation{};
        if (mCurrentFixPosition->hasAttribute(zTerminalPositionId)) {
            elevation
                = MAX_ELEVATION * static_cast<float>(mCurrentFixPosition->getDoubleAttribute(zTerminalPositionId));
        } else {
            elevation = snapshots.primary.z;
        };
        mSources.getPrimarySource().setElevation(elevation, SourceLinkNotification::notify);
    }

    // refresh trajectory
    mPositionAutomationManager.setTrajectoryType(mPositionAutomationManager.getTrajectoryType(),
                                                 mSources.getPrimarySource().getPos());

    return true;
}

void ControlGrisAudioProcessor::copyFixedPositionXmlElement(XmlElement * src, XmlElement * dest)
{
    if (dest == nullptr)
        dest = new XmlElement(FIXED_POSITION_DATA_TAG);

    forEachXmlChildElement(*src, element)
    {
        auto * newData = new XmlElement("ITEM");
        newData->setAttribute("ID", element->getIntAttribute("ID"));
        for (SourceIndex i{}; i < SourceIndex{ MAX_NUMBER_OF_SOURCES }; ++i) {
            newData->setAttribute(getFixedPosSourceName(FixedPositionType::initial, i, 0),
                                  element->getDoubleAttribute(getFixedPosSourceName(FixedPositionType::initial, i, 0)));
            newData->setAttribute(getFixedPosSourceName(FixedPositionType::initial, i, 1),
                                  element->getDoubleAttribute(getFixedPosSourceName(FixedPositionType::initial, i, 1)));
            newData->setAttribute(getFixedPosSourceName(FixedPositionType::initial, i, 2),
                                  element->getDoubleAttribute(getFixedPosSourceName(FixedPositionType::initial, i, 2)));
        }
        newData->setAttribute(
            getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 0),
            element->getDoubleAttribute(getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 0)));
        newData->setAttribute(
            getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 1),
            element->getDoubleAttribute(getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 1)));
        newData->setAttribute(
            getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 2),
            element->getDoubleAttribute(getFixedPosSourceName(FixedPositionType::terminal, SourceIndex{ 0 }, 2)));

        dest->addChildElement(newData);
    }
}

void ControlGrisAudioProcessor::deleteFixedPosition(int id)
{
    bool found = false;
    XmlElement * fpos = mFixPositionData.getFirstChildElement();
    while (fpos) {
        if (fpos->getIntAttribute("ID") == id) {
            found = true;
            break;
        }
        fpos = fpos->getNextElement();
    }

    if (found) {
        mFixPositionData.removeChildElement(fpos, true);
        XmlElementDataSorter sorter("ID", true);
        mFixPositionData.sortChildElements(sorter);
    }
}

//==============================================================================
String const ControlGrisAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ControlGrisAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ControlGrisAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ControlGrisAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

//==============================================================================
void ControlGrisAudioProcessor::initialize()
{
    mNeedsInitialization = true;
    mLastTime = mLastTimerTime = 10000000.0;
    mCanStopActivate = true;

    // TODO : is this necessary?
    // If a preset is actually selected, we always recall it on initialize because
    // the automation won't trigger parameterChanged if it stays on the same value.
    //    if (mCurrentPositionPreset != 0) {
    //        MessageManagerLock const messageManagerLock{};
    //        if (recallFixedPosition(mCurrentPositionPreset)) {
    //            auto * ed = dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor());
    //            if (ed != nullptr) {
    //                ed->updatePositionPreset(mCurrentPositionPreset);
    //            }
    //        }
    //    }
}

void ControlGrisAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (mIsPlaying == 0)
        initialize();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ControlGrisAudioProcessor::isBusesLayoutSupported(const BusesLayout & layouts) const
{
    #if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
    return true;
    #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

            // This checks if the input layout matches the output layout
        #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
        #endif

    return true;
    #endif
}
#endif

void ControlGrisAudioProcessor::processBlock(AudioBuffer<float> & buffer, MidiBuffer & midiMessages)
{
    bool const isPositionTrajectoryActive{ mPositionAutomationManager.getPositionActivateState() };
    bool const isElevationTrajectoryActive{ mElevationAutomationManager.getPositionActivateState() };

    auto const wasPlaying{ mIsPlaying };
    AudioPlayHead * audioPlayHead = getPlayHead();
    if (audioPlayHead != nullptr) {
        AudioPlayHead::CurrentPositionInfo currentPositionInfo{};
        audioPlayHead->getCurrentPosition(currentPositionInfo);
        mIsPlaying = currentPositionInfo.isPlaying;
        mBpm = currentPositionInfo.bpm;
        if (mNeedsInitialization) {
            mInitTimeOnPlay = mCurrentTime
                = currentPositionInfo.timeInSeconds < 0.0 ? 0.0 : currentPositionInfo.timeInSeconds;
            mNeedsInitialization = false;
        } else {
            mCurrentTime = currentPositionInfo.timeInSeconds;
        }
    }

    if (!wasPlaying && mIsPlaying) { // Initialization here only for Logic (also Reaper and Live), which are not
        PluginHostType hostType;     // calling prepareToPlay every time the sequence starts.
        if (hostType.isLogic() || hostType.isReaper() || hostType.isAbletonLive()) {
            initialize();
        }
    }

    static bool positionGestureStarted{ false };
    static bool elevationGestureStarted{ false };
    if (isPositionTrajectoryActive && mIsPlaying && !positionGestureStarted) {
        positionGestureStarted = true;
        beginSourcePositionChangeGesture();
    } else if ((!isPositionTrajectoryActive || !mIsPlaying) && positionGestureStarted) {
        positionGestureStarted = false;
        endSourcePositionChangeGesture();
    }
    if (mSpatMode == SpatMode::cube) {
        if (isElevationTrajectoryActive && mIsPlaying && !elevationGestureStarted) {
            elevationGestureStarted = true;
            beginSourceElevationChangeGesture();
        } else if ((!isElevationTrajectoryActive || !mIsPlaying) && elevationGestureStarted) {
            elevationGestureStarted = false;
            endSourceElevationChangeGesture();
        }
    }
    mLastTime = mCurrentTime;
}

//==============================================================================
AudioProcessorEditor * ControlGrisAudioProcessor::createEditor()
{
    return new ControlGrisAudioProcessorEditor(*this,
                                               mParameters,
                                               mPositionAutomationManager,
                                               mElevationAutomationManager);
}

//==============================================================================
void ControlGrisAudioProcessor::getStateInformation(MemoryBlock & destData)
{
    for (int i{}; i < MAX_NUMBER_OF_SOURCES; ++i) {
        String id(i);
        mParameters.state.setProperty(String("p_azimuth_") + id, mSources[i].getNormalizedAzimuth().toFloat(), nullptr);
        mParameters.state.setProperty(String("p_elevation_") + id,
                                      mSources[i].getNormalizedElevation().toFloat(),
                                      nullptr);
        mParameters.state.setProperty(String("p_distance_") + id, mSources[i].getDistance(), nullptr);
    }

    auto state = mParameters.copyState();

    auto xmlState{ state.createXml() };

    if (xmlState != nullptr) {
        auto * childExist = xmlState->getChildByName(FIXED_POSITION_DATA_TAG);
        if (childExist) {
            xmlState->removeChildElement(childExist, true);
        }
        if (mFixPositionData.getNumChildElements() > 0) {
            auto * positionData = xmlState->createNewChildElement(FIXED_POSITION_DATA_TAG);
            copyFixedPositionXmlElement(&mFixPositionData, positionData);
        }
        copyXmlToBinary(*xmlState, destData);
    }
}

void ControlGrisAudioProcessor::setStateInformation(const void * data, int sizeInBytes)
{
    MessageManagerLock mmLock{};

    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr) {
        // Set global settings values.
        //----------------------------
        ValueTree valueTree = ValueTree::fromXml(*xmlState);
        setSpatMode((SpatMode)(int)valueTree.getProperty("oscFormat", 0));
        setOscPortNumber(valueTree.getProperty("oscPortNumber", 18032));
        handleOscConnection(valueTree.getProperty("oscConnected", true));
        setNumberOfSources(valueTree.getProperty("numberOfSources", 1), false);
        setFirstSourceId(SourceId{ valueTree.getProperty("firstSourceId", 1) });
        setOscOutputPluginId(valueTree.getProperty("oscOutputPluginId", 1));

        if (valueTree.getProperty("oscInputConnected", false)) {
            createOscInputConnection(valueTree.getProperty("oscInputPortNumber", 9000));
        }

        if (valueTree.getProperty("oscOutputConnected", false)) {
            createOscOutputConnection(valueTree.getProperty("oscOutputAddress", "192.168.1.100"),
                                      valueTree.getProperty("oscOutputPortNumber", 8000));
        }

        // Load saved fixed positions.
        //----------------------------
        XmlElement * positionData = xmlState->getChildByName(FIXED_POSITION_DATA_TAG);
        if (positionData) {
            mFixPositionData.deleteAllChildElements();
            copyFixedPositionXmlElement(positionData, &mFixPositionData);
        }
        // Replace the state and call automated parameter current values.
        //---------------------------------------------------------------
        mParameters.replaceState(ValueTree::fromXml(*xmlState));
    }

    setPluginState();
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor * JUCE_CALLTYPE createPluginFilter()
{
    return new ControlGrisAudioProcessor();
}
