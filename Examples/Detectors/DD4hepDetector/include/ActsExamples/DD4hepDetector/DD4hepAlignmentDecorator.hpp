#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include <Acts/Geometry/TrackingGeometry.hpp>

#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ActsExamples {
struct AlgorithmContext;

namespace DD4hep {
class DD4hepAlignmentDecorator : public IContextDecorator {
 public:
  using LayerStore = std::vector<std::shared_ptr<Acts::DD4hepDetectorElement>>;
  using DetectorStore = std::vector<LayerStore>;
  struct Config {
    // whether use the nominal geometry
    bool nominal = true;
    // path of Json file which is used to store the misalignment matrix of each
    // detector element
    // @todo use `JsonMisalignmentConfig`
    std::string misAlignedGeoJsonPath = "odd-misalignment-matrix.json";
    // tracking geometry
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry = nullptr;
  };

  DD4hepAlignmentDecorator(const Config& cfg,
                           std::unique_ptr<const Acts::Logger> logger =
                               Acts::getDefaultLogger("AlignmentDecorator",
                                                      Acts::Logging::INFO));
  ~DD4hepAlignmentDecorator() override = default;
  ProcessCode decorate(AlgorithmContext& context) override;
  const std::string& name() const override { return m_name; }

 private:
  Config m_cfg;                                  ///< the configuration class
  std::unique_ptr<const Acts::Logger> m_logger;  ///!< the logging instance
  const Acts::Logger& logger() const { return *m_logger; }
  std::string m_name = "Aligned Detector";
  std::unordered_map<std::string, Acts::Transform3>
      m_misalignmentAtConstruction;
  std::unordered_map<std::string, Acts::Transform3> m_nominalStore;
  std::unordered_map<std::string, Acts::Transform3> m_mistransform;
  void parseGeometry(const Acts::TrackingGeometry& tGeometry);
  void initializeMisFromJson(const std::string& misAlignedGeoJsonFile);
};

inline void DD4hepAlignmentDecorator::initializeMisFromJson(
    const std::string& misJson) {
  std::ifstream file(misJson);
  if (!file.is_open())
    throw std::runtime_error("Unable to open file");
  nlohmann::json jsonData;
  file >> jsonData;
  for (auto& [key, value] : jsonData.items()) {
    if (value.is_array() && value.size() == 6) {
      double x = value[0].get<double>();
      double y = value[1].get<double>();
      double z = value[2].get<double>();
      double alpha = value[3].get<double>() / 180 * M_PI;
      double beta = value[4].get<double>() / 180 * M_PI;
      double gamma = value[5].get<double>() / 180 * M_PI;
      Acts::Transform3 translation =
          Eigen::Affine3d(Eigen::Translation3d(x, y, z));
      Acts::Transform3 delta_rotationx =
          Eigen::Affine3d(Eigen::AngleAxisd(alpha, Eigen::Vector3d::UnitX()));
      Acts::Transform3 delta_rotationy =
          Eigen::Affine3d(Eigen::AngleAxisd(beta, Eigen::Vector3d::UnitY()));
      Acts::Transform3 delta_rotationz =
          Eigen::Affine3d(Eigen::AngleAxisd(gamma, Eigen::Vector3d::UnitZ()));
      m_misalignmentAtConstruction[key] =
          translation * delta_rotationx * delta_rotationy * delta_rotationz;
    }
  }
  std::cout << "Successfully initialize the JSON file" << std::endl;
}
}  // namespace DD4hep
}  // namespace ActsExamples
