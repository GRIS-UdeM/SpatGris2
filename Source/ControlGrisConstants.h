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

const StringArray SOURCE_PLACEMENT_SKETCH {String("Left Alternate"), String("Right Alternate"),
                                           String("Left Clockwise"), String("Left Counter Clockwise"),
                                           String("Right Clockwise"), String("Right Counter Clockwise"),
                                           String("Top Clockwise"), String("Top Counter Clockwise")};

const StringArray SOURCE_LINK_TYPES       {String("Independant"), String("Circular"),
                                           String("Circular Fixed Radius"), String("Circular Fixed Angle"),
                                           String("Circular Fully Fixed"), String("Delta Lock")};

const StringArray TRAJECTORY_TYPES        {String("Drawing"), String("Circle Clockwise"),
                                           String("Circle Counter Clockwise"), String("Ellipse Clockwise"),
                                           String("Ellipse Counter Clockwise"), String("Spiral Clockwise")};