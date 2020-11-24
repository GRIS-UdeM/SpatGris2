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

#include "cg_ControlGrisAudioProcessor.hpp"

#include "cg_ControlGrisAudioProcessorEditor.hpp"
#include "cg_TrajectoryManager.hpp"

namespace gris
{
enum class FixedPositionType { terminal, initial };

//==============================================================================
// The parameter Layout creates the automatable mAudioProcessorValueTreeState.
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Parameter = juce::AudioProcessorValueTreeState::Parameter;

    std::vector<std::unique_ptr<Parameter>> parameters;

    parameters.push_back(std::make_unique<Parameter>(Automation::Ids::X,
                                                     juce::String("Recording Trajectory X"),
                                                     juce::String(),
                                                     juce::NormalisableRange<float>(0.0f, 1.0f),
                                                     0.0f,
                                                     nullptr,
                                                     nullptr));
    parameters.push_back(std::make_unique<Parameter>(Automation::Ids::Y,
                                                     juce::String("Recording Trajectory Y"),
                                                     juce::String(),
                                                     juce::NormalisableRange<float>(0.0f, 1.0f),
                                                     0.0f,
                                                     nullptr,
                                                     nullptr));
    parameters.push_back(std::make_unique<Parameter>(Automation::Ids::Z,
                                                     juce::String("Recording Trajectory Z"),
                                                     juce::String(),
                                                     juce::NormalisableRange<float>(0.0f, 1.0f),
                                                     0.0f,
                                                     nullptr,
                                                     nullptr));

    parameters.push_back(std::make_unique<Parameter>(
        Automation::Ids::POSITION_SOURCE_LINK,
        juce::String("Source Link"),
        juce::String(),
        juce::NormalisableRange<float>(0.0f, static_cast<float>(POSITION_SOURCE_LINK_TYPES.size() - 1), 1.0f),
        0.0f,
        nullptr,
        nullptr,
        false,
        true,
        true));
    parameters.push_back(std::make_unique<Parameter>(Automation::Ids::ELEVATION_SOURCE_LINK,
                                                     juce::String("Source Link Alt"),
                                                     juce::String(),
                                                     juce::NormalisableRange<float>(0.0f, 4.0f, 1.0f),
                                                     0.0f,
                                                     nullptr,
                                                     nullptr,
                                                     false,
                                                     true,
                                                     true));

    parameters.push_back(std::make_unique<Parameter>(Automation::Ids::POSITION_PRESET,
                                                     juce::String("Position Preset"),
                                                     juce::String(),
                                                     juce::NormalisableRange<float>(0.0f, 50.0f, 1.0f),
                                                     0.0f,
                                                     nullptr,
                                                     nullptr,
                                                     false,
                                                     true,
                                                     true));

    parameters.push_back(std::make_unique<Parameter>(Automation::Ids::AZIMUTH_SPAN,
                                                     juce::String("Azimuth Span"),
                                                     juce::String(),
                                                     juce::NormalisableRange<float>(0.0f, 1.0f),
                                                     0.0f,
                                                     nullptr,
                                                     nullptr));
    parameters.push_back(std::make_unique<Parameter>(Automation::Ids::ELEVATION_SPAN,
                                                     juce::String("Elevation Span"),
                                                     juce::String(),
                                                     juce::NormalisableRange<float>(0.0f, 1.0f),
                                                     0.0f,
                                                     nullptr,
                                                     nullptr));

