// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace Acts {

void ObjVisualization3D::vertex(const Vector3& vtx, Color color) {
  auto& o = object();
  o.vertexColors[o.vertices.size()] = color;
  o.vertices.push_back(vtx.template cast<ValueType>());
}

void ObjVisualization3D::line(const Vector3& a, const Vector3& b, Color color) {
  auto& o = object();
  if (color != Color{0, 0, 0}) {
    o.lineColors[o.lines.size()] = color;
  }
  // not implemented
  vertex(a, color);
  vertex(b, color);
  o.lines.push_back({o.vertices.size() - 2, o.vertices.size() - 1});
}

void ObjVisualization3D::face(const std::vector<Vector3>& vtxs, Color color) {
  auto& o = object();
  if (color != Color{0, 0, 0}) {
    o.faceColors[o.faces.size()] = color;
  }
  FaceType idxs;
  idxs.reserve(vtxs.size());
  for (const auto& vtx : vtxs) {
    vertex(vtx, color);
    idxs.push_back(o.vertices.size() - 1);
  }
  o.faces.push_back(std::move(idxs));
}

void ObjVisualization3D::faces(const std::vector<Vector3>& vtxs,
                               const std::vector<FaceType>& faces,
                               Color color) {
  auto& o = object();
  // No faces given - call the face() method
  if (faces.empty()) {
    face(vtxs, color);
  } else {
    if (color != Color{0, 0, 0}) {
      o.faceColors[o.faces.size()] = color;
    }
    auto vtxoffs = o.vertices.size();
    if (color != Color{0, 0, 0}) {
      o.vertexColors[o.vertices.size()] = color;
    }
    o.vertices.insert(o.vertices.end(), vtxs.begin(), vtxs.end());
    for (const auto& face : faces) {
      if (face.size() == 2) {
        o.lines.push_back({face[0] + vtxoffs, face[2] + vtxoffs});
      } else {
        FaceType rawFace;
        std::ranges::transform(
            face, std::back_inserter(rawFace),
            [&](unsigned long iv) { return (iv + vtxoffs); });
        o.faces.push_back(rawFace);
      }
    }
  }
}

void ObjVisualization3D::write(const std::filesystem::path& path) const {
  std::ofstream os;
  std::filesystem::path objectpath = path;
  if (!objectpath.has_extension()) {
    objectpath.replace_extension(std::filesystem::path("obj"));
  }
  os.open(std::filesystem::absolute(objectpath).string());
  std::filesystem::path mtlpath = objectpath;
  mtlpath.replace_extension(std::filesystem::path("mtl"));

  const std::string mtlpathString = std::filesystem::absolute(mtlpath).string();
  os << "mtllib " << mtlpathString << "\n";
  std::ofstream mtlos;
  mtlos.open(mtlpathString);

  write(os, mtlos);
  os.close();
  mtlos.close();
}

void ObjVisualization3D::write(std::ostream& os) const {
  std::stringstream sterile;
  write(os, sterile);
}

void ObjVisualization3D::write(std::ostream& os, std::ostream& mos) const {
  std::map<std::string, bool, std::less<>> materials;

  auto mixColor = [&](const Color& color) {
    std::string materialName;
    materialName = "material_";
    materialName += std::to_string(color[0]) + std::string("_");
    materialName += std::to_string(color[1]) + std::string("_");
    materialName += std::to_string(color[2]);

    if (!materials.contains(materialName)) {
      mos << "newmtl " << materialName << "\n";
      std::vector<std::string> shadings = {"Ka", "Kd", "Ks"};
      for (const auto& shd : shadings) {
        mos << shd << " " << std::to_string(color[0] / 256.) << " ";
        mos << std::to_string(color[1] / 256.) << " ";
        mos << std::to_string(color[2] / 256.) << " " << "\n";
      }
      mos << "\n";
    }
    return std::string("usemtl ") + materialName;
  };

  std::size_t vertexOffset = 0;
  for (const auto& o : m_objects) {
    if (!o.name.empty()) {
      os << "o " << o.name << "\n";
    }

    std::size_t iv = 0;
    Color lastVertexColor = {0, 0, 0};
    for (const VertexType& vtx : o.vertices) {
      if (o.vertexColors.contains(iv)) {
        auto color = o.vertexColors.find(iv)->second;
        if (color != lastVertexColor) {
          os << mixColor(color) << "\n";
          lastVertexColor = color;
        }
      }

      os << "v " << std::setprecision(m_outputPrecision)
         << m_outputScalor * vtx.x() << " " << m_outputScalor * vtx.y() << " "
         << m_outputScalor * vtx.z() << "\n";
      ++iv;
    }
    std::size_t il = 0;
    Color lastLineColor = {0, 0, 0};
    for (const auto& [start, end] : o.lines) {
      if (o.lineColors.contains(il)) {
        auto color = o.lineColors.find(il)->second;
        if (color != lastLineColor) {
          os << mixColor(color) << "\n";
          lastLineColor = color;
        }
      }
      os << "l " << vertexOffset + start + 1 << " " << vertexOffset + end + 1
         << "\n";
      ++il;
    }
    std::size_t is = 0;
    Color lastFaceColor = {0, 0, 0};
    for (const FaceType& fc : o.faces) {
      if (o.faceColors.contains(is)) {
        auto color = o.faceColors.find(is)->second;
        if (color != lastFaceColor) {
          os << mixColor(color) << "\n";
          lastFaceColor = color;
        }
      }
      os << "f";
      for (std::size_t fi : fc) {
        os << " " << vertexOffset + fi + 1;
      }
      os << "\n";
      ++is;
    }

    vertexOffset += iv;
  }
}

void ObjVisualization3D::clear() {
  m_objects.clear();
}

void ObjVisualization3D::object(const std::string& name) {
  if (name.empty()) {
    throw std::invalid_argument{"Object name can not be empty"};
  }
  m_objects.push_back(Object{.name = name});
}

ObjVisualization3D::Object& ObjVisualization3D::object() {
  if (m_objects.empty()) {
    m_objects.push_back(Object{.name = ""});
  }

  return m_objects.back();
}

const ObjVisualization3D::Object& ObjVisualization3D::object() const {
  if (m_objects.empty()) {
    throw std::runtime_error{"No objects present"};
  }

  return m_objects.back();
}

}  // namespace Acts
