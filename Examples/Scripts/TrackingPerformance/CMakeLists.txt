add_executable(ActsAnalysisResidualsAndPulls ResidualsAndPulls.cpp)
target_link_libraries(
    ActsAnalysisResidualsAndPulls
    ROOT::Core
    ROOT::Hist
    ROOT::Tree
    ROOT::TreePlayer
    Boost::program_options
)

add_executable(ActsAnalysisTrackSummary TrackSummary.cpp)
target_link_libraries(
    ActsAnalysisTrackSummary
    ActsExamplesFramework
    ROOT::Core
    ROOT::Hist
    ROOT::Tree
    ROOT::TreePlayer
    Boost::program_options
    nlohmann_json::nlohmann_json
)

install(
    TARGETS ActsAnalysisResidualsAndPulls ActsAnalysisTrackSummary
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
