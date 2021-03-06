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
#include "ControlGrisConstants.h"

ControlGrisAudioProcessorEditor::ControlGrisAudioProcessorEditor (ControlGrisAudioProcessor& p,
                                                                  AudioProcessorValueTreeState& vts,
                                                                  AutomationManager& automan,
                                                                  AutomationManager& automanAlt)
    : AudioProcessorEditor (&p),
      processor (p),
      valueTreeState (vts), 
      automationManager (automan), 
      automationManagerAlt (automanAlt), 
      mainField (automan),
      elevationField (automanAlt)
{ 
    setLookAndFeel(&grisLookAndFeel);

    m_isInsideSetPluginState = false;
    m_selectedSource = 0;

    // Set up the interface.
    //----------------------
    mainBanner.setLookAndFeel(&grisLookAndFeel);
    mainBanner.setText("Azimuth - Elevation", NotificationType::dontSendNotification);
    addAndMakeVisible(&mainBanner);

    elevationBanner.setLookAndFeel(&grisLookAndFeel);
    elevationBanner.setText("Elevation", NotificationType::dontSendNotification);
    addAndMakeVisible(&elevationBanner);

    trajectoryBanner.setLookAndFeel(&grisLookAndFeel);
    trajectoryBanner.setText("Trajectories", NotificationType::dontSendNotification);
    addAndMakeVisible(&trajectoryBanner);

    settingsBanner.setLookAndFeel(&grisLookAndFeel);
    settingsBanner.setText("Configuration", NotificationType::dontSendNotification);
    addAndMakeVisible(&settingsBanner);

    positionPresetBanner.setLookAndFeel(&grisLookAndFeel);
    positionPresetBanner.setText("Preset", NotificationType::dontSendNotification);
    addAndMakeVisible(&positionPresetBanner);

    mainField.setLookAndFeel(&grisLookAndFeel);
    mainField.addListener(this);
    addAndMakeVisible(&mainField);

    elevationField.setLookAndFeel(&grisLookAndFeel);
    elevationField.addListener(this);
    addAndMakeVisible(&elevationField);

    parametersBox.setLookAndFeel(&grisLookAndFeel);
    parametersBox.addListener(this);
    addAndMakeVisible(&parametersBox);

    trajectoryBox.setLookAndFeel(&grisLookAndFeel);
    trajectoryBox.addListener(this);
    addAndMakeVisible(trajectoryBox);
    trajectoryBox.setSourceLink(automationManager.getSourceLink());
    trajectoryBox.setSourceLinkAlt(automationManagerAlt.getSourceLink());

    settingsBox.setLookAndFeel(&grisLookAndFeel);
    settingsBox.addListener(this);

    sourceBox.setLookAndFeel(&grisLookAndFeel);
    sourceBox.addListener(this);

    interfaceBox.setLookAndFeel(&grisLookAndFeel);
    interfaceBox.addListener(this);

    Colour bg = grisLookAndFeel.findColour (ResizableWindow::backgroundColourId);

    configurationComponent.setLookAndFeel(&grisLookAndFeel);
    configurationComponent.setColour(TabbedComponent::backgroundColourId, bg);
    configurationComponent.addTab ("Settings", bg, &settingsBox, false);
    configurationComponent.addTab ("Source", bg, &sourceBox, false);
    configurationComponent.addTab ("Controllers", bg, &interfaceBox, false);
    addAndMakeVisible(configurationComponent);

    positionPresetBox.setLookAndFeel(&grisLookAndFeel);
    positionPresetBox.addListener(this);
    addAndMakeVisible(&positionPresetBox);

    // Add sources to the fields.
    //---------------------------
    mainField.setSources(processor.getSources(), processor.getNumberOfSources());
    elevationField.setSources(processor.getSources(), processor.getNumberOfSources());

    parametersBox.setSelectedSource(&processor.getSources()[m_selectedSource]);
    processor.setSelectedSourceId(m_selectedSource);

    // Manage dynamic window size of the plugin.
    //------------------------------------------
    setResizeLimits(MIN_FIELD_WIDTH + 50, MIN_FIELD_WIDTH + 20, 1800, 1300);

    lastUIWidth .referTo (processor.parameters.state.getChildWithName ("uiState").getPropertyAsValue ("width",  nullptr));
    lastUIHeight.referTo (processor.parameters.state.getChildWithName ("uiState").getPropertyAsValue ("height", nullptr));

    // set our component's initial size to be the last one that was stored in the filter's settings
    setSize(lastUIWidth.getValue(), lastUIHeight.getValue());

    lastUIWidth.addListener(this);
    lastUIHeight.addListener(this);

    // Load the last saved state of the plugin.
    //-----------------------------------------
    setPluginState();
}

