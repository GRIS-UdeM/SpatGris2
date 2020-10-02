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

#include <JuceHeader.h>

#include "cg_Source.hpp"
#include "cg_SourceSnapshot.hpp"
#include "cg_constants.hpp"

class Source;
class Sources;
class SourcesSnapshots;
class SourceLinkEnforcer;

//==============================================================================
class LinkStrategy
{
    friend SourceLinkEnforcer;
    bool mInitialized{ false };

public:
    //==============================================================================
    LinkStrategy() noexcept = default;
    virtual ~LinkStrategy() noexcept = default;

    LinkStrategy(LinkStrategy const &) = default;
    LinkStrategy(LinkStrategy &&) noexcept = default;

    LinkStrategy & operator=(LinkStrategy const &) = default;
    LinkStrategy & operator=(LinkStrategy &&) = default;
    //==============================================================================
    void computeParameters(Sources const & finalStates, SourcesSnapshots const & initialStates);
    void enforce(Sources & finalStates, SourcesSnapshots const & initialState, SourceIndex const sourceIndex) const;
    void enforce(Sources & finalStates, SourcesSnapshots const & initialState) const;
    [[nodiscard]] SourceSnapshot computeInitialStateFromFinalState(Sources const & finalStates,
                                                                   SourcesSnapshots const & initialStates,
                                                                   SourceIndex const sourceIndex) const;
    bool isInitialized() const { return mInitialized; }
    //==============================================================================
    static std::unique_ptr<LinkStrategy> make(PositionSourceLink sourceLink);
    static std::unique_ptr<LinkStrategy> make(ElevationSourceLink sourceLink);

private:
    //==============================================================================
    virtual void computeParameters_implementation(Sources const & finalStates, SourcesSnapshots const & initialStates)
        = 0;
    virtual void enforce_implementation(Sources & finalStates,
                                        SourcesSnapshots const & initialStates,
                                        SourceIndex const sourceIndex) const = 0;
    [[nodiscard]] virtual SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const = 0;
    //==============================================================================
    JUCE_LEAK_DETECTOR(LinkStrategy)

}; // class LinkStrategy

