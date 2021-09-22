add_executable(ActsAnalysisResidualsAndPulls ResidualsAndPulls.cpp)
target_link_libraries(ActsAnalysisResidualsAndPulls ROOT::Core ROOT::Hist ROOT::Tree ROOT::TreePlayer Boost::program_options)

install(
  TARGETS
  ActsAnalysisResidualsAndPulls
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


add_executable(ActsAnalysisPerigeeParameters PerigeeParameters.cpp)
target_link_libraries(ActsAnalysisPerigeeParameters ActsExamplesFramework ROOT::Core ROOT::Hist ROOT::Tree ROOT::TreePlayer Boost::program_options)
  
install(
  TARGETS
  ActsAnalysisPerigeeParameters
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})  