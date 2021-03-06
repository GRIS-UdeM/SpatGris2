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

#define SOURCE_ID_AZIMUTH 0
#define SOURCE_ID_ELEVATION 1
#define SOURCE_ID_DISTANCE 2
#define SOURCE_ID_X 3
#define SOURCE_ID_Y 4
#define SOURCE_ID_AZIMUTH_SPAN 5
#define SOURCE_ID_ELEVATION_SPAN 6

static const float kSourceRadius = 12;
static const float kSourceDiameter = kSourceRadius * 2;

class Source
{
public:
    Source();
    ~Source();

    void setId(int id);
    int getId();

    void setRadiusIsElevation(bool shouldBeElevation);

    void setAzimuth(float azimuth);
    void setNormalizedAzimuth(float value);
    float getAzimuth();
    float getNormalizedAzimuth();
    void setElevationNoClip(float elevation);
    void setElevation(float elevation);
    void setNormalizedElevation(float value);
    float getElevation();
    float getNormalizedElevation();
    void setDistance(float distance);
    void setDistanceNoClip(float distance);
    float getDistance();
    void setAzimuthSpan(float azimuthSpan);
    float getAzimuthSpan();
    void setElevationSpan(float elevationSpan);
    float getElevationSpan();

    void setCoordinates(float azimuth, float elevation, float distance);

    void setX(float x);
    float getX();
    void setY(float y);
    float getY();
    Point<float> getPos();
    void setPos(Point<float> pos);

    void computeXY();
    void computeAzimuthElevation();

    void setFixedPosition(float x, float y);
    void setFixedElevation(float z);

    void setSymmetricX(float x, float y);
    void setSymmetricY(float x, float y);

    void fixSourcePosition(bool shouldBeFixed);
    void fixSourcePositionElevation(bool shouldBeFixed);

    float getDeltaX();
    float getDeltaY();
    float getDeltaAzimuth();
    float getDeltaElevation();
    float getDeltaDistance();

    void setCoordinatesFromFixedSource(float deltaAzimuth, float deltaElevation, float deltaDistance);
    void setXYCoordinatesFromFixedSource(float deltaX, float deltaY);
    void setElevationFromFixedSource(float deltaY);

    void setColour(Colour col);
    Colour getColour();

private:
    int m_id;
    bool m_changed;
    bool m_radiusIsElevation;

    float m_azimuth;
    float m_elevation;
    float m_elevationNoClip;
    float m_distance;
    float m_distanceNoClip;
    float m_aziSpan;
    float m_eleSpan;
    float m_x;
    float m_y;

    Colour colour;

    float fixedAzimuth;
    float fixedElevation;
    float fixedDistance;
    float fixedX;
    float fixedY;

    inline double degreeToRadian(float degree) { return (degree / 360.0 * 2.0 * M_PI); }

};
