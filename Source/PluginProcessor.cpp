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

String getFixedPosSourceName(int index, int dimension) {
    if (dimension == 0)
        return String("S") + String(index + 1) + String("_X");
    else if (dimension == 1)
        return String("S") + String(index + 1) + String("_Y");
    else if (dimension == 2)
        return String("S") + String(index + 1) + String("_Z");
    else
        return String();
}

// The parameter Layout creates the automatable parameters.
AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    using Parameter = AudioProcessorValueTreeState::Parameter;

    std::vector<std::unique_ptr<Parameter>> parameters;

    parameters.push_back(std::make_unique<Parameter>(String("recordingTrajectory_x"), String("Recording Trajectory X"),
                                                     String(), NormalisableRange<float>(0.f, 1.f), 0.5f, nullptr, nullptr));
    parameters.push_back(std::make_unique<Parameter>(String("recordingTrajectory_y"), String("Recording Trajectory Y"),
                                                     String(), NormalisableRange<float>(0.f, 1.f), 0.5f, nullptr, nullptr));
    parameters.push_back(std::make_unique<Parameter>(String("recordingTrajectory_z"), String("Recording Trajectory Z"),
                                                     String(), NormalisableRange<float>(0.f, 1.f), 0.5f, nullptr, nullptr));

    parameters.push_back(std::make_unique<Parameter>(String("sourceLink"), String("Source Link"), String(),
                                                     NormalisableRange<float>(0.f, 1.f), 0.f, nullptr, nullptr));
    parameters.push_back(std::make_unique<Parameter>(String("sourceLinkAlt"), String("Source Link Alt"), String(),
                                                     NormalisableRange<float>(0.f, 1.f), 0.f, nullptr, nullptr));

    parameters.push_back(std::make_unique<Parameter>(String("positionPreset"), String("Position Preset"), String(),
                                                     NormalisableRange<float>(0.f, 1.f, 1.f/50.f), 1.f, nullptr, nullptr,
                                                     false, true, true));

    for (int i = 0; i < MAX_NUMBER_OF_SOURCES; i++) {
        String id(i);
        String id1(i + 1);
        parameters.push_back(std::make_unique<Parameter>(
                                 String("azimuthSpan_") + id, String("Source ") + id1 + String(" Azimuth Span"),
                                 String(), NormalisableRange<float>(0.f, 1.f), 0.f, nullptr, nullptr));
        parameters.push_back(std::make_unique<Parameter>(
                                 String("elevationSpan_") + id, String("Source ") + id1 + String(" Elevation Span"),
                                 String(), NormalisableRange<float>(0.f, 1.f), 0.f, nullptr, nullptr));
    }

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
ControlGrisAudioProcessor::ControlGrisAudioProcessor()
     :
#ifndef JucePlugin_PreferredChannelConfigurations
     AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters (*this, nullptr, Identifier(JucePlugin_Name), createParameterLayout()),
    fixPositionData (FIXED_POSITION_DATA_TAG)
{
    m_lock = false;
    m_somethingChanged = false;
    m_numOfSources = 1;
    m_firstSourceId = 1;
    m_selectedSourceId = 1;
    m_selectedOscFormat = (SPAT_MODE_ENUM)0;
    m_currentOSCPort = 18032;
    m_lastConnectedOSCPort = -1;
    m_oscConnected = true;
    m_oscInputConnected = false;
    m_currentOSCInputPort = 8000;

    m_initTimeOnPlay = m_currentTime = 0.0;
    m_lastTime = m_lastTimerTime = 10000000.0;

    m_bpm = 120;

    m_canStopActivate = false;

    // Size of the plugin window.
    parameters.state.addChild ({ "uiState", { { "width",  650 }, { "height", 680 } }, {} }, -1, nullptr);

    // Global setting parameters.
    parameters.state.setProperty("oscFormat", 0, nullptr);
    parameters.state.setProperty("oscPortNumber", 18032, nullptr);
    parameters.state.setProperty("oscInputPortNumber", 8000, nullptr);
    parameters.state.setProperty("oscConnected", true, nullptr);
    parameters.state.setProperty("oscInputConnected", false, nullptr);
    parameters.state.setProperty("numberOfSources", 2, nullptr);
    parameters.state.setProperty("firstSourceId", 1, nullptr);
    parameters.state.setProperty("azimuthSpanLink", false, nullptr);
    parameters.state.setProperty("elevationSpanLink", false, nullptr);
    parameters.state.setProperty("spanLinkState", false, nullptr);

    // Trajectory box persitent settings.
    parameters.state.setProperty("trajectoryType", 1, nullptr);
    parameters.state.setProperty("trajectoryTypeAlt", 1, nullptr);
    parameters.state.setProperty("cycleDuration", 5, nullptr);
    parameters.state.setProperty("durationUnit", 1, nullptr);

    // Per source parameters. Because there is no attachment to the automatable
    // parameters, we need to keep track of the current parameter values to be
    // able to reload the last state of the plugin when we close/open the UI.
    for (int i = 0; i < MAX_NUMBER_OF_SOURCES; i++) {
        String id(i);
        // Non-automatable, per source, parameters.
        parameters.state.setProperty(String("p_azimuth_") + id, 0.0, nullptr);
        parameters.state.setProperty(String("p_elevation_") + id, 0.0, nullptr);
        parameters.state.setProperty(String("p_distance_") + id, 0.0, nullptr);
        parameters.state.setProperty(String("p_x_") + id, 0.0, nullptr);
        parameters.state.setProperty(String("p_y_") + id, 0.0, nullptr);

        // Automatable, per source, parameters.
        parameters.addParameterListener(String("azimuthSpan_") + id, this);
        parameters.addParameterListener(String("elevationSpan_") + id, this);

        // Gives the source an initial id.
        sources[i].setId(i + m_firstSourceId - 1);
    }

    // Automation values for the recording trajectory.
    parameters.addParameterListener(String("recordingTrajectory_x"), this);
    parameters.addParameterListener(String("recordingTrajectory_y"), this);
    parameters.addParameterListener(String("recordingTrajectory_z"), this);
    parameters.addParameterListener(String("sourceLink"), this);
    parameters.addParameterListener(String("sourceLinkAlt"), this);
    parameters.addParameterListener(String("positionPreset"), this);

    automationManager.addListener(this);
    automationManagerAlt.addListener(this);

    // The timer's callback send OSC messages periodically.
    //-----------------------------------------------------
    startTimerHz(50);
}

