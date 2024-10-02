// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

const char* head_xml =
    R""""(<?xml version="1.0" encoding="UTF-8"?>
<lccdd xmlns:compact="http://www.lcsim.org/schemas/compact/1.0" xmlns:xs="http://www.w3.org/2001/XMLSchema" xs:noNamespaceSchemaLocation="http://www.lcsim.org/schemas/compact/1.0/compact.xsd">
    <info name="Cylinder" title="Cylinder Detector" author="Andreas.Salzburger@cern.ch" url="" status="alpha" version="0">
                <comment>Test conversion of cylinder elements</comment>
    </info>

    <materials>
        <element Z="1" formula="H" name="H" >
            <atom type="A" unit="g/mol" value="1.00794" />
        </element>
        <material name="Vacuum">
            <D type="density" unit="g/cm3" value="0.00000001" />
            <fraction n="1" ref="H" />
        </material>
        <material name="Air">
            <D type="density" unit="g/cm3" value="0.00000001" />
            <fraction n="1" ref="H" />
        </material>
    </materials>

    <define>
        <!--World-->
        <constant name="world_size" value="10.*m"/>
        <constant name="world_x" value="world_size"/>
        <constant name="world_y" value="world_size"/>
        <constant name="world_z" value="world_size"/>
    </define>
)"""";

const char* segmentation_xml =
    R""""(<readouts>
    <readout name="PixelReadout">
      <segmentation type="CartesianGridXY" grid_size_x="0.05*mm" grid_size_y="0.05*mm"/>
      <id>system:4,layer:4,stave:8,module:4,sensor:1,x:24:-12,y:-12</id>
    </readout>
  </readouts>
  )"""";

const char* end_xml = "</lccdd>";
