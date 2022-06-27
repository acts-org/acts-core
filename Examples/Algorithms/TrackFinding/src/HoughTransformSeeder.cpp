// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFinding/HoughTransformSeeder.hpp"

#include "Acts/Seeding/BinFinder.hpp"
#include "Acts/Seeding/BinnedSPGroup.hpp"
#include "Acts/Seeding/Seed.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/Seedfinder.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"

#include <stdexcept>

static inline int quant(double min, double max, unsigned nSteps, double val);
static inline double unquant(double min, double max, unsigned nSteps, int step);
template <typename T> static inline std::string to_string(std::vector<T> v);


ActsExamples::HoughTransformSeeder::HoughTransformSeeder(
    ActsExamples::HoughTransformSeeder::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::BareAlgorithm("HoughTransformSeeder", lvl),
      m_cfg(std::move(cfg)) {
  if (m_cfg.inputSpacePoints.empty()) {
    throw std::invalid_argument("Missing space point input collections");
  }

  // require spacepoints or input measurements (or both), but at least one kind of input
  bool foundInput = false;
  for (const auto& i : m_cfg.inputSpacePoints) {
     if (!(i.empty())) {
        foundInput = true;
        if (m_cfg.findLayerIDSP == nullptr) 
           throw std::invalid_argument("We are using some spacepoints but don't have a function pointer to find the ID");
    }
  }
  if (!(m_cfg.inputMeasurements.empty())) {
     foundInput = true;
     if (m_cfg.findLayerIDMeasurement == nullptr) 
        throw std::invalid_argument("We are using some input measurements but don't have a function pointer to find the ID");
  }

  if (!foundInput) {
    throw std::invalid_argument("Missing some kind of input");
  }

  if (m_cfg.outputProtoTracks.empty()) {
    throw std::invalid_argument("Missing hough tracks output collection");
  }
  if (m_cfg.inputSourceLinks.empty()) {
    throw std::invalid_argument("Missing source link input collection");
  }

  if (m_cfg.gridConfig.rMax != m_cfg.seedFinderConfig.rMax) {
    throw std::invalid_argument("Inconsistent config rMax");
  }

  if (m_cfg.gridConfig.deltaRMax != m_cfg.seedFinderConfig.deltaRMax) {
    throw std::invalid_argument("Inconsistent config deltaRMax");
  } 

  if (not m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }

  if (m_cfg.geometrySelection.empty()) {
    throw std::invalid_argument("Missing geometry selection");
  }
  // ensure geometry selection contains only valid inputs
  for (const auto& geoId : m_cfg.geometrySelection) {
    if ((geoId.approach() != 0u) or (geoId.boundary() != 0u) or
        (geoId.sensitive() != 0u)) {
      throw std::invalid_argument(
          "Invalid geometry selection: only volume and layer are allowed to be "
          "set");
    }
  }
  // remove geometry selection duplicates
  //
  // the geometry selections must be mutually exclusive, i.e. if we have a
  // selection that contains both a volume and a layer within that same volume,
  // we would create the space points for the layer twice.
  auto isDuplicate = [](Acts::GeometryIdentifier ref,
                        Acts::GeometryIdentifier cmp) {
    // code assumes ref < cmp and that only volume and layer can be non-zero
    // root node always contains everything
    if (ref.volume() == 0) {
      return true;
    }
    // unequal volumes always means separate hierarchies
    if (ref.volume() != cmp.volume()) {
      return false;
    }
    // within the same volume hierarchy only consider layers
    return (ref.layer() == cmp.layer());
  };
  auto geoSelBeg = m_cfg.geometrySelection.begin();
  auto geoSelEnd = m_cfg.geometrySelection.end();
  // sort geometry selection so the unique filtering works
  std::sort(geoSelBeg, geoSelEnd);
  auto geoSelLastUnique = std::unique(geoSelBeg, geoSelEnd, isDuplicate);
  if (geoSelLastUnique != geoSelEnd) {
    ACTS_WARNING("Removed " << std::distance(geoSelLastUnique, geoSelEnd)
                            << " geometry selection duplicates");
    m_cfg.geometrySelection.erase(geoSelLastUnique, geoSelEnd);
  }
  ACTS_INFO("Hough geometry selection:");
  for (const auto& geoId : m_cfg.geometrySelection) {
    ACTS_INFO("  " << geoId);
  }



  // Fill convenience variables
  m_step_x = (m_cfg.m_xMax - m_cfg.m_xMin) / m_cfg.m_imageSize_x;
  m_step_y = (m_cfg.m_yMax - m_cfg.m_yMin) / m_cfg.m_imageSize_y;
  for (unsigned i = 0; i <= m_cfg.m_imageSize_x; i++)
     m_bins_x.push_back(unquant(m_cfg.m_xMin, m_cfg.m_xMax, m_cfg.m_imageSize_x, i));
  for (unsigned i = 0; i <= m_cfg.m_imageSize_y; i++)
     m_bins_y.push_back(unquant(m_cfg.m_yMin, m_cfg.m_yMax, m_cfg.m_imageSize_y, i));

}

ActsExamples::ProcessCode ActsExamples::HoughTransformSeeder::execute(
   const AlgorithmContext& ctx) const {

   // hopefully not needed in the future!
   std::vector<std::shared_ptr<const MeasurementKludge >> measurementKludges;


   // construct the combined input container of space point pointers from all
   // configured input sources.
   // pre-compute the total size required so we only need to allocate once
   size_t nSpacePoints = 0;
   for (const auto& isp : m_cfg.inputSpacePoints) {
      nSpacePoints += ctx.eventStore.get<SimSpacePointContainer>(isp).size();
   }

   std::vector<const SimSpacePoint*> spacePointPtrs;
   spacePointPtrs.reserve(nSpacePoints);
   for (const auto& isp : m_cfg.inputSpacePoints) {
      for (auto& spacePoint :
              ctx.eventStore.get<SimSpacePointContainer>(isp)) {
         // since the event store owns the space points, their pointers should be
         // stable and we do noet need to create local copies.
         spacePointPtrs.push_back(&spacePoint); 
      }
   }

   // JAA need to use this later, keep for now, perhaps
   auto finder = Acts::Seedfinder<SimSpacePoint>(m_cfg.seedFinderConfig);
   
   const auto& measurements =
      ctx.eventStore.get<MeasurementContainer>(m_cfg.inputMeasurements);
   const auto& sourceLinks =
      ctx.eventStore.get<IndexSourceLinkContainer>(m_cfg.inputSourceLinks);
   
   
   for (Acts::GeometryIdentifier geoId : m_cfg.geometrySelection) {
      // select volume/layer depending on what is set in the geometry id
      auto range = selectLowestNonZeroGeometryObject(sourceLinks, geoId);
      // groupByModule only works with geometry containers, not with an
      // arbitrary range. do the equivalent grouping manually
      auto groupedByModule = makeGroupBy(range, detail::GeometryIdGetter());
   
      for (auto [moduleGeoId, moduleSourceLinks] : groupedByModule) {
         // find corresponding surface
         const Acts::Surface* surface =
            m_cfg.trackingGeometry->findSurface(moduleGeoId);
         if (not surface) {
            ACTS_ERROR("Could not find surface " << moduleGeoId);
            return ProcessCode::ABORT;
         }
      
         for (auto& sourceLink : moduleSourceLinks) {
            // extract a local position/covariance independent from the concrecte
            // measurement content. since we do not know if and where the local
            // parameters are contained in the measurement parameters vector, they
            // are transformed to the bound space where we do know their location.
            // if the local parameters are not measured, this results in a
            // zero location, which is a reasonable default fall-back.
            auto [localPos, localCov] = std::visit(
               [](const auto& meas) {
                  auto expander = meas.expander();
                  Acts::BoundVector par = expander * meas.parameters();
                  Acts::BoundSymMatrix cov =
                  expander * meas.covariance() * expander.transpose();
                  // extract local position
                  Acts::Vector2 lpar(par[Acts::eBoundLoc0], par[Acts::eBoundLoc1]);
                  // extract local position covariance.
                  Acts::SymMatrix2 lcov =
                  cov.block<2, 2>(Acts::eBoundLoc0, Acts::eBoundLoc0);
                  return std::make_pair(lpar, lcov);
               },
               measurements[sourceLink.get().index()]);
         
            // transform local position to global coordinates
            Acts::Vector3 globalFakeMom(1, 1, 1);
            Acts::Vector3 globalPos =
               surface->localToGlobal(ctx.geoContext, localPos, globalFakeMom);
            double r = sqrt(globalPos[Acts::ePos0]*globalPos[Acts::ePos0] + globalPos[Acts::ePos1]*globalPos[Acts::ePos1]);
            double phi = atan2(globalPos[Acts::ePos1],globalPos[Acts::ePos0]);
            unsigned hitlayer = (*(m_cfg.findLayerIDMeasurement))(r);
            std::shared_ptr<const MeasurementKludge> kludge = std::shared_ptr<const MeasurementKludge>(new MeasurementKludge(hitlayer,phi,r,sourceLink.get().index()));
            measurementKludges.push_back(kludge);
         }
      }
   }

   ActsExamples::Image m_image = createImage(spacePointPtrs,measurementKludges);
   static thread_local ProtoTrackContainer protoTracks;
   protoTracks.clear();
   
   for (unsigned y = 0; y < m_cfg.m_imageSize_y; y++)
      for (unsigned x = 0; x < m_cfg.m_imageSize_x; x++)
         if (passThreshold(m_image, x, y))
         {
            ProtoTrack protoTrack;
            for (auto index : m_image(y, x).second) protoTrack.push_back(index);
            protoTracks.push_back(protoTrack);
         }
   ACTS_DEBUG("Created " << protoTracks.size() << " track seeds from "
                        << spacePointPtrs.size() << " space points");
   
   ctx.eventStore.add(m_cfg.outputProtoTracks, ProtoTrackContainer{protoTracks});

   return ActsExamples::ProcessCode::SUCCESS;
}



ActsExamples::Image ActsExamples::HoughTransformSeeder::createLayerImage(unsigned layer, std::vector<const SimSpacePoint*> & spacepoints, std::vector<std::shared_ptr<const MeasurementKludge >> & kludges) const
{
   ActsExamples::Image image(m_cfg.m_imageSize_y, m_cfg.m_imageSize_x);

    for (auto sp : spacepoints) 
    {
       float r = sqrt(sp->x()*sp->x()+sp->y()*sp->y());
       unsigned hitlayer = (*(m_cfg.findLayerIDSP))(r);
       if (hitlayer != layer) continue; 
       float phi = atan2(sp->y(),sp->x());

       // This scans over y (pT) because that is more efficient in memory, in C.
       // Unknown if firmware will want to scan over x instead.
       for (unsigned y_ = 0; y_ < m_cfg.m_imageSize_y; y_++)
       {
          unsigned y_bin_min = y_;
          unsigned y_bin_max = (y_ + 1);
          
          // Find the min/max x bins
          auto xBins = yToXBins(y_bin_min, y_bin_max, r, phi, hitlayer);
          // Update the image
          for (unsigned y = y_bin_min; y < y_bin_max; y++)
             for (unsigned x = xBins.first; x < xBins.second; x++)
             {
                image(y, x).first++;
                image(y, x).second.insert(sp->measurementIndex());
             }
       }
    }

    // now do the same thing for measurements, this is an extra kludge! And we will have to skip the SPs. And we are not separating the two strip sides
    for (auto kludge : kludges)
    {
       if (kludge->layer != layer) continue; 

       // This scans over y (pT) because that is more efficient in memory, in C.
       // Unknown if firmware will want to scan over x instead.
       for (unsigned y_ = 0; y_ < m_cfg.m_imageSize_y; y_++)
       {
          unsigned y_bin_min = y_;
          unsigned y_bin_max = (y_ + 1);
    
          // Find the min/max x bins
          auto xBins = yToXBins(y_bin_min, y_bin_max, kludge->radius, kludge->phi, kludge->layer);
          // Update the image
          for (unsigned y = y_bin_min; y < y_bin_max; y++)
             for (unsigned x = xBins.first; x < xBins.second; x++)
             {
                image(y, x).first++;
                image(y, x).second.insert(kludge->index);
             }
       }
    }

    return image;
}

ActsExamples::Image ActsExamples::HoughTransformSeeder::createImage(std::vector<const SimSpacePoint*> & spacepoints, std::vector<std::shared_ptr<const MeasurementKludge >> & kludges) const
{
   ActsExamples::Image image(m_cfg.m_imageSize_y, m_cfg.m_imageSize_x);

    for (unsigned i = 0; i < m_cfg.m_nLayers; i++)
    {
       Image layerImage = createLayerImage(i, spacepoints, kludges);
        for (unsigned x = 0; x < m_cfg.m_imageSize_x; ++x)
            for (unsigned y = 0; y < m_cfg.m_imageSize_y; ++y)
                if (layerImage(y, x).first > 0)
                {
                    image(y, x).first++;
                    image(y, x).second.insert(layerImage(y, x).second.begin(), layerImage(y, x).second.end());
                }
    }


    return image;
}

bool ActsExamples::HoughTransformSeeder::passThreshold(Image const & image, unsigned x, unsigned y) const
{   

    // Pass window threshold
   unsigned width = m_cfg.m_threshold.size() / 2;
   if (x < width || (image.size(1) - x) < width) return false;
    for (unsigned i = 0; i < m_cfg.m_threshold.size(); i++) {
       if (image(y, x - width + i).first < m_cfg.m_threshold[i]) return false;
    }
   
    // Pass local-maximum check
    if (m_cfg.m_localMaxWindowSize)
        for (int j = -m_cfg.m_localMaxWindowSize; j <= m_cfg.m_localMaxWindowSize; j++)
            for (int i = -m_cfg.m_localMaxWindowSize; i <= m_cfg.m_localMaxWindowSize; i++)
            {
                if (i == 0 && j == 0) continue;
                if (y + j < image.size(0) && x + i < image.size(1))
                {
                    if (image(y+j, x+i).first > image(y, x).first) return false;
                    if (image(y+j, x+i).first == image(y, x).first)
                    {
                        if (image(y+j, x+i).second.size() > image(y, x).second.size()) return false;
                        if (image(y+j, x+i).second.size() == image(y, x).second.size()
                            && j <= 0 && i <= 0) return false; // favor bottom-left (low phi, low neg q/pt)
                    }
                }
            }

     return true;
}

///////////////////////////////////////////////////////////////////////////////
// Helpers


// Quantizes val, given a range [min, max) split into nSteps. Returns the bin below.
static inline int quant(double min, double max, unsigned nSteps, double val)
{
    return static_cast<int>((val - min) / (max - min) * nSteps);
}

// Returns the lower bound of the bin specified by step
static inline double unquant(double min, double max, unsigned nSteps, int step)
{
    return min + (max - min) * step / nSteps;
}

template <typename T>
static inline std::string to_string(std::vector<T> v)
{
    std::ostringstream oss;
    oss << "[";
    if (!v.empty())
    {
        std::copy(v.begin(), v.end()-1, std::ostream_iterator<T>(oss, ", "));
        oss << v.back();
    }
    oss << "]";
    return oss.str();
}



double ActsExamples::HoughTransformSeeder::yToX(double y, double r, double phi) const
{
   double d0 = 0; // d0 correction TO DO allow for this
   double x = asin(r * ActsExamples::HoughTransformSeeder::m_cfg.kA * y - d0 / r) + phi;
    
   if (m_cfg.fieldCorrection) x += (*(m_cfg.fieldCorrection))(0, y, r);

   return x;
}

// Find the min/max x bins of the hit's line, in each y bin. Max is exclusive.
// Note this assumes yToX is monotonic. Returns {0, 0} if hit lies out of bounds.
std::pair<unsigned, unsigned> ActsExamples::HoughTransformSeeder::yToXBins(size_t yBin_min, size_t yBin_max, double r, double phi, unsigned layer) const
{

   double x_min = yToX(m_bins_y[yBin_min], r, phi);
   double x_max = yToX(m_bins_y[yBin_max], r, phi);
   if (x_min > x_max) std::swap(x_min, x_max);
   if (x_max < m_cfg.m_xMin || x_min > m_cfg.m_xMax)
      return { 0, 0 }; // out of bounds
   
   // Get bins
   int x_bin_min = quant(m_cfg.m_xMin, m_cfg.m_xMax, m_cfg.m_imageSize_x, x_min);
   int x_bin_max = quant(m_cfg.m_xMin, m_cfg.m_xMax, m_cfg.m_imageSize_x, x_max) + 1; // exclusive
   
    // Extend bins
    unsigned extend = getExtension(yBin_min, layer);  
    x_bin_min -= extend;
    x_bin_max += extend;
    
    // Clamp bins
    if (x_bin_min < 0) x_bin_min = 0;
    if (x_bin_max > static_cast<int>(m_cfg.m_imageSize_x)) x_bin_max = m_cfg.m_imageSize_x;
    
    return { x_bin_min, x_bin_max };
}

// We allow variable extension based on the size of m_hitExtend_x. See comments below.
unsigned ActsExamples::HoughTransformSeeder::getExtension(unsigned y, unsigned layer) const
{
   if (m_cfg.m_hitExtend_x.size() == m_cfg.m_nLayers) return m_cfg.m_hitExtend_x[layer];
   if (m_cfg.m_hitExtend_x.size() == m_cfg.m_nLayers * 2)
   {
      // different extension for low pt vs high pt, split in half but irrespective of sign
      // first nLayers entries of m_hitExtend_x is for low pt half, rest are for high pt half
      if (y < m_cfg.m_imageSize_y / 4 || y > 3 * m_cfg.m_imageSize_y / 4) return m_cfg.m_hitExtend_x[layer];
      return m_cfg.m_hitExtend_x[m_cfg.m_nLayers + layer];
   }
   return 0;
}