ControlGrisAudioProcessor::~ControlGrisAudioProcessor() {
    disconnectOSC();
}

//==============================================================================
void ControlGrisAudioProcessor::parameterChanged(const String &parameterID, float newValue) {
    if (std::isnan(newValue) || std::isinf(newValue)) {
        return;
    }

    bool needToLinkSourcePositions = false;
    if (parameterID.compare("recordingTrajectory_x") == 0) {
        automationManager.setPlaybackPositionX(newValue);
        needToLinkSourcePositions = true;
    } else if (parameterID.compare("recordingTrajectory_y") == 0) {
        automationManager.setPlaybackPositionY(newValue);
        needToLinkSourcePositions = true;
    } else if (parameterID.compare("recordingTrajectory_z") == 0 && m_selectedOscFormat == SPAT_MODE_LBAP) {
        automationManagerAlt.setPlaybackPositionY(newValue);
        linkSourcePositionsAlt();
    }

    if (needToLinkSourcePositions) {
        linkSourcePositions();
    }

    if (parameterID.compare("sourceLink") == 0) {
        int val = (int)(newValue * 5) + 1;
        if (val != automationManager.getSourceLink()) {
            automationManager.setSourceLink(val);
            automationManager.fixSourcePosition();
            onSourceLinkChanged(val);
            ControlGrisAudioProcessorEditor *ed = dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor());
            if (ed != nullptr) {
                ed->updateSourceLinkCombo(val);
            }
        }
    }

    if (parameterID.compare("sourceLinkAlt") == 0) {
        int val = (int)(newValue * 4) + 1;
        if (val != automationManagerAlt.getSourceLink()) {
            automationManagerAlt.setSourceLink(val);
            automationManagerAlt.fixSourcePosition();
            onSourceLinkAltChanged(val);
            ControlGrisAudioProcessorEditor *ed = dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor());
            if (ed != nullptr) {
                ed->updateSourceLinkAltCombo(val);
            }
        }
    }

    if (parameterID.compare("positionPreset") == 0) {
        int newPreset = (int)(newValue * NUMBER_OF_POSITION_PRESETS + 1);
        recallFixedPosition(newPreset);
        ControlGrisAudioProcessorEditor *ed = dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor());
        if (ed != nullptr) {
            ed->updatePositionPreset(newPreset);
        }
    }

    int sourceId = parameterID.getTrailingIntValue();
    if (parameterID.startsWith("azimuthSpan_")) {
        sources[sourceId].setAzimuthSpan(newValue);
    } else if (parameterID.startsWith("elevationSpan_")) {
        sources[sourceId].setElevationSpan(newValue);
    }

    m_somethingChanged = true;
}

//== Tools for sorting sources based on azimuth values. ==
struct Sorter {
    int index;
    float value;
};

bool compareLessThan(const Sorter &a, const Sorter &b) {
    return a.value <= b.value;
}
//========================================================

