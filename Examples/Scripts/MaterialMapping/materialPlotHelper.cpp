// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "materialPlotHelper.hpp"

#include <iomanip>
#include <ostream>
#include <string>

/// Information on a given surface.

struct sinfo {
  std::string name;
  std::string idname;
  std::string id;
  int type;
  float pos;
  float range_min;
  float range_max;
};

std::ostream& Acts::operator<<(std::ostream& os, Acts::GeometryIdentifier id) {
  os << "[ " << std::setw(3) << id.volume();
  os << " | " << std::setw(3) << id.boundary();
  os << " | " << std::setw(3) << id.layer();
  os << " | " << std::setw(3) << id.approach();
  os << " | " << std::setw(4) << id.sensitive() << " ]";
  return os;
}

/// Initialise the information on each surface.

void Initialise_info(sinfo& surface_info,
                     const std::map<std::string, std::string>& surfaceName,
                     const std::uint64_t& id, const int& type, const float& pos,
                     const float& range_min, const float& range_max) {
  Acts::GeometryIdentifier ID(id);
  std::ostringstream layerID;
  layerID << ID;
  std::string surface_id = layerID.str();

  std::string Id_temp = surface_id;
  std::string delimiter = " | ";
  std::size_t del_pos = 0;
  std::vector<std::string> Ids;
  while ((del_pos = Id_temp.find(delimiter)) != std::string::npos) {
    Ids.push_back(Id_temp.substr(0, del_pos));
    Id_temp.erase(0, del_pos + delimiter.length());
  }
  Ids.push_back(Id_temp);

  for (int tag = 0; tag < 5; tag++) {
    Ids[tag].erase(std::remove(Ids[tag].begin(), Ids[tag].end(), ' '),
                   Ids[tag].end());
    Ids[tag].erase(std::remove(Ids[tag].begin(), Ids[tag].end(), '['),
                   Ids[tag].end());
    Ids[tag].erase(std::remove(Ids[tag].begin(), Ids[tag].end(), ']'),
                   Ids[tag].end());
  }

  surface_info.idname = "v" + Ids[0] + "_b" + Ids[1] + "_l" + Ids[2] + "_a" +
                        Ids[3] + "_s" + Ids[4];
  surface_info.type = type;

  if (surfaceName.contains(surface_id)) {
    surface_info.name = surfaceName.at(surface_id);
  } else {
    surface_info.name = "";
  }

  surface_info.id = surface_id;
  surface_info.pos = pos;
  surface_info.range_min = range_min;
  surface_info.range_max = range_max;
}