ControlGrisAudioProcessorEditor::~ControlGrisAudioProcessorEditor() {
    configurationComponent.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void ControlGrisAudioProcessorEditor::setPluginState() {
    m_isInsideSetPluginState = true;

    // Set global settings values.
    //----------------------------
    settingsBoxOscFormatChanged(processor.getOscFormat());
    settingsBoxOscPortNumberChanged(processor.getOscPortNumber());
    settingsBoxOscActivated(processor.getOscConnected());
    settingsBoxFirstSourceIdChanged(processor.getFirstSourceId());
    settingsBoxNumberOfSourcesChanged(processor.getNumberOfSources());

    interfaceBox.setOscOutputPluginId(valueTreeState.state.getProperty("oscOutputPluginId", 1));
    interfaceBox.setOscReceiveToggleState(valueTreeState.state.getProperty("oscInputConnected", false));
    interfaceBox.setOscReceiveInputPort(valueTreeState.state.getProperty("oscInputPortNumber", 9000));

    interfaceBox.setOscSendToggleState(valueTreeState.state.getProperty("oscOutputConnected", false));
    interfaceBox.setOscSendOutputAddress(valueTreeState.state.getProperty("oscOutputAddress", "192.168.1.100"));
    interfaceBox.setOscSendOutputPort(valueTreeState.state.getProperty("oscOutputPortNumber", 8000));

    // Set state for trajectory box persistent values.
    //------------------------------------------------
    trajectoryBox.setTrajectoryType(valueTreeState.state.getProperty("trajectoryType", 1));
    trajectoryBox.setTrajectoryTypeAlt(valueTreeState.state.getProperty("trajectoryTypeAlt", 1));
    trajectoryBox.setBackAndForth(valueTreeState.state.getProperty("backAndForth", false));
    trajectoryBox.setBackAndForthAlt(valueTreeState.state.getProperty("backAndForthAlt", false));
    trajectoryBox.setDampeningCycles(valueTreeState.state.getProperty("dampeningCycles", 0));
    automationManager.setDampeningCycles(valueTreeState.state.getProperty("dampeningCycles", 0));
    trajectoryBox.setDampeningCyclesAlt(valueTreeState.state.getProperty("dampeningCyclesAlt", 0));
    automationManagerAlt.setDampeningCycles(valueTreeState.state.getProperty("dampeningCyclesAlt", 0));
    trajectoryBox.setDeviationPerCycle(valueTreeState.state.getProperty("deviationPerCycle", 0));
    automationManager.setDeviationPerCycle(valueTreeState.state.getProperty("deviationPerCycle", 0));
    trajectoryBox.setCycleDuration(valueTreeState.state.getProperty("cycleDuration", 5.0));
    trajectoryBox.setDurationUnit(valueTreeState.state.getProperty("durationUnit", 1));

    // Update the position preset box.
    //--------------------------------
    for (int i = 0; i < NUMBER_OF_POSITION_PRESETS; i++) {
        positionPresetBox.presetSaved(i+1, false);
    }
    XmlElement *positionData = processor.getFixedPositionData();
    XmlElement *fpos = positionData->getFirstChildElement();
    while (fpos) {
        positionPresetBox.presetSaved(fpos->getIntAttribute("ID"), true);
        fpos = fpos->getNextElement();
    }

    // Update the interface.
    //----------------------
    parametersBox.setSelectedSource(&processor.getSources()[m_selectedSource]);
    mainField.setSelectedSource(m_selectedSource);
    elevationField.setSelectedSource(m_selectedSource);
    processor.setSelectedSourceId(m_selectedSource);
    sourceBox.updateSelectedSource(&processor.getSources()[m_selectedSource], m_selectedSource, processor.getOscFormat());

    int preset = (int)((float)valueTreeState.getParameterAsValue("positionPreset").getValue());
    positionPresetBox.setPreset(preset, true);

    m_isInsideSetPluginState = false;
}

void ControlGrisAudioProcessorEditor::updateSpanLinkButton(bool state) {
    parametersBox.setSpanLinkState(state);
}

void ControlGrisAudioProcessorEditor::updateSourceLinkCombo(int value) {
    trajectoryBox.sourceLinkCombo.setSelectedId(value, NotificationType::dontSendNotification);
}

void ControlGrisAudioProcessorEditor::updateSourceLinkAltCombo(int value) {
    trajectoryBox.sourceLinkAltCombo.setSelectedId(value, NotificationType::dontSendNotification);
}

void ControlGrisAudioProcessorEditor::updatePositionPreset(int presetNumber) {
    positionPresetBox.setPreset(presetNumber, true);
}

// Value::Listener callback. Called when the stored window size changes.
//----------------------------------------------------------------------
void ControlGrisAudioProcessorEditor::valueChanged (Value&) {
    setSize (lastUIWidth.getValue(), lastUIHeight.getValue());
}

// SettingsBoxComponent::Listener callbacks.
//------------------------------------------
void ControlGrisAudioProcessorEditor::settingsBoxOscFormatChanged(SPAT_MODE_ENUM mode) {
    settingsBox.setOscFormat(mode);
    processor.setOscFormat(mode);
    bool selectionIsLBAP = mode == SPAT_MODE_LBAP;
    parametersBox.setDistanceEnabled(selectionIsLBAP);
    mainField.setSpatMode(mode);
    trajectoryBox.setSpatMode(mode);
    repaint();
    resized();
}

void ControlGrisAudioProcessorEditor::settingsBoxOscPortNumberChanged(int oscPort) {
    processor.setOscPortNumber(oscPort);
    settingsBox.setOscPortNumber(oscPort);
}

void ControlGrisAudioProcessorEditor::settingsBoxOscActivated(bool state) {
    processor.handleOscConnection(state);
    settingsBox.setActivateButtonState(processor.getOscConnected());
}

void ControlGrisAudioProcessorEditor::settingsBoxNumberOfSourcesChanged(int numOfSources) {
    bool initSourcePlacement = false;
    if (processor.getNumberOfSources() != numOfSources || m_isInsideSetPluginState) {
        if (processor.getNumberOfSources() != numOfSources) {
            initSourcePlacement = true;
        }
        if (numOfSources != 2 && (automationManager.getSourceLink() == SOURCE_LINK_SYMMETRIC_X ||
                                  automationManager.getSourceLink() == SOURCE_LINK_SYMMETRIC_Y)) {
            automationManager.setSourceLink(SOURCE_LINK_INDEPENDENT);
            updateSourceLinkCombo(SOURCE_LINK_INDEPENDENT);
        }
        m_selectedSource = 0;
        processor.setNumberOfSources(numOfSources);
        processor.setSelectedSourceId(m_selectedSource);
        settingsBox.setNumberOfSources(numOfSources);
        trajectoryBox.setNumberOfSources(numOfSources);
        parametersBox.setSelectedSource(&processor.getSources()[m_selectedSource]);
        mainField.setSources(processor.getSources(), numOfSources);
        elevationField.setSources(processor.getSources(), numOfSources);
        sourceBox.setNumberOfSources(numOfSources, processor.getFirstSourceId());
        if (initSourcePlacement) {
            sourceBoxPlacementChanged(SOURCE_PLACEMENT_LEFT_ALTERNATE);
        }
    }
}

void ControlGrisAudioProcessorEditor::settingsBoxFirstSourceIdChanged(int firstSourceId) {
    processor.setFirstSourceId(firstSourceId);
    settingsBox.setFirstSourceId(firstSourceId);
    parametersBox.setSelectedSource(&processor.getSources()[m_selectedSource]);
    sourceBox.setNumberOfSources(processor.getNumberOfSources(), firstSourceId);

    mainField.repaint();
    if (processor.getOscFormat() == SPAT_MODE_LBAP)
        elevationField.repaint();
}

// SourceBoxComponent::Listener callbacks.
//----------------------------------------
void ControlGrisAudioProcessorEditor::sourceBoxSelectionChanged(int sourceNum) {
    m_selectedSource = sourceNum;

    parametersBox.setSelectedSource(&processor.getSources()[m_selectedSource]);
    mainField.setSelectedSource(m_selectedSource);
    elevationField.setSelectedSource(m_selectedSource);
    processor.setSelectedSourceId(m_selectedSource);
    sourceBox.updateSelectedSource(&processor.getSources()[m_selectedSource], m_selectedSource, processor.getOscFormat());
}

void ControlGrisAudioProcessorEditor::sourceBoxPlacementChanged(int value) {
    int numOfSources = processor.getNumberOfSources();
    const float azims2[2] = {-90.0f, 90.0f};
    const float azims4[4] = {-45.0f, 45.0f, -135.0f, 135.0f};
    const float azims6[6] = {-30.0f, 30.0f, -90.0f, 90.0f, -150.0f, 150.0f};
    const float azims8[8] = {-22.5f, 22.5f, -67.5f, 67.5f, -112.5f, 112.5f, -157.5f, 157.5f};

    bool isLBAP = processor.getOscFormat() == SPAT_MODE_LBAP;

    float offset = 360.0f / numOfSources / 2.0f;
    float distance = isLBAP ? 0.7f : 1.0f;

    switch(value) {
        case SOURCE_PLACEMENT_LEFT_ALTERNATE:
            for (int i = 0; i < numOfSources; i++) {
                if (numOfSources <= 2)
                    processor.getSources()[i].setCoordinates(-azims2[i], isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
                else if (numOfSources <= 4)
                    processor.getSources()[i].setCoordinates(-azims4[i], isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
                else if (numOfSources <= 6)
                    processor.getSources()[i].setCoordinates(-azims6[i], isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
                else
                    processor.getSources()[i].setCoordinates(-azims8[i], isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
            }
            break;
        case SOURCE_PLACEMENT_RIGHT_ALTERNATE:
            for (int i = 0; i < numOfSources; i++) {
                if (numOfSources <= 2)
                    processor.getSources()[i].setCoordinates(azims2[i], isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
                else if (numOfSources <= 4)
                    processor.getSources()[i].setCoordinates(azims4[i], isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
                else if (numOfSources <= 6)
                    processor.getSources()[i].setCoordinates(azims6[i], isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
                else
                    processor.getSources()[i].setCoordinates(azims8[i], isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
            }
            break;
        case SOURCE_PLACEMENT_LEFT_CLOCKWISE:
            for (int i = 0; i < numOfSources; i++) {
                processor.getSources()[i].setCoordinates(360.0f / numOfSources * -i + offset, isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
            }
            break;
        case SOURCE_PLACEMENT_LEFT_COUNTER_CLOCKWISE:
            for (int i = 0; i < numOfSources; i++) {
                processor.getSources()[i].setCoordinates(360.0f / numOfSources * i + offset, isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
            }
            break;
        case SOURCE_PLACEMENT_RIGHT_CLOCKWISE:
            for (int i = 0; i < numOfSources; i++) {
                processor.getSources()[i].setCoordinates(360.0f / numOfSources * -i - offset, isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
            }
            break;
        case SOURCE_PLACEMENT_RIGHT_COUNTER_CLOCKWISE:
            for (int i = 0; i < numOfSources; i++) {
                processor.getSources()[i].setCoordinates(360.0f / numOfSources * i - offset, isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
            }
            break;
        case SOURCE_PLACEMENT_TOP_CLOCKWISE:
            for (int i = 0; i < numOfSources; i++) {
                processor.getSources()[i].setCoordinates(360.0f / numOfSources * -i, isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
            }
            break;
        case SOURCE_PLACEMENT_TOP_COUNTER_CLOCKWISE:
            for (int i = 0; i < numOfSources; i++) {
                processor.getSources()[i].setCoordinates(360.0f / numOfSources * i, isLBAP ? processor.getSources()[i].getElevation() : 0.0f, distance);
            }
            break;
    }

    for (int i = 0; i < numOfSources; i++) {
        processor.setSourceParameterValue(i, SOURCE_ID_AZIMUTH, processor.getSources()[i].getNormalizedAzimuth());
        processor.setSourceParameterValue(i, SOURCE_ID_ELEVATION, processor.getSources()[i].getNormalizedElevation());
        processor.setSourceParameterValue(i, SOURCE_ID_DISTANCE, processor.getSources()[i].getDistance());
    }

    sourceBox.updateSelectedSource(&processor.getSources()[m_selectedSource], m_selectedSource, processor.getOscFormat());

    for (int i = 0; i < numOfSources; i++) {
        processor.getSources()[i].fixSourcePosition(true);
    }

    automationManager.setDrawingType(automationManager.getDrawingType(), processor.getSources()[0].getPos());

    repaint();
}

void ControlGrisAudioProcessorEditor::sourceBoxPositionChanged(int sourceNum, float angle, float rayLen) {
    if (processor.getOscFormat() == SPAT_MODE_LBAP) {
        float currentElevation = processor.getSources()[sourceNum].getElevation();
        processor.getSources()[sourceNum].setCoordinates(angle, currentElevation, rayLen);
    } else {
        processor.getSources()[sourceNum].setCoordinates(angle, 90.0f - (rayLen * 90.0f), 1.0f);
    }

    processor.getSources()[sourceNum].fixSourcePosition(true);

    automationManager.setDrawingType(automationManager.getDrawingType(), processor.getSources()[0].getPos());

    repaint();
}

// ParametersBoxComponent::Listener callbacks.
//--------------------------------------------
void ControlGrisAudioProcessorEditor::parametersBoxParameterChanged(int parameterId, double value) {
    processor.setSourceParameterValue(m_selectedSource, parameterId, value);

    mainField.repaint();
    if (processor.getOscFormat() == SPAT_MODE_LBAP)
        elevationField.repaint();
}

void ControlGrisAudioProcessorEditor::parametersBoxSelectedSourceClicked() {
    m_selectedSource = (m_selectedSource + 1) % processor.getNumberOfSources();
    parametersBox.setSelectedSource(&processor.getSources()[m_selectedSource]);
    mainField.setSelectedSource(m_selectedSource);
    elevationField.setSelectedSource(m_selectedSource);
    processor.setSelectedSourceId(m_selectedSource);
    sourceBox.updateSelectedSource(&processor.getSources()[m_selectedSource], m_selectedSource, processor.getOscFormat());
}

// TrajectoryBoxComponent::Listener callbacks.
//--------------------------------------------
void ControlGrisAudioProcessorEditor::trajectoryBoxSourceLinkChanged(int value) {
    processor.setSourceLink(value);
    mainField.repaint();
}

void ControlGrisAudioProcessorEditor::trajectoryBoxSourceLinkAltChanged(int value) {
    processor.setSourceLinkAlt(value);
    elevationField.repaint();
}

void ControlGrisAudioProcessorEditor::trajectoryBoxTrajectoryTypeChanged(int value) {
    valueTreeState.state.setProperty("trajectoryType", value, nullptr);
    automationManager.setDrawingType(value, processor.getSources()[0].getPos());
    mainField.repaint();
}

void ControlGrisAudioProcessorEditor::trajectoryBoxTrajectoryTypeAltChanged(int value) {
    valueTreeState.state.setProperty("trajectoryTypeAlt", value, nullptr);
    automationManagerAlt.setDrawingTypeAlt(value);
    elevationField.repaint();
}

void ControlGrisAudioProcessorEditor::trajectoryBoxBackAndForthChanged(bool value) {
    valueTreeState.state.setProperty("backAndForth", value, nullptr);
    automationManager.setBackAndForth(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxBackAndForthAltChanged(bool value) {
    valueTreeState.state.setProperty("backAndForthAlt", value, nullptr);
    automationManagerAlt.setBackAndForth(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxDampeningCyclesChanged(int value) {
    valueTreeState.state.setProperty("dampeningCycles", value, nullptr);
    automationManager.setDampeningCycles(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxDampeningCyclesAltChanged(int value) {
    valueTreeState.state.setProperty("dampeningCyclesAlt", value, nullptr);
    automationManagerAlt.setDampeningCycles(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxDeviationPerCycleChanged(float value) {
    valueTreeState.state.setProperty("deviationPerCycle", value, nullptr);
    automationManager.setDeviationPerCycle(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxCycleDurationChanged(double duration, int mode) {
    valueTreeState.state.setProperty("cycleDuration", duration, nullptr);
    double dur = duration;
    if (mode == 2) {
        dur = duration * 60.0 / processor.getBPM();
    }
    automationManager.setPlaybackDuration(dur);
    automationManagerAlt.setPlaybackDuration(dur);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxDurationUnitChanged(double duration, int mode) {
    valueTreeState.state.setProperty("durationUnit", mode, nullptr);
    double dur = duration;
    if (mode == 2) {
        dur = duration * 60.0 / processor.getBPM();
    }
    automationManager.setPlaybackDuration(dur);
    automationManagerAlt.setPlaybackDuration(dur);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxActivateChanged(bool value) {
    automationManager.setActivateState(value);
}

void ControlGrisAudioProcessorEditor::trajectoryBoxActivateAltChanged(bool value) {
    automationManagerAlt.setActivateState(value);
}

// Update the interface if anything has changed (mostly automations).
//-------------------------------------------------------------------
void ControlGrisAudioProcessorEditor::refresh() {
    parametersBox.setSelectedSource(&processor.getSources()[m_selectedSource]);
    sourceBox.updateSelectedSource(&processor.getSources()[m_selectedSource], m_selectedSource, processor.getOscFormat());

    mainField.setIsPlaying(processor.getIsPlaying());
    elevationField.setIsPlaying(processor.getIsPlaying());

    mainField.repaint();
    if (processor.getOscFormat() == SPAT_MODE_LBAP)
        elevationField.repaint();

    if (trajectoryBox.getActivateState() != automationManager.getActivateState()) {
        trajectoryBox.setActivateState(automationManager.getActivateState());
    }
    if (trajectoryBox.getActivateAltState() != automationManagerAlt.getActivateState()) {
        trajectoryBox.setActivateAltState(automationManagerAlt.getActivateState());
    }
}

// FieldComponent::Listener callback.
//-----------------------------------
void ControlGrisAudioProcessorEditor::fieldSourcePositionChanged(int sourceId, int whichField) {
    processor.sourcePositionChanged(sourceId, whichField);
    m_selectedSource = sourceId;
    parametersBox.setSelectedSource(&processor.getSources()[sourceId]);
    mainField.setSelectedSource(m_selectedSource);
    elevationField.setSelectedSource(m_selectedSource);
    processor.setSelectedSourceId(m_selectedSource);
    sourceBox.updateSelectedSource(&processor.getSources()[m_selectedSource], m_selectedSource, processor.getOscFormat());

    processor.setPositionPreset(0);
    positionPresetBox.setPreset(0);
}

void ControlGrisAudioProcessorEditor::fieldTrajectoryHandleClicked(int whichField) {
    if (whichField == 0) {
        automationManager.fixSourcePosition();
        processor.onSourceLinkChanged(automationManager.getSourceLink());
    } else {
        automationManagerAlt.fixSourcePosition();
        processor.onSourceLinkAltChanged(automationManagerAlt.getSourceLink());
    }
}

// PositionPresetComponent::Listener callback.
//--------------------------------------------
void ControlGrisAudioProcessorEditor::positionPresetChanged(int presetNumber) {
    processor.setPositionPreset(presetNumber);
}

void ControlGrisAudioProcessorEditor::positionPresetSaved(int presetNumber) {
    processor.addNewFixedPosition(presetNumber);
}

void ControlGrisAudioProcessorEditor::positionPresetDeleted(int presetNumber) {
    processor.deleteFixedPosition(presetNumber);
}

// InterfaceBoxComponent::Listener callback.
//------------------------------------------
void ControlGrisAudioProcessorEditor::oscOutputPluginIdChanged(int value) {
    processor.setOscOutputPluginId(value);
}

void ControlGrisAudioProcessorEditor::oscInputConnectionChanged(bool state, int oscPort) {
    if (state) {
        processor.createOscInputConnection(oscPort);
    } else {
        processor.disconnectOSCInput(oscPort);
    }
}

void ControlGrisAudioProcessorEditor::oscOutputConnectionChanged(bool state, String oscAddress, int oscPort) {
    if (state) {
        processor.createOscOutputConnection(oscAddress, oscPort);
    } else {
        processor.disconnectOSCOutput(oscAddress, oscPort);
    }
}

//==============================================================================
void ControlGrisAudioProcessorEditor::paint (Graphics& g) {
    GrisLookAndFeel *lookAndFeel;
    lookAndFeel = static_cast<GrisLookAndFeel *> (&getLookAndFeel());
    g.fillAll (lookAndFeel->findColour (ResizableWindow::backgroundColourId));
}

void ControlGrisAudioProcessorEditor::resized() {
    double width = getWidth() - 50; // Remove position preset space. 
    double height = getHeight();

    double fieldSize = width / 2;
    if (fieldSize < MIN_FIELD_WIDTH) { fieldSize = MIN_FIELD_WIDTH; }

    automationManager.setFieldWidth(fieldSize);
    automationManagerAlt.setFieldWidth(fieldSize);

    mainBanner.setBounds(0, 0, fieldSize, 20);
    mainField.setBounds(0, 20, fieldSize, fieldSize);

    if (processor.getOscFormat() == SPAT_MODE_LBAP) {
        mainBanner.setText("Azimuth - Distance", NotificationType::dontSendNotification);
        elevationBanner.setVisible(true);
        elevationField.setVisible(true);
        elevationBanner.setBounds(fieldSize, 0, fieldSize, 20);
        elevationField.setBounds(fieldSize, 20, fieldSize, fieldSize);
    } else {
        mainBanner.setText("Azimuth - Elevation", NotificationType::dontSendNotification);
        elevationBanner.setVisible(false);
        elevationField.setVisible(false);
    }

    parametersBox.setBounds(0, fieldSize + 20, width, 50);

    trajectoryBanner.setBounds(0, fieldSize + 70, width, 20);
    trajectoryBox.setBounds(0, fieldSize + 90, width, 160);

    settingsBanner.setBounds(0, fieldSize + 250, width, 20);
    configurationComponent.setBounds(0, fieldSize + 270, width, 130);

    lastUIWidth  = getWidth();
    lastUIHeight = getHeight();

    positionPresetBanner.setBounds(width, 0, 50, 20);
    positionPresetBox.setBounds(width, 20, 50, height - 20);
}