void ControlGrisAudioProcessor::onSourceLinkChanged(int value) {
    // Fixed radius.
    if (value == SOURCE_LINK_CIRCULAR_FIXED_RADIUS || value == SOURCE_LINK_CIRCULAR_FULLY_FIXED) {
        if (m_selectedOscFormat == SPAT_MODE_LBAP) {
            for (int i = 1; i < m_numOfSources; i++) {
                sources[i].setDistance(sources[0].getDistance());
            }
        } else {
            for (int i = 1; i < m_numOfSources; i++) {
                sources[i].setElevation(sources[0].getElevation());
            }
        }
    }

    // Fixed angle.
    if (value == SOURCE_LINK_CIRCULAR_FIXED_ANGLE || value == SOURCE_LINK_CIRCULAR_FULLY_FIXED) {
        Sorter tosort[m_numOfSources];
        for (int i = 0; i < m_numOfSources; i++) {
            tosort[i].index = i;
            tosort[i].value = sources[i].getAzimuth();
        }
        std::sort(tosort, tosort + m_numOfSources, compareLessThan);

        int posOfFirstSource;
        for (int i = 0; i < m_numOfSources; i++) {
            if (tosort[i].index == 0) {
                posOfFirstSource = i;
            }
        }

        float currentPos = sources[0].getAzimuth();
        for (int i = 0; i < m_numOfSources; i++) {
            float newPos = 360.0 / m_numOfSources * i + currentPos;
            int ioff = (i + posOfFirstSource) % m_numOfSources;
            sources[tosort[ioff].index].setAzimuth(newPos);
        }
    }

    bool shouldBeFixed = value != SOURCE_LINK_INDEPENDENT;
    for (int i = 0; i < m_numOfSources; i++) {
        sources[i].fixSourcePosition(shouldBeFixed);
    }
}

