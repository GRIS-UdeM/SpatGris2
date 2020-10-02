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

#include "cg_ElevationSourceComponent.hpp"

#include "cg_ControlGrisAudioProcessor.hpp"
#include "cg_FieldComponent.hpp"
#include "cg_Source.hpp"

//==============================================================================
ElevationSourceComponent::ElevationSourceComponent(ElevationFieldComponent & fieldComponent, Source & source) noexcept
    : SourceComponent(source.getColour(), source.getId().toString())
    , mFieldComponent(fieldComponent)
    , mAutomationManager(fieldComponent.getAutomationManager())
    , mSource(source)
{
    source.addGuiListener(this);
    updatePositionInParent();
}

//==============================================================================
ElevationSourceComponent::~ElevationSourceComponent() noexcept
{
    mSource.removeGuiListener(this);
}

//==============================================================================
void ElevationSourceComponent::updatePositionInParent()
{
    auto const newCenter{
        mFieldComponent.sourceElevationToComponentPosition(mSource.getElevation(), mSource.getIndex()).roundToInt()
    };
    setCentrePosition(newCenter.getX(), newCenter.getY());
}

//==============================================================================
void ElevationSourceComponent::sourceMoved()
{
    updatePositionInParent();
}

//==============================================================================
void ElevationSourceComponent::mouseDown(MouseEvent const & event)
{
    mDisplacementMode = getDisplacementMode(event);
    if (mSource.isPrimarySource() || mDisplacementMode == DisplacementMode::all) {
        mAutomationManager.getProcessor().getChangeGestureManager().beginGesture(Automation::Ids::Z);
    }
    setSourcePosition(event);
}

//==============================================================================
void ElevationSourceComponent::setSourcePosition(MouseEvent const & event) const
{
    auto const eventRelativeToFieldComponent{ event.getEventRelativeTo(&mFieldComponent) };
    auto const newElevation{ mFieldComponent.componentPositionToSourceElevation(
        eventRelativeToFieldComponent.getPosition().toFloat()) };
    auto const sourceLinkBehavior{ mDisplacementMode == DisplacementMode::selectedSourceOnly
                                       ? Source::OriginOfChange::userAnchorMove
                                       : Source::OriginOfChange::userMove };
    mSource.setElevation(newElevation, sourceLinkBehavior);
}

//==============================================================================
void ElevationSourceComponent::mouseDrag(MouseEvent const & event)
{
    if (mFieldComponent.getSelectedSourceIndex() == mSource.getIndex()) {
        jassert(mFieldComponent.getWidth() == mFieldComponent.getHeight());
        setSourcePosition(event);
    }
}

//==============================================================================
void ElevationSourceComponent::mouseUp(MouseEvent const & event)
{
    mouseDrag(event);
    if (mSource.isPrimarySource() || mDisplacementMode == DisplacementMode::all) {
        mAutomationManager.getProcessor().getChangeGestureManager().endGesture(Automation::Ids::Z);
    }
}

//==============================================================================
SourceIndex ElevationSourceComponent::getSourceIndex() const
{
    return mSource.getIndex();
}