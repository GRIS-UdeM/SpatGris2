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
#include "SettingsBoxComponent.h"

SettingsBoxComponent::SettingsBoxComponent() {
    oscFormatLabel.setText("OSC Format:", NotificationType::dontSendNotification);
    addAndMakeVisible(&oscFormatLabel);

    oscFormatCombo.addItem("DOME - SpatGris", 1);
    oscFormatCombo.addItem("CUBE - SpatGris", 2);
    oscFormatCombo.onChange = [this] {
            listeners.call([&] (Listener& l) { l.settingsBoxOscFormatChanged((SPAT_MODE_ENUM)(oscFormatCombo.getSelectedId()-1)); });
        };
    oscFormatCombo.setSelectedId(1);
    addAndMakeVisible(&oscFormatCombo);

    oscPortLabel.setText("OSC Port:", NotificationType::dontSendNotification);
    addAndMakeVisible(&oscPortLabel);

    String defaultPort("18032");
    oscPortEditor.setExplicitFocusOrder(4);
    oscPortEditor.setText(defaultPort);
    oscPortEditor.setInputRestrictions(5, "0123456789");
    oscPortEditor.onReturnKey = [this] {
            oscFormatCombo.grabKeyboardFocus();
        };
    oscPortEditor.onFocusLost = [this, defaultPort] {
            if (! oscPortEditor.isEmpty()) {
                listeners.call([&] (Listener& l) { l.settingsBoxOscPortNumberChanged(oscPortEditor.getText().getIntValue()); });
            } else {
                listeners.call([&] (Listener& l) { l.settingsBoxOscPortNumberChanged(defaultPort.getIntValue()); oscPortEditor.setText(defaultPort); });
            }
        };
    addAndMakeVisible(&oscPortEditor);

    numOfSourcesLabel.setText("Number of Sources:", NotificationType::dontSendNotification);
    addAndMakeVisible(&numOfSourcesLabel);

    numOfSourcesEditor.setExplicitFocusOrder(2);
    numOfSourcesEditor.setText("2");
    numOfSourcesEditor.setInputRestrictions(1, "12345678");
    numOfSourcesEditor.onReturnKey = [this] {
            oscFormatCombo.grabKeyboardFocus();
        };
    numOfSourcesEditor.onFocusLost = [this] {
            if (! numOfSourcesEditor.isEmpty()) {
                listeners.call([&] (Listener& l) { l.settingsBoxNumberOfSourcesChanged(numOfSourcesEditor.getText().getIntValue()); });
            } else {
                listeners.call([&] (Listener& l) { l.settingsBoxNumberOfSourcesChanged(1); numOfSourcesEditor.setText("1"); });
            }
        };
    addAndMakeVisible(&numOfSourcesEditor);

    firstSourceIdLabel.setText("First Source ID:", NotificationType::dontSendNotification);
    addAndMakeVisible(&firstSourceIdLabel);

    firstSourceIdEditor.setExplicitFocusOrder(3);
    firstSourceIdEditor.setText("1");
    firstSourceIdEditor.setInputRestrictions(3, "0123456789");
    firstSourceIdEditor.onReturnKey = [this] {
            oscFormatCombo.grabKeyboardFocus();
        };
    firstSourceIdEditor.onFocusLost = [this] {
            if (! firstSourceIdEditor.isEmpty()) {
                listeners.call([&] (Listener& l) { l.settingsBoxFirstSourceIdChanged(firstSourceIdEditor.getText().getIntValue()); });
            } else {
                listeners.call([&] (Listener& l) { l.settingsBoxFirstSourceIdChanged(1); firstSourceIdEditor.setText("1"); });
            }
        };
    addAndMakeVisible(&firstSourceIdEditor);

    activateButton.setExplicitFocusOrder(1);
    activateButton.setButtonText("Activate OSC");
    activateButton.onClick = [this] {
            listeners.call([&] (Listener& l) { l.settingsBoxOscActivated(activateButton.getToggleState()); });
        };
    addAndMakeVisible(&activateButton);
}

SettingsBoxComponent::~SettingsBoxComponent() {
    setLookAndFeel(nullptr);
}

void SettingsBoxComponent::setOscFormat(SPAT_MODE_ENUM mode) {
    oscFormatCombo.setSelectedId((int)(mode+1), NotificationType::dontSendNotification);
}

void SettingsBoxComponent::setOscPortNumber(int oscPortNumber) {
    oscPortEditor.setText(String(oscPortNumber));
}

void SettingsBoxComponent::setNumberOfSources(int numOfSources) {
    numOfSourcesEditor.setText(String(numOfSources));
}

void SettingsBoxComponent::setFirstSourceId(int firstSourceId) {
    firstSourceIdEditor.setText(String(firstSourceId));
}

void SettingsBoxComponent::setActivateButtonState(bool shouldBeOn) {
    activateButton.setToggleState(shouldBeOn, NotificationType::dontSendNotification);
}

void SettingsBoxComponent::paint(Graphics& g) {
    GrisLookAndFeel *lookAndFeel;
    lookAndFeel = static_cast<GrisLookAndFeel *> (&getLookAndFeel());
    g.fillAll (lookAndFeel->findColour (ResizableWindow::backgroundColourId));
}

void SettingsBoxComponent::resized() {
    oscFormatLabel.setBounds(5, 10, 90, 15);
    oscFormatCombo.setBounds(95, 10, 150, 20);

    oscPortLabel.setBounds(5, 40, 90, 15);
    oscPortEditor.setBounds(95, 40, 150, 20);

    numOfSourcesLabel.setBounds(265, 10, 130, 15);
    numOfSourcesEditor.setBounds(395, 10, 40, 15);

    firstSourceIdLabel.setBounds(265, 40, 130, 15);
    firstSourceIdEditor.setBounds(395, 40, 40, 15);

    activateButton.setBounds(5, 70, 150, 20);
}