void ControlGrisAudioProcessor::onSourceLinkAltChanged(int value) {
    if (getOscFormat() == SPAT_MODE_LBAP) {
        // Fixed elevation.
        if (value == SOURCE_LINK_ALT_FIXED_ELEVATION) {
            for (int i = 1; i < m_numOfSources; i++) {
                sources[i].setElevation(sources[0].getElevation());
            }
        }

        // Linear min.
        if (value == SOURCE_LINK_ALT_LINEAR_MIN) {
            for (int i = 0; i < m_numOfSources; i++) {
                sources[i].setElevation(60.0 / m_numOfSources * i);
            }
        }

        // Linear max.
        if (value == SOURCE_LINK_ALT_LINEAR_MAX) {
            for (int i = 0; i < m_numOfSources; i++) {
                sources[i].setElevation(90.0 - (60.0 / m_numOfSources * i));
            }
        }

        bool shouldBeFixed = value != SOURCE_LINK_ALT_INDEPENDENT;
        for (int i = 0; i < m_numOfSources; i++) {
            sources[i].fixSourcePositionElevation(shouldBeFixed);
        }
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setOscFormat(SPAT_MODE_ENUM oscFormat) {
    m_selectedOscFormat = oscFormat;
    parameters.state.setProperty("oscFormat", m_selectedOscFormat, nullptr);
    for (int i = 0; i < MAX_NUMBER_OF_SOURCES; i++) {
        sources[i].setRadiusIsElevation(m_selectedOscFormat != SPAT_MODE_LBAP);
    }
}

SPAT_MODE_ENUM ControlGrisAudioProcessor::getOscFormat() {
    return m_selectedOscFormat;
}

void ControlGrisAudioProcessor::setOscPortNumber(int oscPortNumber) {
    m_currentOSCPort = oscPortNumber;
    parameters.state.setProperty("oscPortNumber", m_currentOSCPort, nullptr);
}

int ControlGrisAudioProcessor::getOscPortNumber() {
    return m_currentOSCPort;
}

void ControlGrisAudioProcessor::setFirstSourceId(int firstSourceId, bool propagate) {
    m_firstSourceId = firstSourceId;
    parameters.state.setProperty("firstSourceId", m_firstSourceId, nullptr);
    for (int i = 0; i < MAX_NUMBER_OF_SOURCES; i++) {
        sources[i].setId(i + m_firstSourceId - 1);
    }

    if (propagate)
        sendOscMessage();
}

int ControlGrisAudioProcessor::getFirstSourceId() {
    return m_firstSourceId;
}

void ControlGrisAudioProcessor::setNumberOfSources(int numOfSources, bool propagate) {
    m_numOfSources = numOfSources;
    parameters.state.setProperty("numberOfSources", m_numOfSources, nullptr);

    if (propagate)
        sendOscMessage();
}

void ControlGrisAudioProcessor::setSelectedSourceId(int id) {
    m_selectedSourceId = id;
}

int ControlGrisAudioProcessor::getNumberOfSources() {
    return m_numOfSources;
}

Source * ControlGrisAudioProcessor::getSources() {
    return sources;
}

//==============================================================================
bool ControlGrisAudioProcessor::createOscConnection(int oscPort) {
    disconnectOSC();

    m_oscConnected = oscSender.connect("127.0.0.1", oscPort);
    if (!m_oscConnected)
        std::cout << "Error: could not connect to UDP port " << oscPort << "." << std::endl;
    else
        m_lastConnectedOSCPort = oscPort;

    return m_oscConnected;
}

bool ControlGrisAudioProcessor::disconnectOSC() {
    if (m_oscConnected) {
        if (oscSender.disconnect()) {
            m_oscConnected = false;
            m_lastConnectedOSCPort = -1;
        }
    }
    return !m_oscConnected;
}

bool ControlGrisAudioProcessor::getOscConnected() {
    return m_oscConnected;
}

void ControlGrisAudioProcessor::handleOscConnection(bool state) {
    if (state) {
        if (m_lastConnectedOSCPort != m_currentOSCPort) {
            createOscConnection(m_currentOSCPort);
        }
    } else {
        disconnectOSC();
    }
    parameters.state.setProperty("oscConnected", getOscConnected(), nullptr);
}

void ControlGrisAudioProcessor::sendOscMessage() {
    if (! m_oscConnected)
        return;

    OSCAddressPattern oscPattern("/spat/serv");
    OSCMessage message(oscPattern);

    for (int i = 0; i < m_numOfSources; i++) {
        message.clear();
        float azim = -sources[i].getAzimuth() / 180.0 * M_PI;
        float elev = (M_PI / 2.0) - (sources[i].getElevation() / 360.0 * M_PI * 2.0);
        message.addInt32(sources[i].getId());
        message.addFloat32(azim);
        message.addFloat32(elev);
        message.addFloat32(sources[i].getAzimuthSpan() * 2.0);
        message.addFloat32(sources[i].getElevationSpan() * 0.5);
        if (m_selectedOscFormat == SPAT_MODE_LBAP) {
            message.addFloat32(sources[i].getDistance() / 0.6);
        } else {
            message.addFloat32(sources[i].getDistance());
        }
        message.addFloat32(0.0);

        if (!oscSender.send(message)) {
            //std::cout << "Error: could not send OSC message." << std::endl;
            return;
        }
    }
}

bool ControlGrisAudioProcessor::createOscInputConnection(int oscPort) {
    disconnectOSCInput();

    m_oscInputConnected = oscReceiver.connect(oscPort);
    if (!m_oscInputConnected) {
        std::cout << "Error: could not connect to UDP input port " << oscPort << "." << std::endl;
    } else {
        oscReceiver.addListener (this, "/controlgris/traj/1/x");
        oscReceiver.addListener (this, "/controlgris/traj/1/y");
        oscReceiver.addListener (this, "/controlgris/traj/1/z");
        oscReceiver.addListener (this, "/controlgris/traj/1/xy");
        oscReceiver.addListener (this, "/controlgris/traj/1/xyz");
        m_currentOSCInputPort = oscPort;
        parameters.state.setProperty("oscInputPortNumber", oscPort, nullptr);
    }

    parameters.state.setProperty("oscInputConnected", getOscInputConnected(), nullptr);

    return m_oscInputConnected;
}

bool ControlGrisAudioProcessor::disconnectOSCInput() {
    if (m_oscInputConnected) {
        if (oscReceiver.disconnect()) {
            m_oscInputConnected = false;
        }
    }

    parameters.state.setProperty("oscInputConnected", getOscInputConnected(), nullptr);

    return !m_oscInputConnected;
}

bool ControlGrisAudioProcessor::getOscInputConnected() {
    return m_oscInputConnected;
}

void ControlGrisAudioProcessor::oscMessageReceived(const OSCMessage& message) {
    String address = message.getAddressPattern().toString().toStdString();
    if (address == "/controlgris/traj/1/x" && automationManager.getDrawingType() == TRAJECTORY_TYPE_REALTIME) {
        automationManager.setSourcePositionX(message[0].getFloat32());
        automationManager.sendTrajectoryPositionChangedEvent();
    } else if (address == "/controlgris/traj/1/y" && automationManager.getDrawingType() == TRAJECTORY_TYPE_REALTIME) {
        automationManager.setSourcePositionY(message[0].getFloat32());
        automationManager.sendTrajectoryPositionChangedEvent();
    } else if (address == "/controlgris/traj/1/z" && automationManagerAlt.getDrawingType() == TRAJECTORY_TYPE_ALT_REALTIME) {
        automationManagerAlt.setSourcePositionY(message[0].getFloat32());
        automationManagerAlt.sendTrajectoryPositionChangedEvent();
    } else if (address == "/controlgris/traj/1/xy" && automationManager.getDrawingType() == TRAJECTORY_TYPE_REALTIME) {
        automationManager.setSourcePositionX(message[0].getFloat32());
        automationManager.setSourcePositionY(message[1].getFloat32());
        automationManager.sendTrajectoryPositionChangedEvent();
    } else if (address == "/controlgris/traj/1/xyz") {
        if (automationManager.getDrawingType() == TRAJECTORY_TYPE_REALTIME) {
            automationManager.setSourcePositionX(message[0].getFloat32());
            automationManager.setSourcePositionY(message[1].getFloat32());
            automationManager.sendTrajectoryPositionChangedEvent();
        }
        if (automationManagerAlt.getDrawingType() == TRAJECTORY_TYPE_ALT_REALTIME) {
            automationManagerAlt.setSourcePositionY(message[2].getFloat32());
            automationManagerAlt.sendTrajectoryPositionChangedEvent();
        }
    }
}

void ControlGrisAudioProcessor::timerCallback() {
    if (m_somethingChanged) {
        m_somethingChanged = false;
    }

    // MainField automation.
    if (automationManager.getActivateState()) {
        if (automationManager.getDrawingType() == TRAJECTORY_TYPE_REALTIME) {
            //...
        } else if (m_lastTimerTime != getCurrentTime()) {
            automationManager.setTrajectoryDeltaTime(getCurrentTime() - getInitTimeOnPlay());
        }
    } else if (m_isPlaying && automationManager.hasValidPlaybackPosition()) {
        automationManager.setSourcePosition(automationManager.getPlaybackPosition());
    }

    // ElevationField automation.
    if (getOscFormat() == SPAT_MODE_LBAP && automationManagerAlt.getActivateState()) {
        if (automationManagerAlt.getDrawingType() == TRAJECTORY_TYPE_ALT_REALTIME) {
            //...
        } else if (m_lastTimerTime != getCurrentTime()) {
            automationManagerAlt.setTrajectoryDeltaTime(getCurrentTime() - getInitTimeOnPlay());
        }
    } else if (m_isPlaying && automationManagerAlt.hasValidPlaybackPosition()) {
        automationManagerAlt.setSourcePosition(automationManagerAlt.getPlaybackPosition());
    }

    m_lastTimerTime = getCurrentTime();

    if (m_canStopActivate && automationManager.getActivateState() && !m_isPlaying) {
        automationManager.setActivateState(false);
    }
    if (m_canStopActivate && automationManagerAlt.getActivateState() && !m_isPlaying) {
        automationManagerAlt.setActivateState(false);
    }
    if (m_canStopActivate && !m_isPlaying) {
        m_canStopActivate = false;
        // Reset source positions on stop.
        for (int i = 0; i < m_numOfSources; i++) {
            sources[i].setPos(sourceInitPositions[i]);
        }
    }

    ControlGrisAudioProcessorEditor *editor = dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor());
    if (editor != nullptr) {
        editor->refresh();
    }

    sendOscMessage();
}

//==============================================================================
void ControlGrisAudioProcessor::setPluginState() {
    // Set global settings values.
    //----------------------------
    setOscFormat((SPAT_MODE_ENUM)(int)parameters.state.getProperty("oscFormat", 0));
    setOscPortNumber(parameters.state.getProperty("oscPortNumber", 18032));
    handleOscConnection(parameters.state.getProperty("oscConnected", true));
    setNumberOfSources(parameters.state.getProperty("numberOfSources", 1), false);
    setFirstSourceId(parameters.state.getProperty("firstSourceId", 1));

    if (parameters.state.getProperty("oscInputConnected", false)) {
        createOscInputConnection(parameters.state.getProperty("oscInputPortNumber", 8000));
    }

    // Set parameter values for sources.
    //----------------------------------
    for (int i = 0; i < m_numOfSources; i++) {
        String id(i);
        sources[i].setNormalizedAzimuth(parameters.state.getProperty(String("p_azimuth_") + id));
        sources[i].setNormalizedElevation(parameters.state.getProperty(String("p_elevation_") + id));
        sources[i].setDistance(parameters.state.getProperty(String("p_distance_") + id));
    }

    ControlGrisAudioProcessorEditor *editor = dynamic_cast<ControlGrisAudioProcessorEditor *>(getActiveEditor());
    if (editor != nullptr) {
        editor->setPluginState();
    }

    sendOscMessage();
}

// Called whenever a source has changed.
//--------------------------------------
void ControlGrisAudioProcessor::setSourceParameterValue(int sourceId, int parameterId, double value) {
    String id(sourceId);
    switch (parameterId) {
        case SOURCE_ID_AZIMUTH:
            sources[sourceId].setNormalizedAzimuth(value);
            parameters.state.setProperty("p_azimuth_" + id, value, nullptr);
            break;
        case SOURCE_ID_ELEVATION:
            sources[sourceId].setNormalizedElevation(value);
            parameters.state.setProperty(String("p_elevation_") + id, value, nullptr);
            break;
        case SOURCE_ID_DISTANCE:
            sources[sourceId].setDistance(value);
            parameters.state.setProperty(String("p_distance_") + id, value, nullptr);
            break;
        case SOURCE_ID_X:
            sources[sourceId].setX(value);
            break;
        case SOURCE_ID_Y:
            sources[sourceId].setY(value);
            break;
        case SOURCE_ID_AZIMUTH_SPAN:
            sources[sourceId].setAzimuthSpan(value);
            parameters.getParameter("azimuthSpan_" + id)->beginChangeGesture();
            parameters.getParameter("azimuthSpan_" + id)->setValueNotifyingHost(value);
            parameters.getParameter("azimuthSpan_" + id)->endChangeGesture();
            break;
        case SOURCE_ID_ELEVATION_SPAN:
            sources[sourceId].setElevationSpan(value);
            parameters.getParameter("elevationSpan_" + id)->beginChangeGesture();
            parameters.getParameter("elevationSpan_" + id)->setValueNotifyingHost(value);
            parameters.getParameter("elevationSpan_" + id)->endChangeGesture();
            break;
    }

    setLinkedParameterValue(sourceId, parameterId);
}

// Checks if link buttons are on and update sources consequently.
//---------------------------------------------------------------
void ControlGrisAudioProcessor::setLinkedParameterValue(int sourceId, int parameterId) {
    bool linkAzimuthSpan = (parameterId == SOURCE_ID_AZIMUTH_SPAN && parameters.state.getProperty("azimuthSpanLink", false));
    bool linkElevationSpan = (parameterId == SOURCE_ID_ELEVATION_SPAN && parameters.state.getProperty("elevationSpanLink", false));
    for (int i = 0; i < m_numOfSources; i++) {
        String id(i);
        if (linkAzimuthSpan) {
            sources[i].setAzimuthSpan(sources[sourceId].getAzimuthSpan());
            parameters.getParameter("azimuthSpan_" + id)->beginChangeGesture();
            parameters.getParameter("azimuthSpan_" + id)->setValueNotifyingHost(sources[i].getAzimuthSpan());
            parameters.getParameter("azimuthSpan_" + id)->endChangeGesture();
        }
        if (linkElevationSpan) {
            sources[i].setElevationSpan(sources[sourceId].getElevationSpan());
            parameters.getParameter("elevationSpan_" + id)->beginChangeGesture();
            parameters.getParameter("elevationSpan_" + id)->setValueNotifyingHost(sources[i].getElevationSpan());
            parameters.getParameter("elevationSpan_" + id)->endChangeGesture();
        }
    }
}

void ControlGrisAudioProcessor::trajectoryPositionChanged(AutomationManager *manager, Point<float> position) {
    if (manager == &automationManager) {
        parameters.getParameter("recordingTrajectory_x")->beginChangeGesture();
        parameters.getParameter("recordingTrajectory_x")->setValueNotifyingHost(position.x);
        parameters.getParameter("recordingTrajectory_x")->endChangeGesture();
        parameters.getParameter("recordingTrajectory_y")->beginChangeGesture();
        parameters.getParameter("recordingTrajectory_y")->setValueNotifyingHost(position.y);
        parameters.getParameter("recordingTrajectory_y")->endChangeGesture();
        linkSourcePositions();
    } else if (manager == &automationManagerAlt) {
        parameters.getParameter("recordingTrajectory_z")->beginChangeGesture();
        parameters.getParameter("recordingTrajectory_z")->setValueNotifyingHost(position.y);
        parameters.getParameter("recordingTrajectory_z")->endChangeGesture();
        linkSourcePositionsAlt();
    }
}

void ControlGrisAudioProcessor::linkSourcePositions() {
    float deltaAzimuth = 0.0f, deltaX = 0.0f, deltaY = 0.0f;

    float delta = kSourceDiameter / (float)FIELD_WIDTH;
    Point<float> autopos = automationManager.getSourcePosition() - Point<float> (0.5, 0.5);
    float mag = sqrtf(autopos.x*autopos.x + autopos.y*autopos.y);
    float ang = atan2f(autopos.y, autopos.x);
    float x = (mag + (mag * delta)) * cosf(ang) + 0.5;
    float y = (mag + (mag * delta)) * sinf(ang) + 0.5;

    switch (automationManager.getSourceLink()) {
        case SOURCE_LINK_INDEPENDENT:
            sources[0].setPos(Point<float> (x, y));
            break;
        case SOURCE_LINK_CIRCULAR:
        case SOURCE_LINK_CIRCULAR_FIXED_RADIUS:
        case SOURCE_LINK_CIRCULAR_FIXED_ANGLE:
        case SOURCE_LINK_CIRCULAR_FULLY_FIXED:
            sources[0].setPos(Point<float> (x, y));
            if (getOscFormat() == SPAT_MODE_LBAP) {
                float deltaAzimuth = sources[0].getDeltaAzimuth();
                float deltaDistance = sources[0].getDeltaDistance();
                for (int i = 1; i < m_numOfSources; i++) {
                    sources[i].setCoordinatesFromFixedSource(deltaAzimuth, 0.0, deltaDistance);
                }
            } else {
                float deltaAzimuth = sources[0].getDeltaAzimuth();
                float deltaElevation = sources[0].getDeltaElevation();
                for (int i = 1; i < m_numOfSources; i++) {
                    sources[i].setCoordinatesFromFixedSource(deltaAzimuth, deltaElevation, 0.0);
                }
            }
            break;
        case SOURCE_LINK_DELTA_LOCK:
            deltaX = automationManager.getSource().getDeltaX();
            deltaY = automationManager.getSource().getDeltaY();
            for (int i = 0; i < m_numOfSources; i++) {
                sources[i].setXYCoordinatesFromFixedSource(deltaX, deltaY);
            }
            break;
    }
}

void ControlGrisAudioProcessor::linkSourcePositionsAlt() {
    float deltaY = 0.0f;

    switch (automationManagerAlt.getSourceLink()) {
        case SOURCE_LINK_ALT_INDEPENDENT:
            sources[0].setNormalizedElevation(automationManagerAlt.getSourcePosition().y);
            break;
        case SOURCE_LINK_ALT_FIXED_ELEVATION:
            for (int i = 0; i < m_numOfSources; i++) {
                sources[i].setNormalizedElevation(automationManagerAlt.getSourcePosition().y);
            }
            break;
        case SOURCE_LINK_ALT_LINEAR_MIN:
            for (int i = 0; i < m_numOfSources; i++) {
                float offset = automationManagerAlt.getSourcePosition().y * 90.0;
                sources[i].setElevation(60.0 / m_numOfSources * i + offset);
            }
            break;
        case SOURCE_LINK_ALT_LINEAR_MAX:
            for (int i = 0; i < m_numOfSources; i++) {
                float offset = 90.0 - automationManagerAlt.getSourcePosition().y * 90.0;
                sources[i].setElevation(90.0 - (60.0 / m_numOfSources * i) - offset);
            }
            break;
        case SOURCE_LINK_ALT_DELTA_LOCK:
            deltaY = automationManagerAlt.getSource().getDeltaY();
            for (int i = 0; i < m_numOfSources; i++) {
                sources[i].setElevationFromFixedSource(deltaY);
            }
            break;
    }
}

//==============================================================================
void ControlGrisAudioProcessor::setPositionPreset(int presetNumber) {
    recallFixedPosition(presetNumber);
    float value = (presetNumber - 1) / (float)NUMBER_OF_POSITION_PRESETS;
    parameters.getParameter("positionPreset")->beginChangeGesture();
    parameters.getParameter("positionPreset")->setValueNotifyingHost(value);
    parameters.getParameter("positionPreset")->endChangeGesture();
}

//==============================================================================
void ControlGrisAudioProcessor::addNewFixedPosition(int id) {
    while (m_lock) {}
    // Build a new fixed position element.
    XmlElement *newData = new XmlElement("ITEM");
    newData->setAttribute("ID", id);
    for (int i = 0; i < MAX_NUMBER_OF_SOURCES; i++) {
        newData->setAttribute(getFixedPosSourceName(i, 0), sources[i].getX());
        newData->setAttribute(getFixedPosSourceName(i, 1), sources[i].getY());
        if (m_selectedOscFormat == SPAT_MODE_LBAP) {
            newData->setAttribute(getFixedPosSourceName(i, 2), sources[i].getNormalizedElevation());
        }
    }
    // Does trajectory really need to be in fixed position ?
    newData->setAttribute("T1_X", automationManager.getSourcePosition().x);
    newData->setAttribute("T1_Y", automationManager.getSourcePosition().y);
    newData->setAttribute("T1_Z", automationManagerAlt.getSourcePosition().y);

    // Replace an element if the new one has the same ID as one already saved.
    bool found = false;
    XmlElement *fpos = fixPositionData.getFirstChildElement();
    while (fpos) {
        if (fpos->getIntAttribute("ID") == id) {
            found = true;
            break;
        }
        fpos = fpos->getNextElement();
    }

    if (found) {
        fixPositionData.replaceChildElement(fpos, newData);
    } else {
        fixPositionData.addChildElement(newData);
    }

    XmlElementDataSorter sorter("ID", true);
    fixPositionData.sortChildElements(sorter);

    recallFixedPosition(id);
    //currentFixPosition = fixPositionData.getFirstChildElement();
}

void ControlGrisAudioProcessor::recallFixedPosition(int id) {
    // called twice on new preset selection...
    bool found = false;
    XmlElement *fpos = fixPositionData.getFirstChildElement();
    while (fpos) {
        if (fpos->getIntAttribute("ID") == id) {
            found = true;
            break;
        }
        fpos = fpos->getNextElement();
    }

    if (! found) {
        return;
    }

    currentFixPosition = fpos;

    float x, y, z = 0.0;
    for (int i = 0; i < m_numOfSources; i++) {
        x = currentFixPosition->getDoubleAttribute(getFixedPosSourceName(i, 0));
        y = currentFixPosition->getDoubleAttribute(getFixedPosSourceName(i, 1));
        sources[i].setPos(Point<float> (x, y));
        sources[i].setFixedPosition(x, y);
        if (m_selectedOscFormat == SPAT_MODE_LBAP) {
            z = currentFixPosition->getDoubleAttribute(getFixedPosSourceName(i, 2));
            sources[i].setFixedElevation(z);
            sources[i].setNormalizedElevation(z);
        }
    }

    x = currentFixPosition->getDoubleAttribute("T1_X");
    y = currentFixPosition->getDoubleAttribute("T1_Y");
    automationManager.setSourcePosition(Point<float> (x, y));
    if (getOscFormat() == SPAT_MODE_LBAP) {
        z = currentFixPosition->getDoubleAttribute("T1_Z");
        automationManagerAlt.setSourcePosition(Point<float> (0.0, z));
    }
}

void ControlGrisAudioProcessor::copyFixedPositionXmlElement(XmlElement *src, XmlElement *dest) {
    if (dest == nullptr)
        dest = new XmlElement(FIXED_POSITION_DATA_TAG);

    forEachXmlChildElement (*src, element) {
        XmlElement *newData = new XmlElement("ITEM");
        newData->setAttribute("ID", element->getIntAttribute("ID"));
        for (int i = 0; i < MAX_NUMBER_OF_SOURCES; i++) {
            newData->setAttribute(getFixedPosSourceName(i, 0), element->getDoubleAttribute(getFixedPosSourceName(i, 0)));
            newData->setAttribute(getFixedPosSourceName(i, 1), element->getDoubleAttribute(getFixedPosSourceName(i, 1)));
            newData->setAttribute(getFixedPosSourceName(i, 2), element->getDoubleAttribute(getFixedPosSourceName(i, 2)));
        }

        newData->setAttribute("T1_X", element->getDoubleAttribute("T1_X"));
        newData->setAttribute("T1_Y", element->getDoubleAttribute("T1_Y"));
        newData->setAttribute("T1_Z", element->getDoubleAttribute("T1_Z"));

        dest->addChildElement(newData);
    }
}

XmlElement * ControlGrisAudioProcessor::getFixedPositionData() {
    return &fixPositionData;
}

void ControlGrisAudioProcessor::deleteFixedPosition(int id) {
    while (m_lock) {}
    bool found = false;
    XmlElement *fpos = fixPositionData.getFirstChildElement();
    while (fpos) {
        if (fpos->getIntAttribute("ID") == id) {
            found = true;
            break;
        }
        fpos = fpos->getNextElement();
    }

    if (found) {
        fixPositionData.removeChildElement(fpos, true);
        XmlElementDataSorter sorter("ID", true);
        fixPositionData.sortChildElements(sorter);
        //currentFixPosition = fixPositionData.getFirstChildElement();
    }
}

//==============================================================================
double ControlGrisAudioProcessor::getInitTimeOnPlay() {
    return m_initTimeOnPlay;
}

double ControlGrisAudioProcessor::getCurrentTime() {
    return m_currentTime;
}

bool ControlGrisAudioProcessor::getIsPlaying() {
    return m_isPlaying;
}

double ControlGrisAudioProcessor::getBPM() {
    return m_bpm;
}

//==============================================================================
const String ControlGrisAudioProcessor::getName() const
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

double ControlGrisAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ControlGrisAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ControlGrisAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ControlGrisAudioProcessor::setCurrentProgram (int index)
{
}

const String ControlGrisAudioProcessor::getProgramName (int index)
{
    return {};
}

void ControlGrisAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void ControlGrisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_needInitialization = true;
    m_lastTime = m_lastTimerTime = 10000000.0;
    m_canStopActivate = true;
    // Keep in memory source positions at the time we hit play.
    for (int i = 0; i < m_numOfSources; i++) {
        sourceInitPositions[i] = sources[i].getPos();
    }
}

void ControlGrisAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ControlGrisAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ControlGrisAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
    m_lock = true;
    AudioPlayHead* phead = getPlayHead();
    if (phead != nullptr) {
        AudioPlayHead::CurrentPositionInfo playposinfo;
        phead->getCurrentPosition(playposinfo);
        m_isPlaying = playposinfo.isPlaying;
        m_bpm = playposinfo.bpm;
        if (m_needInitialization) {
            m_initTimeOnPlay = m_currentTime = playposinfo.timeInSeconds;
            m_needInitialization = false;
        } else {
            m_currentTime = playposinfo.timeInSeconds;
        }
    }
    m_lastTime = m_currentTime;
    m_lock = false;
}

//==============================================================================
bool ControlGrisAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* ControlGrisAudioProcessor::createEditor()
{
    return new ControlGrisAudioProcessorEditor (*this, parameters, automationManager, automationManagerAlt);
}

//==============================================================================
void ControlGrisAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    for (int i = 0; i < MAX_NUMBER_OF_SOURCES; i++) {
        String id(i);
        parameters.state.setProperty(String("p_azimuth_") + id, sources[i].getNormalizedAzimuth(), nullptr);
        parameters.state.setProperty(String("p_elevation_") + id, sources[i].getNormalizedElevation(), nullptr);
        parameters.state.setProperty(String("p_distance_") + id, sources[i].getDistance(), nullptr);
    }

    auto state = parameters.copyState();

    std::unique_ptr<XmlElement> xmlState (state.createXml());

    if (xmlState.get() != nullptr) {
        XmlElement *childExist = xmlState->getChildByName(FIXED_POSITION_DATA_TAG);
        if (childExist) {
            xmlState->removeChildElement(childExist, true);
        }
        if (fixPositionData.getNumChildElements() > 0) {
            XmlElement *positionData = xmlState->createNewChildElement(FIXED_POSITION_DATA_TAG);
            copyFixedPositionXmlElement(&fixPositionData, positionData);
        }
        copyXmlToBinary (*xmlState, destData);
    }
}

void ControlGrisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr) {
        XmlElement *positionData = xmlState->getChildByName(FIXED_POSITION_DATA_TAG);
        if (positionData) {
            fixPositionData.deleteAllChildElements();
            copyFixedPositionXmlElement(positionData, &fixPositionData);
        }
        parameters.replaceState (ValueTree::fromXml (*xmlState));
    }

    if (fixPositionData.getNumChildElements() > 0) {
        currentFixPosition = fixPositionData.getFirstChildElement();
    }

    setPluginState();
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ControlGrisAudioProcessor();
}
