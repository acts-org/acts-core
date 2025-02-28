target_sources(ActsCore PRIVATE Common.cpp Direction.cpp ParticleData.cpp)

include(ActsCodegen)

acts_code_generation(
      TARGET ParticleDataTable
      PYTHON ${CMAKE_SOURCE_DIR}/Fatras/scripts/generate_particle_data_table.py
      OUTPUT ParticleDataTable.hpp
)

target_link_libraries(ActsCore PRIVATE ParticleDataTable)
