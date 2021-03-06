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

// Global variables.
//------------------
const int MIN_FIELD_WIDTH = 300;
const int MAX_NUMBER_OF_SOURCES = 8;
const int NUMBER_OF_POSITION_PRESETS = 50;

// Spatialisation modes.
//----------------------
enum SPAT_MODE_ENUM {
    SPAT_MODE_VBAP = 0,
    SPAT_MODE_LBAP
};

const String WARNING_CIRCULAR_SOURCE_SELECTION ("Only source 1 can be selected \nin circular or symmetric links!");

// Configuration -> Source tab -> Source Placement popup choices.
//---------------------------------------------------------------
enum SOURCE_PLACEMENT_ENUM {    SOURCE_PLACEMENT_LEFT_ALTERNATE = 1,
                                SOURCE_PLACEMENT_RIGHT_ALTERNATE,
                                SOURCE_PLACEMENT_LEFT_CLOCKWISE,
                                SOURCE_PLACEMENT_LEFT_COUNTER_CLOCKWISE,
                                SOURCE_PLACEMENT_RIGHT_CLOCKWISE,
                                SOURCE_PLACEMENT_RIGHT_COUNTER_CLOCKWISE,
                                SOURCE_PLACEMENT_TOP_CLOCKWISE,
                                SOURCE_PLACEMENT_TOP_COUNTER_CLOCKWISE
                           };
const StringArray SOURCE_PLACEMENT_SKETCH {String("Left Alternate"), String("Right Alternate"),
                                           String("Left Clockwise"), String("Left Counter Clockwise"),
                                           String("Right Clockwise"), String("Right Counter Clockwise"),
                                           String("Top Clockwise"), String("Top Counter Clockwise")};

// Trajectories -> Source Link popup choices.
//-------------------------------------------
enum SOURCE_LINK_ENUM { SOURCE_LINK_INDEPENDENT = 1,
                        SOURCE_LINK_CIRCULAR,
                        SOURCE_LINK_CIRCULAR_FIXED_RADIUS,
                        SOURCE_LINK_CIRCULAR_FIXED_ANGLE,
                        SOURCE_LINK_CIRCULAR_FULLY_FIXED,
                        SOURCE_LINK_DELTA_LOCK,
                        SOURCE_LINK_SYMMETRIC_X,
                        SOURCE_LINK_SYMMETRIC_Y
                      };
const StringArray SOURCE_LINK_TYPES       {String("Independent"), String("Circular"),
                                           String("Circular Fixed Radius"), String("Circular Fixed Angle"),
                                           String("Circular Fully Fixed"), String("Delta Lock"),
                                           String("Symmetric X"), String("Symmetric Y")};

enum SOURCE_LINK_ALT_ENUM { SOURCE_LINK_ALT_INDEPENDENT = 1,
                            SOURCE_LINK_ALT_FIXED_ELEVATION,
                            SOURCE_LINK_ALT_LINEAR_MIN,
                            SOURCE_LINK_ALT_LINEAR_MAX,
                            SOURCE_LINK_ALT_DELTA_LOCK
                      };
const StringArray SOURCE_LINK_ALT_TYPES   {String("Independent"), String("Fixed Elevation"),
                                           String("Linear Min"), String("Linear Max"), String("Delta Lock")};

// Trajectories -> Trajectory Type popup choices.
//-----------------------------------------------
enum TRAJECTORY_TYPE_ENUM { TRAJECTORY_TYPE_REALTIME = 1,
                            TRAJECTORY_TYPE_DRAWING,
                            TRAJECTORY_TYPE_CIRCLE_CLOCKWISE,
                            TRAJECTORY_TYPE_CIRCLE_COUNTER_CLOCKWISE,
                            TRAJECTORY_TYPE_ELLIPSE_CLOCKWISE,
                            TRAJECTORY_TYPE_ELLIPSE_COUNTER_CLOCKWISE,
                            TRAJECTORY_TYPE_SPIRAL_CLOCKWISE_OUT_IN,
                            TRAJECTORY_TYPE_SPIRAL_COUNTER_CLOCKWISE_OUT_IN,
                            TRAJECTORY_TYPE_SPIRAL_CLOCKWISE_IN_OUT,
                            TRAJECTORY_TYPE_SPIRAL_COUNTER_CLOCKWISE_IN_OUT,
                            TRAJECTORY_TYPE_SQUARE_CLOCKWISE,
                            TRAJECTORY_TYPE_SQUARE_COUNTER_CLOCKWISE,
                            TRAJECTORY_TYPE_TRIANGLE_CLOCKWISE,
                            TRAJECTORY_TYPE_TRIANGLE_COUNTER_CLOCKWISE
                          };
const StringArray TRAJECTORY_TYPE_TYPES   {String("Realtime"), String("Drawing"), String("Circle Clockwise"),
                                           String("Circle Counter Clockwise"), String("Ellipse Clockwise"),
                                           String("Ellipse Counter Clockwise"), String("Spiral Clockwise Out In"),
                                           String("Spiral Counter Clockwise Out In"), String("Spiral Clockwise In Out"),
                                           String("Spiral Counter Clockwise In Out"), String("Square Clockwise"),
                                           String("Square Counter Clockwise"), String("Triangle Clockwise"),
                                           String("Triangle Counter Clockwise")};

enum TRAJECTORY_TYPE_ALT_ENUM { TRAJECTORY_TYPE_ALT_REALTIME = 1,
                                TRAJECTORY_TYPE_ALT_DRAWING,
                                TRAJECTORY_TYPE_ALT_DOWN_UP,
                                TRAJECTORY_TYPE_ALT_UP_DOWN
                              };
const StringArray TRAJECTORY_TYPE_ALT_TYPES    {String("Realtime"), String("Drawing"), String("Up Down"),
                                                String("Down Up")};

// Fix position data headers.
//---------------------------
const StringArray FIXED_POSITION_DATA_HEADERS { String("ID"),
                                                String("S1_X"), String("S1_Y"), String("S1_Z"),
                                                String("S2_X"), String("S2_Y"), String("S2_Z"),
                                                String("S3_X"), String("S3_Y"), String("S3_Z"),
                                                String("S4_X"), String("S4_Y"), String("S4_Z"),
                                                String("S5_X"), String("S5_Y"), String("S5_Z"),
                                                String("S6_X"), String("S6_Y"), String("S6_Z"),
                                                String("S7_X"), String("S7_Y"), String("S7_Z"),
                                                String("S8_X"), String("S8_Y"), String("S8_Z"),
                                                String("T1_X"), String("T1_Y"), String("T1_Z"),
                                                String("T2_X"), String("T2_Y"), String("T2_Z")};

const String FIXED_POSITION_DATA_TAG ("Fix_Position_Data");
