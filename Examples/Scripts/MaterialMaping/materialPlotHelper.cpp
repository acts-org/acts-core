// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "materialPlotHelper.hpp"

#include <iomanip>
#include <ostream>
#include <string>

#include "../../../thirdparty/nlohmann_json/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

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

/// Parse the surface map json file to associate the surface name to each id

void Parse_Json(const json& Det,
                std::map<std::string, std::string>& surface_name) {
  std::string name;
  for (auto& [key, value] : Det.items()) {
    // Check if this the volume key
    if (key == "volumes") {
      // Get the volume json
      auto volj = value;
      for (auto& [vkey, vvalue] : volj.items()) {
        // Get the volume name
        name = vvalue["Name"];
        // If boundaries associate the vol name to their id
        if (!vvalue["boundaries"].empty()) {
          for (auto& [bkey, bvalue] : vvalue["boundaries"].items()) {
            surface_name[bvalue["SGeoid"]] = name;
          }
        }
        // If layer associate the vol name to their id
        if (!vvalue["layers"].empty()) {
          for (auto& [lkey, lvalue] : vvalue["layers"].items()) {
            surface_name[lvalue["Geoid"]] = name;
            // Finally loop over layer components
            for (auto& [lckey, lcvalue] : lvalue.items()) {
              if (lckey == "representing" && !lcvalue.empty()) {
                surface_name[lcvalue["SGeoid"]] = name;
              } else if (lckey == "approach" && !lcvalue.empty()) {
                // Loop over approach surfaces
                for (auto& [askey, asvalue] : lcvalue.items()) {
                  surface_name[asvalue["SGeoid"]] = name;
                }
              } else if (lckey == "sensitive" && !lcvalue.empty()) {
                // Loop over sensitive surfaces
                for (auto& [sskey, ssvalue] : lcvalue.items()) {
                  surface_name[ssvalue["SGeoid"]] = name;
                }
              }
            }
          }
        }
      }
    }
  }
  return;
}

/// Initialise the information on each surface.

void Initialise_info(sinfo& surface_info,
                     const std::map<std::string, std::string>& surface_name,
                     const uint64_t& id, const int& type, const float& pos,
                     const float& range_min, const float& range_max) {
  Acts::GeometryIdentifier ID(id);
  std::ostringstream layerID;
  layerID << ID;
  std::string surface_id = layerID.str();

  std::string Id_temp = surface_id;
  std::string delimiter = " | ";
  size_t del_pos = 0;
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

  if (surface_name.find(surface_id) != surface_name.end()) {
    surface_info.name = surface_name.at(surface_id);
  } else
    surface_info.name = "";

  surface_info.id = surface_id;
  surface_info.pos = pos;
  surface_info.range_min = range_min;
  surface_info.range_max = range_max;
}
