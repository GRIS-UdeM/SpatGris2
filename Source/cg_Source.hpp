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

#include <array>

#include <JuceHeader.h>

#include "cg_Normalized.hpp"
#include "cg_SourceId.hpp"
#include "cg_SourceIndex.hpp"
#include "cg_constants.hpp"

//==============================================================================
class ControlGrisAudioProcessor;

enum class SourceParameter { azimuth, elevation, distance, x, y, azimuthSpan, elevationSpan };
//==============================================================================
class Source
{
public:
    //==============================================================================
    enum class OriginOfChange { none, userMove, userAnchorMove, link, trajectory, automation, presetRecall, osc };
    enum class ChangeType { position, elevation };
    //==============================================================================
    class Listener : private juce::AsyncUpdater
    {
    public:
        //==============================================================================
        Listener() noexcept = default;
        virtual ~Listener() noexcept = default;

        Listener(Listener const &) = delete;
        Listener(Listener &&) = delete;

        Listener & operator=(Listener const &) = delete;
        Listener & operator=(Listener &&) = delete;
        //==============================================================================
        void update() { triggerAsyncUpdate(); }

    private:
        //==============================================================================
        void handleAsyncUpdate() override { sourceMoved(); }
        virtual void sourceMoved() = 0;
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)

    }; // class Source::Listener

private:
    //==============================================================================
    juce::ListenerList<Listener> mGuiListeners;

    SourceIndex mIndex{};
    SourceId mId{ 1 };
    SpatMode mSpatMode{ SpatMode::dome };

    Radians mAzimuth{};
    Radians mElevation{};
    float mDistance{ 1.0f };

    Point<float> mPosition{};

    Normalized mAzimuthSpan{};
    Normalized mElevationSpan{};

    Colour mColour{ Colours::black };
    ControlGrisAudioProcessor * mProcessor{};

public:
    //==============================================================================
    void setIndex(SourceIndex const index) { mIndex = index; }
    [[nodiscard]] SourceIndex getIndex() const { return mIndex; }

    void setId(SourceId const id) { mId = id; }
    [[nodiscard]] SourceId getId() const { return mId; }

    void setSpatMode(SpatMode const spatMode) { mSpatMode = spatMode; }
    [[nodiscard]] SpatMode getSpatMode() const { return mSpatMode; }

    void setAzimuth(Radians azimuth, OriginOfChange sourceLinkNotification);
    void setAzimuth(Normalized azimuth, OriginOfChange sourceLinkNotification);
    [[nodiscard]] Radians getAzimuth() const { return mAzimuth; }
    [[nodiscard]] Normalized getNormalizedAzimuth() const;

    void setElevation(Radians elevation, OriginOfChange sourceLinkNotification);
    void setElevation(Normalized elevation, OriginOfChange sourceLinkNotification);
    [[nodiscard]] Radians getElevation() const { return mElevation; }
    [[nodiscard]] Normalized getNormalizedElevation() const;

    void setDistance(float distance, OriginOfChange sourceLinkNotification);
    [[nodiscard]] float getDistance() const { return mDistance; }
    void setAzimuthSpan(Normalized azimuthSpan);
    [[nodiscard]] Normalized getAzimuthSpan() const { return mAzimuthSpan; }
    void setElevationSpan(Normalized elevationSpan);
    [[nodiscard]] Normalized getElevationSpan() const { return mElevationSpan; }

    void setCoordinates(Radians azimuth, Radians elevation, float distance, OriginOfChange sourceLinkNotification);
    [[nodiscard]] bool isPrimarySource() const { return mIndex == SourceIndex{ 0 }; }

    void setX(float x, OriginOfChange sourceLinkNotification);
    void setX(Normalized x, OriginOfChange sourceLinkNotification);
    void setY(Normalized y, OriginOfChange sourceLinkNotification);
    [[nodiscard]] float getX() const { return mPosition.getX(); }
    void setY(float y, OriginOfChange sourceLinkNotification);
    [[nodiscard]] float getY() const { return mPosition.getY(); }
    [[nodiscard]] Point<float> const & getPos() const { return mPosition; }
    void setPosition(Point<float> const & pos, OriginOfChange sourceLinkNotification);

    void computeXY();
    void computeAzimuthElevation();

    void setColorFromIndex(int numTotalSources);
    [[nodiscard]] Colour getColour() const { return mColour; }

    void addGuiListener(Listener * listener) { mGuiListeners.add(listener); }
    void removeGuiListener(Listener * listener) { mGuiListeners.remove(listener); }

    void setProcessor(ControlGrisAudioProcessor * processor) { mProcessor = processor; }

    static Point<float> getPositionFromAngle(Radians angle, float radius);
    static Radians getAngleFromPosition(Point<float> const & position);

    static Point<float> clipPosition(Point<float> const & position, SpatMode spatMode);
    static Point<float> clipDomePosition(Point<float> const & position);
    static Point<float> clipCubePosition(Point<float> const & position);

