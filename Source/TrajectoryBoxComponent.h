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
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "GrisLookAndFeel.h"
#include "ControlGrisConstants.h"

class TrajectoryBoxComponent : public Component
{
public:
    TrajectoryBoxComponent();
    ~TrajectoryBoxComponent();

    void paint(Graphics&) override;
    void resized() override;

    void setNumberOfSources(int numOfSources);
    void setSpatMode(SPAT_MODE_ENUM spatMode);
    void setTrajectoryType(int type);
    void setTrajectoryTypeAlt(int type);
    void setBackAndForth(bool state);
    void setBackAndForthAlt(bool state);
    void setDampeningEditorEnabled(bool state);
    void setDampeningAltEditorEnabled(bool state);
    void setDampeningCycles(int value);
    void setDampeningCyclesAlt(int value);
    void setSourceLink(int value);
    void setSourceLinkAlt(int value);
    void setCycleDuration(double value);
    void setDurationUnit(int value);
    void setDeviationPerCycle(float value);

    bool getActivateState();
    bool getActivateAltState();
    void setActivateState(bool state);
    void setActivateAltState(bool state);

    struct Listener
    {
        virtual ~Listener() {}

        virtual void trajectoryBoxSourceLinkChanged(int value) = 0;
        virtual void trajectoryBoxSourceLinkAltChanged(int value) = 0;
        virtual void trajectoryBoxTrajectoryTypeChanged(int value) = 0;
        virtual void trajectoryBoxTrajectoryTypeAltChanged(int value) = 0;
        virtual void trajectoryBoxBackAndForthChanged(bool value) = 0;
        virtual void trajectoryBoxBackAndForthAltChanged(bool value) = 0;
        virtual void trajectoryBoxDampeningCyclesChanged(int value) = 0;
        virtual void trajectoryBoxDampeningCyclesAltChanged(int value) = 0;
        virtual void trajectoryBoxDeviationPerCycleChanged(float value) = 0;
        virtual void trajectoryBoxCycleDurationChanged(double duration, int mode) = 0;
        virtual void trajectoryBoxDurationUnitChanged(double duration, int mode) = 0;
        virtual void trajectoryBoxActivateChanged(bool value) = 0;
        virtual void trajectoryBoxActivateAltChanged(bool value) = 0;
    };

    void addListener(Listener* l) { listeners.add (l); }
    void removeListener(Listener* l) { listeners.remove (l); }

    ComboBox        sourceLinkCombo;
    ComboBox        sourceLinkAltCombo;

private:
    ListenerList<Listener> listeners;

    SPAT_MODE_ENUM  m_spatMode;

    Label           sourceLinkLabel;

    Label           trajectoryTypeLabel;
    ComboBox        trajectoryTypeCombo;
    ComboBox        trajectoryTypeAltCombo;

    ToggleButton    backAndForthToggle;
    ToggleButton    backAndForthAltToggle;

    Label           dampeningLabel;
    TextEditor      dampeningEditor;
    TextEditor      dampeningAltEditor;

    Label           deviationLabel;
    TextEditor      deviationEditor;

    Label           durationLabel;
    TextEditor      durationEditor;
    ComboBox        durationUnitCombo;

    Label           cycleSpeedLabel;
    Slider          cycleSpeedSlider;

    TextButton      activateButton;
    TextButton      activateAltButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrajectoryBoxComponent)
};