//==============================================================================
// only use full to recall saved positions
class IndependentStrategy final : public LinkStrategy
{
    void computeParameters_implementation(Sources const &, SourcesSnapshots const &) override {}
    void enforce_implementation(Sources & finalStates,
                                SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         [[maybe_unused]] SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(IndependentStrategy)
};

//==============================================================================
class CircularStrategy final : public LinkStrategy
{
    Radians mRotation{};
    float mRadiusRatio{};
    //==============================================================================
    void computeParameters_implementation(Sources const & finalState, SourcesSnapshots const & initialStates) override;
    void enforce_implementation(Sources & finalStates,
                                SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(CircularStrategy)
};

//==============================================================================
class CircularFixedRadiusStrategy final : public LinkStrategy
{
    Radians mRotation{};
    float mRadius{};
    //==============================================================================
    void computeParameters_implementation(Sources const & finalStates, SourcesSnapshots const & initialStates) override;
    void enforce_implementation(Sources & finalStates,
                                SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(CircularFixedRadiusStrategy)
};

//==============================================================================
class CircularFixedAngleStrategy final : public LinkStrategy
{
    Radians mDeviationPerSource{};
    Radians mPrimarySourceFinalAngle{};
    Radians mRotation{};
    float mRadiusRatio{};
    std::array<int, MAX_NUMBER_OF_SOURCES> mOrdering{};
    //==============================================================================
    void computeParameters_implementation(Sources const & finalStates, SourcesSnapshots const & initialStates) override;
    void enforce_implementation(Sources & finalStates,
                                SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(CircularFixedAngleStrategy)
};

//==============================================================================
// TODO : copy-pasted code from fixedAngle
class CircularFullyFixedStrategy final : public LinkStrategy
{
    Radians mDeviationPerSource{};
    Radians mPrimarySourceFinalAngle{};
    Radians mRotation{};
    float mRadius{};
    std::array<int, MAX_NUMBER_OF_SOURCES> mOrdering{};
    //==============================================================================
    void computeParameters_implementation(Sources const & finalStates, SourcesSnapshots const & initialStates) override;
    void enforce_implementation(Sources & finalStates,
                                SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(CircularFullyFixedStrategy)
};

//==============================================================================
class LinkSymmetricXStrategy final : public LinkStrategy
{
    Point<float> mPrimarySourceFinalPosition;
    //==============================================================================
    void computeParameters_implementation(Sources const & finalStates,
                                          [[maybe_unused]] SourcesSnapshots const & initialStates) override;
    void enforce_implementation(Sources & finalStates,
                                [[maybe_unused]] SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation([[maybe_unused]] Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(LinkSymmetricXStrategy)
};

//==============================================================================
class LinkSymmetricYStrategy final : public LinkStrategy
{
    Point<float> mPrimarySourceFinalPosition;
    //==============================================================================
    void computeParameters_implementation(Sources const & finalStates,
                                          [[maybe_unused]] SourcesSnapshots const & initialStates) override;
    void enforce_implementation(Sources & finalStates,
                                [[maybe_unused]] SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation([[maybe_unused]] Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(LinkSymmetricYStrategy)
};

//==============================================================================
class DeltaLockStrategy final : public LinkStrategy
{
    Point<float> mDelta;
    //==============================================================================
    void computeParameters_implementation(Sources const & finalStates, SourcesSnapshots const & initialStates) override;
    void enforce_implementation(Sources & finalStates,
                                SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(DeltaLockStrategy)
};

//==============================================================================
// only usefuLl to recall saved positions
class IndependentElevationStrategy final : public LinkStrategy
{
    void computeParameters_implementation(Sources const &, SourcesSnapshots const &) override {}
    void enforce_implementation(Sources & finalStates,
                                SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation([[maybe_unused]] Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(IndependentElevationStrategy)
};

//==============================================================================
class FixedElevationStrategy final : public LinkStrategy
{
    Radians mElevation{};
    //==============================================================================
    void computeParameters_implementation(Sources const & finalStates,
                                          [[maybe_unused]] SourcesSnapshots const & initialStates) override;
    void enforce_implementation(Sources & finalStates,
                                [[maybe_unused]] SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(FixedElevationStrategy)
};

//==============================================================================
class LinearMinElevationStrategy final : public LinkStrategy
{
    static constexpr Radians ELEVATION_DIFF{ -MAX_ELEVATION / 3.0f * 2.0f };
    Radians mBaseElevation{};
    Radians mElevationPerSource{};
    //==============================================================================
    void computeParameters_implementation(Sources const & sources, SourcesSnapshots const & snapshots) override;
    void enforce_implementation(Sources & finalStates,
                                [[maybe_unused]] SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation([[maybe_unused]] Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(LinearMinElevationStrategy)
};

//==============================================================================
class LinearMaxElevationStrategy final : public LinkStrategy
{
    static constexpr Radians ELEVATION_DIFF{ MAX_ELEVATION / 3.0f * 2.0f };
    Radians mBaseElevation{};
    Radians mElevationPerSource{};
    //==============================================================================
    void computeParameters_implementation(Sources const & sources, SourcesSnapshots const & snapshots) override;
    void enforce_implementation(Sources & finalStates,
                                [[maybe_unused]] SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation([[maybe_unused]] Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(LinearMaxElevationStrategy)
};

//==============================================================================
class DeltaLockElevationStrategy final : public LinkStrategy
{
    Radians mDelta;
    //==============================================================================
    void computeParameters_implementation(Sources const & sources, SourcesSnapshots const & snapshots) override;
    void enforce_implementation(Sources & finalStates,
                                SourcesSnapshots const & initialStates,
                                SourceIndex const sourceIndex) const override;
    [[nodiscard]] SourceSnapshot
        computeInitialStateFromFinalState_implementation(Sources const & finalStates,
                                                         SourcesSnapshots const & initialStates,
                                                         SourceIndex const sourceIndex) const override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(DeltaLockElevationStrategy)
};