private:
    //==============================================================================
    void notify(ChangeType changeType, OriginOfChange origin);
    void notifyGuiListeners();
    static Radians clipElevation(Radians elevation);
    static float clipCoordinate(float coord);
    //==============================================================================
    JUCE_LEAK_DETECTOR(Source)
};

//==============================================================================
class Sources
{
    //==============================================================================
    struct Iterator {
        Sources * sources;
        int index;
        //==============================================================================
        bool operator!=(Iterator const & other) const { return index != other.index; }
        Iterator & operator++()
        {
            ++index;
            return *this;
        }
        Source & operator*() { return sources->get(index); }
        Source const & operator*() const { return sources->get(index); }
    };
    //==============================================================================
    struct ConstIterator {
        Sources const * sources;
        int index;
        //==============================================================================
        bool operator!=(ConstIterator const & other) const { return index != other.index; }
        ConstIterator & operator++()
        {
            ++index;
            return *this;
        }
        Source const & operator*() const { return sources->get(index); }
    };
    //==============================================================================
    int mSize{ 2 };
    Source mPrimarySource;
    std::array<Source, MAX_NUMBER_OF_SOURCES - 1> mSecondarySources{};

public:
    //==============================================================================
    [[nodiscard]] int size() const { return mSize; }
    void setSize(int size);

    [[nodiscard]] Source & get(int const index)
    {
        jassert(index >= 0 && index < MAX_NUMBER_OF_SOURCES); // TODO: should check for mSize
        if (index == 0) {
            return mPrimarySource;
        }
        return mSecondarySources[static_cast<size_t>(index) - 1u];
    }
    [[nodiscard]] Source const & get(int const index) const
    {
        jassert(index >= 0 && index < MAX_NUMBER_OF_SOURCES); // TODO: should check for mSize
        if (index == 0) {
            return mPrimarySource;
        }
        return mSecondarySources[static_cast<size_t>(index) - 1u];
    }
    [[nodiscard]] Source & get(SourceIndex const index) { return get(index.toInt()); }
    [[nodiscard]] Source const & get(SourceIndex const index) const { return get(index.toInt()); }
    [[nodiscard]] Source & operator[](int const index)
    {
        jassert(index >= 0 && index < MAX_NUMBER_OF_SOURCES); // TODO: should check for mSize
        if (index == 0) {
            return mPrimarySource;
        }
        return mSecondarySources[static_cast<size_t>(index) - 1u];
    }
    [[nodiscard]] Source const & operator[](int const index) const
    {
        jassert(index >= 0 && index < MAX_NUMBER_OF_SOURCES); // TODO: should check for mSize
        if (index == 0) {
            return mPrimarySource;
        }
        return mSecondarySources[static_cast<size_t>(index) - 1u];
    }
    [[nodiscard]] Source & operator[](SourceIndex const index) { return (*this)[index.toInt()]; }
    [[nodiscard]] Source const & operator[](SourceIndex const index) const { return (*this)[index.toInt()]; }

    void init(ControlGrisAudioProcessor * processor)
    {
        SourceIndex currentIndex{};
        mPrimarySource.setIndex(currentIndex++);
        mPrimarySource.setProcessor(processor);
        for (auto & secondarySource : mSecondarySources) {
            secondarySource.setIndex(currentIndex++);
            secondarySource.setProcessor(processor);
        }
    }

    [[nodiscard]] Source & getPrimarySource() { return mPrimarySource; }
    [[nodiscard]] Source const & getPrimarySource() const { return mPrimarySource; }
    [[nodiscard]] auto & getSecondarySources() { return mSecondarySources; }
    [[nodiscard]] auto const & getSecondarySources() const { return mSecondarySources; }

    [[nodiscard]] Iterator begin() { return Iterator{ this, 0 }; }
    [[nodiscard]] ConstIterator begin() const { return ConstIterator{ this, 0 }; }
    [[nodiscard]] Iterator end() { return Iterator{ this, mSize }; }
    [[nodiscard]] ConstIterator end() const { return ConstIterator{ this, mSize }; }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Sources)
};