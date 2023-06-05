//basing on Ortho 

#pragma once

#include "Acts/EventData/SpacePointData.hpp"
#include "Acts/Seeding/InternalSeed.hpp"
#include "Acts/Seeding/InternalSpacePoint.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/SeedFinderConfig.hpp"
#include "Acts/Seeding/SeedFinderFTFConfig.hpp"
#include "Acts/Utilities/KDTree.hpp"

//load space point needs this: 
#include "Acts/Seeding/GNN_DataStorage.h"


#include <array>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>


namespace Acts {
    //classes from load space point 
// class TrigSiSpacePointBase; 
// class TrigInDetTriplet;
// class TrigFTF_GNN_DataStorage;
// class IRoiDescriptor;

template <typename external_spacepoint_t>
class SeedFinderFTF { 
 public: 
 //define memebers 
  //since update 
  static constexpr std::size_t NDims = 3;

  // TrigFTF_GNN_Geometry<SpacePoint> mGNNgeo ; 
  //mstorage 
  // Acts::SeedFilter<SpacePoint> mstorage;   
  //think the functions wont work if a mmeber about doesnt wokr 
  //so think need uuse constructor as brackets, 2 inputs 
  // TrigFTF_GNN_Geometry(const std::vector<TrigInDetSiLayer>&, const FASTRACK_CONNECTOR*);
//   const std::vector<TrigInDetSiLayer>& rosievector ; 
//   const FASTRACK_CONNECTOR* rosiefast ; //this might need a constructor 
//   TrigFTF_GNN_Geometry<external_spacepoint_t>& rosietest(rosievector, rosiefast); 

//   TrigFTF_GNN_Geometry<SpacePoint>& rosietest; 

  using seed_t = Seed<external_spacepoint_t>; 
  using internal_sp_t = InternalSpacePoint<external_spacepoint_t>;
  using tree_t = KDTree<NDims, internal_sp_t *, ActsScalar, std::array, 4>;

 //constructors 

  SeedFinderFTF(
      const Acts::SeedFinderFTFConfig<external_spacepoint_t> &config);

  ~SeedFinderFTF() = default;
  SeedFinderFTF() = default;
  SeedFinderFTF(const SeedFinderFTF<external_spacepoint_t> &) =
      delete;
  SeedFinderFTF<external_spacepoint_t> &operator=(
      const SeedFinderFTF<external_spacepoint_t> &) = default;


 //definition of function not the calling of it, define what input type it needs 
 //know eventually will have vector of simspacepoints, here need generic space point type 

  //void loadSpacePoints(const std::vector<external_spacepoint_t>&); 
    void loadSpacePoints(std::vector<const external_spacepoint_t*>); //trying to match type to input in examples 

 //create seeeds function 

  template <typename input_container_t, typename output_container_t,
            typename callable_t>
  void createSeeds(const Acts::SeedFinderOptions &options,
                   const input_container_t &spacePoints,
                   output_container_t &out_cont,
                   callable_t &&extract_coordinates) const;

  template <typename input_container_t, typename callable_t>
  std::vector<seed_t> createSeeds(const Acts::SeedFinderOptions &options,
                                  const input_container_t &spacePoints,
                                  callable_t &&extract_coordinates) const; 



 
 
 private:  

 //since update 
  enum Dim { DimPhi = 0, DimR = 1, DimZ = 2 };


 //declare valid tuple funcitons 
 //create tree function 
 //filter candidates function
 //proccess SP function

 //config object  
  Acts::SeedFinderFTFConfig<external_spacepoint_t> m_config;





}; 



} //end of acts namespace 

#include "Acts/Seeding/SeedFinderFTF.ipp"