    return { parameters.begin(), parameters.end() };
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
    mAudioProcessorValueTreeState(*this, nullptr, juce::Identifier(JucePlugin_Name), createParameterLayout())

{
    setLatencySamples(0);

    // Size of the plugin window.
    mAudioProcessorValueTreeState.state.addChild({ "uiState", { { "width", 650 }, { "height", 700 } }, {} },
                                                 -1,
                                                 nullptr);

    // Global setting mAudioProcessorValueTreeState.
    mAudioProcessorValueTreeState.state.setProperty("oscFormat", 0, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscPortNumber", 18032, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscConnected", true, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscInputPortNumber", 9000, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscInputConnected", false, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscOutputAddress", "192.168.1.100", nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscOutputPortNumber", 8000, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscOutputConnected", false, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("numberOfSources", 2, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("firstSourceId", 1, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscOutputPluginId", 1, nullptr);

    // Trajectory box persitent settings.
    mAudioProcessorValueTreeState.state.setProperty("trajectoryType",
                                                    static_cast<int>(PositionTrajectoryType::realtime),
                                                    nullptr);
    mAudioProcessorValueTreeState.state.setProperty("trajectoryTypeAlt",
                                                    static_cast<int>(ElevationTrajectoryType::realtime),
                                                    nullptr);
    mAudioProcessorValueTreeState.state.setProperty("backAndForth", false, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("backAndForthAlt", false, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("dampeningCycles", 0, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("dampeningCyclesAlt", 0, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("deviationPerCycle", 0, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("cycleDuration", 5, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("durationUnit", 1, nullptr);

    mSources.init(this);
    mPositionSourceLinkEnforcer.numberOfSourcesChanged();
    mElevationSourceLinkEnforcer.numberOfSourcesChanged();
    // Per source mAudioProcessorValueTreeState. Because there is no attachment to the automatable
    // mAudioProcessorValueTreeState, we need to keep track of the current parameter values to be
    // able to reload the last state of the plugin when we close/open the UI.
    for (int i{}; i < MAX_NUMBER_OF_SOURCES; ++i) {
        juce::String oscId(i);
        // Non-automatable, per source, mAudioProcessorValueTreeState.
        mAudioProcessorValueTreeState.state.setProperty(juce::String("p_azimuth_") + oscId,
                                                        i % 2 == 0 ? -90.0 : 90.0,
                                                        nullptr);
        mAudioProcessorValueTreeState.state.setProperty(juce::String("p_elevation_") + oscId, 0.0, nullptr);
        mAudioProcessorValueTreeState.state.setProperty(juce::String("p_distance_") + oscId, 1.0, nullptr);
        mAudioProcessorValueTreeState.state.setProperty(juce::String("p_x_") + oscId, 0.0, nullptr);
        mAudioProcessorValueTreeState.state.setProperty(juce::String("p_y_") + oscId, 0.0, nullptr);

        // Gives the source an initial id...
        auto & source{ mSources.get(i) };
        source.setId(SourceId{ i + mFirstSourceId.toInt() });
        // .. and colour
        source.setColorFromIndex(mSources.size());
        // .. and coordinates.
        auto const azimuth{ i % 2 == 0 ? Degrees{ -45.0f } : Degrees{ 45.0f } };
        source.setCoordinates(azimuth, MAX_ELEVATION, 1.0f, Source::OriginOfChange::userAnchorMove);
    }

    auto * paramX{ mAudioProcessorValueTreeState.getParameter(Automation::Ids::X) };
    if (paramX != nullptr) {
        paramX->setValue(mSources.getPrimarySource().getPos().getX());
    }
    auto * paramY{ mAudioProcessorValueTreeState.getParameter(Automation::Ids::Y) };
    if (paramY != nullptr) {
        paramY->setValue(mSources.getPrimarySource().getPos().y);
    }

    // Automation values for the recording trajectory.
    mAudioProcessorValueTreeState.addParameterListener(Automation::Ids::X, this);
    mAudioProcessorValueTreeState.addParameterListener(Automation::Ids::Y, this);
    mAudioProcessorValueTreeState.addParameterListener(Automation::Ids::Z, this);
    mAudioProcessorValueTreeState.addParameterListener(Automation::Ids::POSITION_SOURCE_LINK, this);
    mAudioProcessorValueTreeState.addParameterListener(Automation::Ids::ELEVATION_SOURCE_LINK, this);
    mAudioProcessorValueTreeState.addParameterListener(Automation::Ids::POSITION_PRESET, this);
    mAudioProcessorValueTreeState.addParameterListener(Automation::Ids::AZIMUTH_SPAN, this);
    mAudioProcessorValueTreeState.addParameterListener(Automation::Ids::ELEVATION_SPAN, this);

    handleOscConnection(true);

    // The timer's callback send OSC messages periodically.
    //-----------------------------------------------------
    startTimerHz(50);
}

//==============================================================================
ControlGrisAudioProcessor::~ControlGrisAudioProcessor()
{
    disconnectOsc();
}

//==============================================================================
void ControlGrisAudioProcessor::parameterChanged(juce::String const & parameterId, float const newValue)
{
    if (std::isnan(newValue) || std::isinf(newValue)) {
        jassertfalse;
        return;
    }

    Normalized const normalized{ newValue };
    if (parameterId.compare(Automation::Ids::X) == 0) {
        mSources.getPrimarySource().setX(normalized, Source::OriginOfChange::automation);
    } else if (parameterId.compare(Automation::Ids::Y) == 0) {
        Normalized const invNormalized{ 1.0f - newValue };
        mSources.getPrimarySource().setY(invNormalized, Source::OriginOfChange::automation);
    } else if (parameterId.compare(Automation::Ids::Z) == 0 && mSpatMode == SpatMode::cube) {
        auto const newElevation{ MAX_ELEVATION - (MAX_ELEVATION * normalized.toFloat()) };
        mSources.getPrimarySource().setElevation(newElevation, Source::OriginOfChange::automation);
    }

    if (parameterId.compare(Automation::Ids::POSITION_SOURCE_LINK) == 0) {
        auto const val{ static_cast<PositionSourceLink>(newValue + 1.0f) };
        setPositionSourceLink(val, SourceLinkEnforcer::OriginOfChange::automation);
    }

    if (parameterId.compare(Automation::Ids::ELEVATION_SOURCE_LINK) == 0) {
        auto const val{ static_cast<ElevationSourceLink>(newValue + 1.0f) };
        setElevationSourceLink(val, SourceLinkEnforcer::OriginOfChange::automation);
    }

    if (parameterId.compare(Automation::Ids::POSITION_PRESET) == 0) {
        auto const value{ static_cast<int>(newValue) };
        mPresetManager.loadIfPresetChanged(value);
    }

    if (parameterId.startsWith(Automation::Ids::AZIMUTH_SPAN)) {
        for (auto & source : mSources) {
            source.setAzimuthSpan(normalized);
        }
    } else if (parameterId.startsWith(Automation::Ids::ELEVATION_SPAN)) {
        for (auto & source : mSources) {
            source.setElevationSpan(normalized);
        }
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setPositionSourceLink(PositionSourceLink newSourceLink,
                                                      SourceLinkEnforcer::OriginOfChange const originOfChange)
{
    if (newSourceLink == mPositionTrajectoryManager.getSourceLink()) {
        return;
    }

    // TODO : this doesnt work at all
    auto const isSymmetricLink{ newSourceLink == PositionSourceLink::symmetricX
                                || newSourceLink == PositionSourceLink::symmetricY };
    if (mSources.size() != 2 && isSymmetricLink) {
        newSourceLink = PositionSourceLink::independent;
    }

    mPositionTrajectoryManager.setSourceLink(newSourceLink);

    auto * editor{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
    if (editor != nullptr) {
        editor->updateSourceLinkCombo(newSourceLink);
    }

    mPositionSourceLinkEnforcer.setSourceLink(newSourceLink, originOfChange);
    mPositionSourceLinkEnforcer.enforceSourceLink();
}

//==============================================================================
void ControlGrisAudioProcessor::setElevationSourceLink(ElevationSourceLink const newSourceLink,
                                                       SourceLinkEnforcer::OriginOfChange const originOfChange)
{
    if (newSourceLink != mElevationTrajectoryManager.getSourceLink()) {
        mElevationTrajectoryManager.setSourceLink(newSourceLink);

        auto * ed{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
        if (ed != nullptr) {
            ed->updateElevationSourceLinkCombo(newSourceLink);
        }

        mElevationSourceLinkEnforcer.setSourceLink(newSourceLink, originOfChange);
        mElevationSourceLinkEnforcer.enforceSourceLink();
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setSpatMode(SpatMode const spatMode)
{
    mSpatMode = spatMode;
    mAudioProcessorValueTreeState.state.setProperty("oscFormat", static_cast<int>(mSpatMode), nullptr);
    for (int i{}; i < MAX_NUMBER_OF_SOURCES; ++i) {
        mSources.get(i).setSpatMode(spatMode);
    }

    if (spatMode == SpatMode::dome) {
        // remove cube-specific gadgets
        mElevationSourceLinkEnforcer.setSourceLink(ElevationSourceLink::independent,
                                                   SourceLinkEnforcer::OriginOfChange::automation);
    } else {
        jassert(spatMode == SpatMode::cube);
        mElevationSourceLinkEnforcer.setSourceLink(mElevationTrajectoryManager.getSourceLink(),
                                                   SourceLinkEnforcer::OriginOfChange::automation);
    }

    auto * editor{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
    if (editor) {
        editor->setSpatMode(spatMode);
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setOscPortNumber(int const oscPortNumber)
{
    mCurrentOscPort = oscPortNumber;
    mAudioProcessorValueTreeState.state.setProperty("oscPortNumber", mCurrentOscPort, nullptr);
}

//==============================================================================
void ControlGrisAudioProcessor::setFirstSourceId(SourceId const firstSourceId, bool const propagate)
{
    mFirstSourceId = firstSourceId;
    mAudioProcessorValueTreeState.state.setProperty("firstSourceId", mFirstSourceId.toInt(), nullptr);
    for (int i{}; i < MAX_NUMBER_OF_SOURCES; ++i) {
        mSources.get(i).setId(SourceId{ i + mFirstSourceId.toInt() });
    }

    if (propagate) {
        sendOscMessage();
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setNumberOfSources(int const numOfSources, bool const propagate)
{
    if (numOfSources == mSources.size()) {
        return;
    }

    mSources.setSize(numOfSources);
    mAudioProcessorValueTreeState.state.setProperty("numberOfSources", mSources.size(), nullptr);

    mPositionSourceLinkEnforcer.numberOfSourcesChanged();
    mElevationSourceLinkEnforcer.numberOfSourcesChanged();

    mPresetManager.numberOfSourcesChanged();

    auto const positionSourceLink{ mPositionTrajectoryManager.getSourceLink() };
    auto const isSymmetricLink{ positionSourceLink == PositionSourceLink::symmetricX
                                || positionSourceLink == PositionSourceLink::symmetricY };
    if (isSymmetricLink && numOfSources != 2) {
        setPositionSourceLink(PositionSourceLink::independent, SourceLinkEnforcer::OriginOfChange::automation);
    }

    if (propagate) {
        sendOscMessage();
    }
}

//==============================================================================
bool ControlGrisAudioProcessor::createOscConnection(int const oscPort)
{
    disconnectOsc();

    mOscConnected = mOscSender.connect("127.0.0.1", oscPort);
    if (!mOscConnected)
        std::cout << "Error: could not connect to UDP port " << oscPort << "." << std::endl;
    else
        mLastConnectedOscPort = oscPort;

    return mOscConnected;
}

//==============================================================================
bool ControlGrisAudioProcessor::disconnectOsc()
{
    if (mOscConnected) {
        if (mOscSender.disconnect()) {
            mOscConnected = false;
            mLastConnectedOscPort = -1;
        }
    }
    return !mOscConnected;
}

//==============================================================================
void ControlGrisAudioProcessor::handleOscConnection(bool const state)
{
    if (state) {
        if (mLastConnectedOscPort != mCurrentOscPort) {
            createOscConnection(mCurrentOscPort);
        }
    } else {
        disconnectOsc();
    }
    mAudioProcessorValueTreeState.state.setProperty("oscConnected", isOscConnected(), nullptr);
}

//==============================================================================
void ControlGrisAudioProcessor::sendOscMessage()
{
    if (!mOscConnected)
        return;

    juce::OSCAddressPattern const oscPattern("/spat/serv");
    juce::OSCMessage message(oscPattern);

    for (auto const & source : mSources) {
        auto const azimuth{ source.getAzimuth().getAsRadians() };
        auto const elevation{ source.getElevation().getAsRadians() };
        auto const azimuthSpan{ source.getAzimuthSpan() * 2.0f };
        auto const elevationSpan{ source.getElevationSpan() * 0.5f };
        auto const distance{ mSpatMode == SpatMode::cube ? source.getDistance() / 0.6f : source.getDistance() };

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
bool ControlGrisAudioProcessor::createOscInputConnection(int const oscPort)
{
    disconnectOscInput(oscPort);

    mOscInputConnected = mOscInputReceiver.connect(oscPort);
    if (!mOscInputConnected) {
        std::cout << "Error: could not connect to UDP input port " << oscPort << "." << std::endl;
    } else {
        mOscInputReceiver.addListener(this);
        mCurrentOscInputPort = oscPort;
        mAudioProcessorValueTreeState.state.setProperty("oscInputPortNumber", oscPort, nullptr);
    }

    mAudioProcessorValueTreeState.state.setProperty("oscInputConnected", getOscInputConnected(), nullptr);

    return mOscInputConnected;
}

//==============================================================================
bool ControlGrisAudioProcessor::disconnectOscInput(int const oscPort)
{
    if (mOscInputConnected) {
        if (mOscInputReceiver.disconnect()) {
            mOscInputConnected = false;
        }
    }

    mAudioProcessorValueTreeState.state.setProperty("oscInputPortNumber", oscPort, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscInputConnected", getOscInputConnected(), nullptr);

    return !mOscInputConnected;
}

//==============================================================================
void ControlGrisAudioProcessor::oscBundleReceived(juce::OSCBundle const & bundle)
{
    for (auto const & element : bundle) {
        if (element.isMessage())
            oscMessageReceived(element.getMessage());
        else if (element.isBundle())
            oscBundleReceived(element.getBundle());
    }
}

//==============================================================================
void ControlGrisAudioProcessor::oscMessageReceived(juce::OSCMessage const & message)
{
    auto positionSourceLinkToProcess{ PositionSourceLink::undefined };
    auto elevationSourceLinkToProcess{ ElevationSourceLink::undefined };
    auto x{ -1.0f };
    auto y{ -1.0f };
    auto z{ -1.0f };
    auto const address{ message.getAddressPattern().toString() };
    auto const pluginInstance{ juce::String{ "/controlgris/" } + juce::String{ getOscOutputPluginId() } };
    if ((address == juce::String(pluginInstance + "/traj/1/x")
         || address == juce::String(pluginInstance + "/traj/1/xyz/1"))
        && mPositionTrajectoryManager.getTrajectoryType() == PositionTrajectoryType::realtime) {
        x = message[0].getFloat32();
    } else if ((address == juce::String(pluginInstance + "/traj/1/y")
                || address == juce::String(pluginInstance + "/traj/1/xyz/2"))
               && mPositionTrajectoryManager.getTrajectoryType() == PositionTrajectoryType::realtime) {
        y = message[0].getFloat32();
    } else if ((address == juce::String(pluginInstance + "/traj/1/z")
                || address == juce::String(pluginInstance + "/traj/1/xyz/3"))
               && static_cast<ElevationTrajectoryType>(mElevationTrajectoryManager.getTrajectoryType())
                      == ElevationTrajectoryType::realtime) {
        z = message[0].getFloat32();
    } else if (address == juce::String(pluginInstance + "/traj/1/xy")
               && mPositionTrajectoryManager.getTrajectoryType() == PositionTrajectoryType::realtime) {
        x = message[0].getFloat32();
        y = message[1].getFloat32();
    } else if (address == juce::String(pluginInstance + "/traj/1/xyz")) {
        if (mPositionTrajectoryManager.getTrajectoryType() == PositionTrajectoryType::realtime) {
            x = message[0].getFloat32();
            y = message[1].getFloat32();
        }
        if (static_cast<ElevationTrajectoryType>(mElevationTrajectoryManager.getTrajectoryType())
            == ElevationTrajectoryType::realtime) {
            z = message[2].getFloat32();
        }
    } else if (address == juce::String(pluginInstance + "/azispan")) {
        for (auto & source : mSources) {
            source.setAzimuthSpan(Normalized{ message[0].getFloat32() });
        }
        auto const gestureLock{ mChangeGesturesManager.getScopedLock(Automation::Ids::AZIMUTH_SPAN) };
        mAudioProcessorValueTreeState.getParameter(Automation::Ids::AZIMUTH_SPAN)
            ->setValueNotifyingHost(message[0].getFloat32());
    } else if (address == juce::String(pluginInstance + "/elespan")) {
        for (auto & source : mSources)
            source.setElevationSpan(Normalized{ message[0].getFloat32() });
        auto const gestureLock{ mChangeGesturesManager.getScopedLock(Automation::Ids::ELEVATION_SPAN) };
        mAudioProcessorValueTreeState.getParameter(Automation::Ids::ELEVATION_SPAN)
            ->setValueNotifyingHost(message[0].getFloat32());
    } else if (address == juce::String(pluginInstance + "/sourcelink/1/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(1);
    } else if (address == juce::String(pluginInstance + "/sourcelink/2/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(2);
    } else if (address == juce::String(pluginInstance + "/sourcelink/3/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(3);
    } else if (address == juce::String(pluginInstance + "/sourcelink/4/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(4);
    } else if (address == juce::String(pluginInstance + "/sourcelink/5/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(5);
    } else if (address == juce::String(pluginInstance + "/sourcelink/6/1")) {
        if (message[0].getFloat32() == 1)
            positionSourceLinkToProcess = static_cast<PositionSourceLink>(6);
    } else if (address == juce::String(pluginInstance + "/sourcelink")) {
        positionSourceLinkToProcess = static_cast<PositionSourceLink>(message[0].getFloat32()); // 1 -> 6
    } else if (address == juce::String(pluginInstance + "/sourcelinkalt/1/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(1);
    } else if (address == juce::String(pluginInstance + "/sourcelinkalt/2/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(2);
    } else if (address == juce::String(pluginInstance + "/sourcelinkalt/3/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(3);
    } else if (address == juce::String(pluginInstance + "/sourcelinkalt/4/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(4);
    } else if (address == juce::String(pluginInstance + "/sourcelinkalt/5/1")) {
        if (message[0].getFloat32() == 1)
            elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(5);
    } else if (address == juce::String(pluginInstance + "/sourcelinkalt")) {
        elevationSourceLinkToProcess = static_cast<ElevationSourceLink>(message[0].getFloat32()); // 1 -> 5
    } else if (address == juce::String(pluginInstance + "/presets")) {
        int newPreset = (int)message[0].getFloat32(); // 1 -> 50
        auto const loaded{ mPresetManager.loadIfPresetChanged(newPreset) };
        if (loaded) {
            mPositionTrajectoryManager.recomputeTrajectory();
            mElevationTrajectoryManager.recomputeTrajectory();
        }
        auto * ed{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
        if (ed != nullptr) {
            ed->updatePositionPreset(newPreset);
        }
    }

    if (x != -1.0f && y != -1.0f) {
        mSources.getPrimarySource().setPosition(juce::Point<float>{ x, y }, Source::OriginOfChange::osc);

    } else if (y != -1.0f) {
        mSources.getPrimarySource().setY(y, Source::OriginOfChange::osc);
    } else if (x != -1.0f) {
        mSources.getPrimarySource().setX(x, Source::OriginOfChange::osc);
    }
    sourcePositionChanged(SourceIndex{ 0 }, 0);

    if (z != -1.0f) {
        mSources.getPrimarySource().setY(z, Source::OriginOfChange::osc);
        mElevationTrajectoryManager.sendTrajectoryPositionChangedEvent();
    }

    mPresetManager.loadIfPresetChanged(0);

    if (static_cast<bool>(positionSourceLinkToProcess)) {
        setPositionSourceLink(positionSourceLinkToProcess, SourceLinkEnforcer::OriginOfChange::user);
    }

    if (static_cast<bool>(elevationSourceLinkToProcess)) {
        setElevationSourceLink(elevationSourceLinkToProcess, SourceLinkEnforcer::OriginOfChange::user);
    }
}

//==============================================================================
bool ControlGrisAudioProcessor::createOscOutputConnection(juce::String const & oscAddress, int const oscPort)
{
    disconnectOscOutput(oscAddress, oscPort);

    mOscOutputConnected = mOscOutputSender.connect(oscAddress, oscPort);
    if (!mOscOutputConnected)
        std::cout << "Error: could not connect to UDP output port " << oscPort << " on address " << oscAddress << "."
                  << std::endl;
    else {
        mCurrentOscOutputPort = oscPort;
        mCurrentOscOutputAddress = oscAddress;
        mAudioProcessorValueTreeState.state.setProperty("oscOutputPortNumber", oscPort, nullptr);
        mAudioProcessorValueTreeState.state.setProperty("oscOutputAddress", oscAddress, nullptr);
    }

    mAudioProcessorValueTreeState.state.setProperty("oscOutputConnected", getOscOutputConnected(), nullptr);

    return mOscOutputConnected;
}

//==============================================================================
bool ControlGrisAudioProcessor::disconnectOscOutput(juce::String const & oscAddress, int const oscPort)
{
    if (mOscOutputConnected) {
        if (mOscOutputSender.disconnect()) {
            mOscOutputConnected = false;
        }
    }

    mAudioProcessorValueTreeState.state.setProperty("oscOutputPortNumber", oscPort, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscOutputAddress", oscAddress, nullptr);
    mAudioProcessorValueTreeState.state.setProperty("oscOutputConnected", getOscOutputConnected(), nullptr);

    return !mOscOutputConnected;
}

//==============================================================================
void ControlGrisAudioProcessor::setOscOutputPluginId(int const pluginId)
{
    mAudioProcessorValueTreeState.state.setProperty("oscOutputPluginId", pluginId, nullptr);
}

//==============================================================================
int ControlGrisAudioProcessor::getOscOutputPluginId() const
{
    return mAudioProcessorValueTreeState.state.getProperty("oscOutputPluginId", 1);
}

//==============================================================================
void ControlGrisAudioProcessor::sendOscOutputMessage()
{
    static constexpr auto IMPOSSIBLE_NUMBER{ std::numeric_limits<float>::min() };

    static auto lastTrajectoryX{ IMPOSSIBLE_NUMBER };
    static auto lastTrajectoryY{ IMPOSSIBLE_NUMBER };
    static auto lastTrajectoryZ{ IMPOSSIBLE_NUMBER };
    static Normalized lastAzimuthSpan{ IMPOSSIBLE_NUMBER };
    static Normalized lastElevationSpan{ IMPOSSIBLE_NUMBER };
    static auto lastPositionLink{ PositionSourceLink::undefined };
    static auto lastElevationLink{ ElevationSourceLink::undefined };
    static auto lastPresetNumber{ std::numeric_limits<int>::min() };

    if (!mOscOutputConnected) {
        return;
    }

    juce::OSCMessage message(juce::OSCAddressPattern("/tmp"));

    auto const pluginInstance = juce::String{ "/controlgris/" } + juce::String{ getOscOutputPluginId() };

    auto const trajectoryHandlePosition{ mSources.getPrimarySource().getPos() };
    auto const trajectory1x = trajectoryHandlePosition.getX();
    auto const trajectory1y = trajectoryHandlePosition.getY();
    auto const trajectory1z = trajectoryHandlePosition.getY();

    if (lastTrajectoryX != trajectory1x) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/traj/1/x"));
        message.addFloat32(trajectory1x);
        mOscOutputSender.send(message);
        message.clear();

        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/traj/1/xyz/1"));
        message.addFloat32(trajectory1x);
        mOscOutputSender.send(message);
        message.clear();
    }

    if (lastTrajectoryY != trajectory1y) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/traj/1/y"));
        message.addFloat32(trajectory1y);
        mOscOutputSender.send(message);
        message.clear();

        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/traj/1/xyz/2"));
        message.addFloat32(trajectory1y);
        mOscOutputSender.send(message);
        message.clear();
    }

    if (lastTrajectoryZ != trajectory1z) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/traj/1/z"));
        message.addFloat32(trajectory1z);
        mOscOutputSender.send(message);
        message.clear();

        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/traj/1/xyz/3"));
        message.addFloat32(trajectory1z);
        mOscOutputSender.send(message);
        message.clear();
    }

    if (lastTrajectoryX != trajectory1x || lastTrajectoryY != trajectory1y || lastTrajectoryZ != trajectory1z) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/traj/1/xy"));
        message.addFloat32(trajectory1x);
        message.addFloat32(trajectory1y);
        mOscOutputSender.send(message);
        message.clear();

        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/traj/1/xyz"));
        message.addFloat32(trajectory1x);
        message.addFloat32(trajectory1y);
        message.addFloat32(trajectory1z);
        mOscOutputSender.send(message);
        message.clear();
    }

    lastTrajectoryX = trajectory1x;
    lastTrajectoryY = trajectory1y;
    lastTrajectoryZ = trajectory1z;

    if (lastAzimuthSpan != mSources.getPrimarySource().getAzimuthSpan()) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/azispan"));
        message.addFloat32(mSources.getPrimarySource().getAzimuthSpan().toFloat());
        mOscOutputSender.send(message);
        message.clear();
        lastAzimuthSpan = mSources.getPrimarySource().getAzimuthSpan();
    }

    if (lastElevationSpan != mSources.getPrimarySource().getElevationSpan()) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/elespan"));
        message.addFloat32(mSources.getPrimarySource().getElevationSpan().toFloat());
        mOscOutputSender.send(message);
        message.clear();
        lastElevationSpan = mSources.getPrimarySource().getElevationSpan();
    }

    if (mPositionTrajectoryManager.getSourceLink() != lastPositionLink) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/sourcelink"));
        message.addInt32(static_cast<juce::int32>(mPositionTrajectoryManager.getSourceLink()));
        mOscOutputSender.send(message);
        message.clear();

        juce::String pattern = pluginInstance + juce::String("/sourcelink/")
                               + juce::String(static_cast<int>(mPositionTrajectoryManager.getSourceLink()))
                               + juce::String("/1");
        message.setAddressPattern(juce::OSCAddressPattern(pattern));
        message.addInt32(1);
        mOscOutputSender.send(message);
        message.clear();

        lastPositionLink = mPositionTrajectoryManager.getSourceLink();
    }

    if (static_cast<ElevationSourceLink>(mElevationTrajectoryManager.getSourceLink()) != lastElevationLink) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/sourcelinkalt"));
        message.addInt32(static_cast<juce::int32>(mElevationTrajectoryManager.getSourceLink()));
        mOscOutputSender.send(message);
        message.clear();

        juce::String patternAlt = pluginInstance + juce::String("/sourcelinkalt/")
                                  + juce::String(static_cast<int>(mElevationTrajectoryManager.getSourceLink()))
                                  + juce::String("/1");
        message.setAddressPattern(juce::OSCAddressPattern(patternAlt));
        message.addInt32(1);
        mOscOutputSender.send(message);
        message.clear();

        lastElevationLink = static_cast<ElevationSourceLink>(mElevationTrajectoryManager.getSourceLink());
    }

    auto const currentPreset{ mPresetManager.getCurrentPreset() };
    if (currentPreset != lastPresetNumber) {
        message.setAddressPattern(juce::OSCAddressPattern(pluginInstance + "/presets"));
        message.addInt32(currentPreset);
        mOscOutputSender.send(message);
        message.clear();

        lastPresetNumber = currentPreset;
    }
}

//==============================================================================
void ControlGrisAudioProcessor::timerCallback()
{
    auto * editor{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };

    // automation
    if (mLastTimerTime != getCurrentTime()) {
        auto const deltaTime{ getCurrentTime() - getInitTimeOnPlay() };
        if (mPositionTrajectoryManager.getPositionActivateState()) {
            mPositionTrajectoryManager.setTrajectoryDeltaTime(deltaTime);
        }
        if (mSpatMode == SpatMode::cube && mElevationTrajectoryManager.getPositionActivateState()) {
            mElevationTrajectoryManager.setTrajectoryDeltaTime(deltaTime);
        }
    }

    mLastTimerTime = getCurrentTime();

    if (mCanStopActivate && !mIsPlaying) {
        if (mPositionTrajectoryManager.getPositionActivateState())
            mPositionTrajectoryManager.setPositionActivateState(false);
        if (mElevationTrajectoryManager.getPositionActivateState())
            mElevationTrajectoryManager.setPositionActivateState(false);
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
    //    if (mPresetManager.getCurrentPreset() == 0) {
    auto & state{ mAudioProcessorValueTreeState.state };
    for (auto & source : mSources) {
        auto const index{ source.getIndex().toString() };

        juce::Identifier const azimuthPropertyString{ juce::String{ "p_azimuth_" } + index };
        juce::Identifier const elevationPropertyString{ juce::String{ "p_elevation_" } + index };
        juce::Identifier const distancePropertyString{ juce::String{ "p_distance_" } + index };

        jassert(state.hasProperty(azimuthPropertyString));
        jassert(state.hasProperty(elevationPropertyString));
        jassert(state.hasProperty(distancePropertyString));

        auto const & rawAzimuth{ state.getProperty(azimuthPropertyString) };
        auto const & rawElevation{ state.getProperty(elevationPropertyString) };
        auto const & rawDistance{ state.getProperty(distancePropertyString) };

        Normalized const azimuth{ rawAzimuth };
        Normalized const elevation{ rawElevation };
        float const distance{ rawDistance };

        source.setAzimuth(azimuth, Source::OriginOfChange::userAnchorMove);
        source.setElevation(elevation, Source::OriginOfChange::userAnchorMove);
        source.setDistance(distance, Source::OriginOfChange::userAnchorMove);
    }
    //    }

    auto * editor{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
    if (editor != nullptr) {
        editor->reloadUiState();
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
            mPositionTrajectoryManager.setTrajectoryType(mPositionTrajectoryManager.getTrajectoryType(),
                                                         mSources.getPrimarySource().getPos());
        }
    } else {
        setSourceParameterValue(sourceIndex, SourceParameter::elevation, source.getNormalizedElevation().toFloat());
        mElevationTrajectoryManager.setTrajectoryType(mElevationTrajectoryManager.getTrajectoryType());
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setSourceParameterValue(SourceIndex const sourceIndex,
                                                        SourceParameter const parameterId,
                                                        float const value)
{
    Normalized const normalized{ static_cast<float>(value) };
    auto const param_id{ sourceIndex.toString() };
    auto & source{ mSources[sourceIndex] };
    switch (parameterId) {
    case SourceParameter::azimuth:
        mAudioProcessorValueTreeState.state.setProperty("p_azimuth_" + param_id, value, nullptr);
        break;
    case SourceParameter::elevation:
        mAudioProcessorValueTreeState.state.setProperty(juce::String("p_elevation_") + param_id, value, nullptr);
        break;
    case SourceParameter::distance:
        mAudioProcessorValueTreeState.state.setProperty(juce::String("p_distance_") + param_id, value, nullptr);
        break;
    case SourceParameter::x:
        jassertfalse;
        break;
    case SourceParameter::y:
        jassertfalse;
        break;
    case SourceParameter::azimuthSpan:
        for (auto & source : mSources) {
            source.setAzimuthSpan(normalized);
        }
        mAudioProcessorValueTreeState.getParameter(Automation::Ids::AZIMUTH_SPAN)->setValueNotifyingHost(value);
        break;
    case SourceParameter::elevationSpan:
        for (auto & source : mSources) {
            source.setElevationSpan(normalized);
        }
        mAudioProcessorValueTreeState.getParameter(Automation::Ids::ELEVATION_SPAN)->setValueNotifyingHost(value);
        break;
    }
}

//==============================================================================
juce::String const ControlGrisAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

//==============================================================================
bool ControlGrisAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

//==============================================================================
bool ControlGrisAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

//==============================================================================
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
}

//==============================================================================
void ControlGrisAudioProcessor::prepareToPlay([[maybe_unused]] double const sampleRate,
                                              [[maybe_unused]] int const samplesPerBlock)
{
    if (!mIsPlaying) {
        initialize();
    }
}

//==============================================================================
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

//==============================================================================
void ControlGrisAudioProcessor::processBlock([[maybe_unused]] juce::AudioBuffer<float> & buffer,
                                             [[maybe_unused]] juce::MidiBuffer & midiMessages)
{
    auto const wasPlaying{ mIsPlaying };
    juce::AudioPlayHead * audioPlayHead = getPlayHead();
    if (audioPlayHead != nullptr) {
        juce::AudioPlayHead::CurrentPositionInfo currentPositionInfo{};
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

    if (!wasPlaying && mIsPlaying) {         // Initialization here only for Logic (also Reaper and Live), which are not
        juce::PluginHostType const hostType; // calling prepareToPlay every time the sequence starts.
        if (hostType.isLogic() || hostType.isReaper() || hostType.isAbletonLive()) {
            initialize();
        }
    }

    // deal with trajectory recording gestures
    bool const isPositionTrajectoryActive{ mPositionTrajectoryManager.getPositionActivateState() };
    bool const isElevationTrajectoryActive{ mElevationTrajectoryManager.getPositionActivateState() };

    static bool positionGestureStarted{ false };
    static bool elevationGestureStarted{ false };

    if (isPositionTrajectoryActive && mIsPlaying && !positionGestureStarted) {
        positionGestureStarted = true;
        mChangeGesturesManager.beginGesture(Automation::Ids::X);
        mChangeGesturesManager.beginGesture(Automation::Ids::Y);
    } else if ((!isPositionTrajectoryActive || !mIsPlaying) && positionGestureStarted) {
        positionGestureStarted = false;
        mChangeGesturesManager.endGesture(Automation::Ids::X);
        mChangeGesturesManager.endGesture(Automation::Ids::Y);
    }
    if (mSpatMode == SpatMode::cube) {
        if (isElevationTrajectoryActive && mIsPlaying && !elevationGestureStarted) {
            elevationGestureStarted = true;
            mChangeGesturesManager.beginGesture(Automation::Ids::Z);
        } else if ((!isElevationTrajectoryActive || !mIsPlaying) && elevationGestureStarted) {
            elevationGestureStarted = false;
            mChangeGesturesManager.endGesture(Automation::Ids::Z);
        }
    }

    mLastTime = mCurrentTime;
}

//==============================================================================
juce::AudioProcessorEditor * ControlGrisAudioProcessor::createEditor()
{
    return new ControlGrisAudioProcessorEditor(*this,
                                               mAudioProcessorValueTreeState,
                                               mPositionTrajectoryManager,
                                               mElevationTrajectoryManager);
}

//==============================================================================
void ControlGrisAudioProcessor::getStateInformation(juce::MemoryBlock & destData)
{
    for (int sourceIndex{}; sourceIndex < MAX_NUMBER_OF_SOURCES; ++sourceIndex) {
        juce::String const id{ sourceIndex };
        juce::Identifier const azimuthId{ juce::String{ "p_azimuth_" } + id };
        juce::Identifier const elevationId{ juce::String{ "p_elevation_" } + id };
        juce::Identifier const distanceId{ juce::String{ "p_distance_" } + id };
        auto const & source{ mSources[sourceIndex] };
        auto const normalizedAzimuth{ source.getNormalizedAzimuth().toFloat() };
        auto const normalizedElevation{ source.getNormalizedElevation().toFloat() };
        auto const distance{ source.getDistance() };

        mAudioProcessorValueTreeState.state.setProperty(azimuthId, normalizedAzimuth, nullptr);
        mAudioProcessorValueTreeState.state.setProperty(elevationId, normalizedElevation, nullptr);
        mAudioProcessorValueTreeState.state.setProperty(distanceId, distance, nullptr);
    }
    auto const state{ mAudioProcessorValueTreeState.copyState() };

    auto xmlState{ state.createXml() };

    if (xmlState != nullptr) {
        auto * childExist = xmlState->getChildByName(FIXED_POSITION_DATA_TAG);
        if (childExist) {
            xmlState->removeChildElement(childExist, true);
        }
        if (mFixPositionData.getNumChildElements() > 0) {
            auto * positionData{ new juce::XmlElement{ mFixPositionData } };
            xmlState->addChildElement(positionData);
        }
        copyXmlToBinary(*xmlState, destData);
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setStateInformation(void const * data, int const sizeInBytes)
{
    juce::MessageManagerLock mmLock{};

    auto const xmlState{ getXmlFromBinary(data, sizeInBytes) };

    if (xmlState != nullptr) {
        // Set global settings values.
        //----------------------------
        auto const valueTree{ juce::ValueTree::fromXml(*xmlState) };
        auto const spatMode{ static_cast<SpatMode>(static_cast<int>(valueTree.getProperty("oscFormat", 0))) };
        setSpatMode(spatMode);
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
        auto * positionData{ xmlState->getChildByName(FIXED_POSITION_DATA_TAG) };
        if (positionData) {
            mFixPositionData.deleteAllChildElements();
            mFixPositionData = *positionData;
            mPositionSourceLinkEnforcer.enforceSourceLink();
            if (mSpatMode == SpatMode::cube) {
                mElevationSourceLinkEnforcer.enforceSourceLink();
            }
        }
        // Replace the state and call automated parameter current values.
        //---------------------------------------------------------------
        mAudioProcessorValueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
    }

    setPluginState();
}

//==============================================================================
void ControlGrisAudioProcessor::sourceChanged(Source & source,
                                              Source::ChangeType changeType,
                                              Source::OriginOfChange origin)
{
    jassert(changeType == Source::ChangeType::position || changeType == Source::ChangeType::elevation);

    auto & trajectoryManager{ changeType == Source::ChangeType::position
                                  ? static_cast<TrajectoryManager &>(mPositionTrajectoryManager)
                                  : static_cast<TrajectoryManager &>(mElevationTrajectoryManager) };
    auto & sourceLinkEnforcer{ changeType == Source::ChangeType::position ? mPositionSourceLinkEnforcer
                                                                          : mElevationSourceLinkEnforcer };
    // auto const isTrajectoryActive{ mPositionTrajectoryManager.getPositionActivateState()
    //                               || mElevationTrajectoryManager.getPositionActivateState() };
    auto const isTrajectoryActive{ trajectoryManager.getPositionActivateState() };
    auto const isPrimarySource{ source.isPrimarySource() };

    switch (origin) {
    case Source::OriginOfChange::none:
        return;
    case Source::OriginOfChange::userMove:
        sourceLinkEnforcer.sourceMoved(source);
        setSelectedSource(source);
        if (isPrimarySource) {
            trajectoryManager.sourceMoved(source);
            updatePrimarySourceParameters(changeType);
        } else {
            getPresetsManager().loadIfPresetChanged(0);
        }
        return;
    case Source::OriginOfChange::userAnchorMove:
        sourceLinkEnforcer.anchorMoved(source);
        setSelectedSource(source);
        if (isPrimarySource) {
            trajectoryManager.sourceMoved(source);
            updatePrimarySourceParameters(changeType);
        }
        mPresetManager.loadIfPresetChanged(0);
        return;
    case Source::OriginOfChange::presetRecall:
        jassert(isPrimarySource);
        sourceLinkEnforcer.sourceMoved(source);
        trajectoryManager.sourceMoved(source);
        return;
    case Source::OriginOfChange::link:
        if (isPrimarySource) {
            sourceLinkEnforcer.sourceMoved(source);
            trajectoryManager.sourceMoved(source);
            updatePrimarySourceParameters(changeType);
        }
        return;
    case Source::OriginOfChange::trajectory:
        jassert(isPrimarySource);
        sourceLinkEnforcer.sourceMoved(source);
        updatePrimarySourceParameters(changeType);
        return;
    case Source::OriginOfChange::osc:
        jassert(isPrimarySource);
        sourceLinkEnforcer.sourceMoved(source);
        trajectoryManager.sourceMoved(source);
        updatePrimarySourceParameters(changeType);
        return;
    case Source::OriginOfChange::automation:
        sourceLinkEnforcer.sourceMoved(source);
        if (!isTrajectoryActive) {
            trajectoryManager.sourceMoved(source);
        }
        return;
    }
    jassertfalse;
}

//==============================================================================
void ControlGrisAudioProcessor::setSelectedSource(const Source & source)
{
    auto * editor{ dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor()) };
    if (editor != nullptr) {
        auto const index{ source.getIndex() };
        juce::MessageManager::callAsync([=] { editor->sourceBoxSelectionChanged(index); });
    }
}

//==============================================================================
void ControlGrisAudioProcessor::updatePrimarySourceParameters(Source::ChangeType const changeType)
{
    auto const & source{ mSources.getPrimarySource() };
    switch (changeType) {
    case Source::ChangeType::position: {
        auto const x_sl{ mChangeGesturesManager.getScopedLock(Automation::Ids::X) };
        auto const y_sl{ mChangeGesturesManager.getScopedLock(Automation::Ids::Y) };
        auto const normalized_x{ (source.getX() + 1.0f) / 2.0f };
        auto const normalized_y{ 1.0f - (source.getY() + 1.0f) / 2.0f };
        auto * x_param{ mAudioProcessorValueTreeState.getParameter(Automation::Ids::X) };
        auto * y_param{ mAudioProcessorValueTreeState.getParameter(Automation::Ids::Y) };
        x_param->setValueNotifyingHost(normalized_x);
        y_param->setValueNotifyingHost(normalized_y);
        break;
    }
    case Source::ChangeType::elevation: {
        jassert(mSpatMode == SpatMode::cube);
        auto const sl{ mChangeGesturesManager.getScopedLock(Automation::Ids::Z) };
        auto const normalized_z{ 1.0f - source.getElevation() / MAX_ELEVATION };
        mAudioProcessorValueTreeState.getParameter(Automation::Ids::Z)->setValueNotifyingHost(normalized_z);
        break;
    }
    default:
        jassertfalse;
    }
}

} // namespace gris

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor * JUCE_CALLTYPE createPluginFilter()
{
    return new gris::ControlGrisAudioProcessor{};
}
