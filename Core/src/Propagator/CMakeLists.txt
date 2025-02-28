target_sources(
    ActsCore
    PRIVATE
        EigenStepperError.cpp
        MultiStepperError.cpp
        NavigatorError.cpp
        SympyStepper.cpp
        PropagatorError.cpp
        StraightLineStepper.cpp
        detail/PointwiseMaterialInteraction.cpp
        detail/CovarianceEngine.cpp
        detail/JacobianEngine.cpp
        detail/SympyCovarianceEngine.cpp
        detail/SympyJacobianEngine.cpp
)

include(ActsCodegen)

acts_code_generation(
      TARGET SympyCovarianceEngine
      PYTHON detail/generate_sympy_cov.py
      WITH_REQUIREMENTS ${CMAKE_SOURCE_DIR}/codegen/requirements.txt
      WITH ${CMAKE_SOURCE_DIR}/codegen
      ISOLATED
      OUTPUT codegen/sympy_cov_math.hpp
)

acts_code_generation(
      TARGET SympyStepperMath
      PYTHON generate_sympy_stepper.py
      WITH_REQUIREMENTS ${CMAKE_SOURCE_DIR}/codegen/requirements.txt
      WITH ${CMAKE_SOURCE_DIR}/codegen
      ISOLATED
      OUTPUT codegen/sympy_stepper_math.hpp
)

acts_code_generation(
      TARGET SympyJacobianEngine
      PYTHON detail/generate_sympy_jac.py
      WITH_REQUIREMENTS ${CMAKE_SOURCE_DIR}/codegen/requirements.txt
      WITH ${CMAKE_SOURCE_DIR}/codegen
      ISOLATED
      OUTPUT codegen/sympy_jac_math.hpp
)

target_link_libraries(
    ActsCore
    PRIVATE SympyCovarianceEngine SympyStepperMath SympyJacobianEngine
)

# message(FATAL_ERROR "stop")
