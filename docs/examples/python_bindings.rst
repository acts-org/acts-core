.. _python-bindings:

Python bindings for the Examples
================================

The examples part of ACTS ships with python bindings using the ``pybind11``
library. Building these bindings can be enabled via
``-DACTS_BUILD_EXAMPLES_PYTHON_BINDINGS=ON``, and requires a python installation
including the development files to be installed. You can then build the special
target `ActsPythonBindings` to build everything that can be accessed in python.

The build will create a setup script in ``$BUILD_DIR/python/setup.sh`` which
modifies ``$PYTHONPATH`` so that you can import the ``acts`` module in python.

Here is a minimal example of a python script using the example bindings, which
sets up the particle propagation and runs a few events.

.. code-block:: python

   import os

   import acts
   import acts.examples

   detector, trackingGeometry, contextDecorators  = acts.examples.GenericDetector.create()
   s = acts.examples.Sequencer(events=10)

   rnd = acts.examples.RandomNumbers(seed=42)

   nav = acts.Navigator(trackingGeometry=trackingGeometry)

   field = acts.ConstantBField(acts.Vector3(0, 0, 2 * acts.UnitConstants.T))
   stepper = acts.EigenStepper(field)

   prop = acts.examples.ConcretePropagator(acts.Propagator(stepper, nav))

   alg = acts.examples.PropagationAlgorithm(
       propagatorImpl=prop,
       level=acts.logging.INFO,
       randomNumberSvc=rnd,
       ntests=1000,
       sterileLogger=False,
       outputSummaryCollection="propagation_summary",
   )

   s.addAlgorithm(alg)

   outputDir = "."
   objDir = outputDir + "/obj"
   if not os.path.exists(objDir):
      os.mkdir(objDir)

   s.addWriter(
       acts.examples.ObjPropagationStepsWriter(
           level=acts.logging.INFO,
           collection="propagation_summary",
           outputDir=objDir,
       )
   )

   s.addWriter(
       acts.examples.RootPropagationStepsWriter(
           level=acts.logging.INFO,
           collection="propagation_summary",
           filePath=outputDir + "/propagation_steps.root",
       )
   )

   s.run()

Python based example scripts
----------------------------

The repository contains a set of example scripts that can be used to execute various workflows.
They can be found in ``$REPO_ROOT/Examples/Scripts/Python``. Make sure you have run

.. code-block:: console

   source $BUILD_DIR/python/setup.sh

to make sure python can find the ``acts`` module.

Python based unit tests
-----------------------

A number of unit tests based on the ``pytest`` library are shipped with the
repository. They are located under ``$REPO_ROOT/Examples/Python/tests``, and
intend to cover the public API of the python bindings. A set of tests also
executed the standalone example scripts.

To run these python based tests, ``pytest`` and a few other dependencies need
to be installed. They can be installed via ``pip install -r
Examples/Python/tests/requirements.txt`` from the repository root.  You can
then simply run ``pytest`` from the repository root.

.. tip::
   :name: python-virtualenv

   It is **strongly recommended** to use a `virtual environment`_ for
   this purpose! For example, run

   .. code-block:: console

      $ python -m venv venv
      $ source venv/bin/activate

   to create a local virtual environment, and then run the `pip` command above.


.. _virtual environment: https://realpython.com/python-virtual-environments-a-primer/

.. _root_hash_checks:

ROOT file hash regression checks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In a number of cases, the python based test suite will run hash based regression tests against ROOT files that are
written by the test workloads. These tests use a custom hash algorithm written in python, which hashes each individual
entry of each ``TTree`` found in a file. These entry hashes are then sorted, concatenated and hashed again for the final output.
This procedure ensures that if the ROOT file content changes, the hash changes, while also giving the same hash when the events
stored in the file are reordered.

The tests are implemented by looking up a reference hash from a central data file ``$REPO_ROOT/Examples/Python/tests/root_file_hashes.txt``
that looks like

.. code-block:: none

   test_ckf_tracks_example_full_seeding__performance_seeding_trees.root: 938bcc9b9425b12c620f5d0efa2c592817dfe92a18c309e97aa9d87412918620
   test_ckf_tracks_example_full_seeding__trackstates_ckf.root: 2faceafd4a521ff4030557301723e29c3d870edad052965eb644b824b57e2146
   test_ckf_tracks_example_truth_estimate__performance_seeding_trees.root: 5c0cf9e84af64e6814ab1ddf4cbaf4be6008ad8b2371b5b0241085b19d0fc52c
   test_ckf_tracks_example_truth_estimate__performance_seeding_trees.root: 5c0cf9e84af64e6814ab1ddf4cbaf4be6008ad8b2371b5b0241085b19d0fc52c
   test_ckf_tracks_example_truth_estimate__trackstates_ckf.root: ac4485c09a68fca3d056cb8d9adb81695e68d822629e48c71fd2b6d2bbd31f88
   # ...

where the left side before the ``:`` indicates the test in which the check is performed and the name of the ROOT file
that is checked. The right side is the reference hash.

.. note:: The file from which reference hashes are loaded can be changed by setting the environment variable ``ROOT_HASH_FILE``
          to the desired file.

These checks have two purposes:

1. Detect regressions in the algorithms: if an algorithm produces different output, the test will catch it. This also means that
   if algorithmic changes are made that intentionally change the output, the reference hashes also have to be updated.

   .. warning:: Please make sure to check the contents of a changed file are correct/reasonable before updating the reference hash!

2. Detect potential reproducibility issues. Tests that run with multiple threads should produce the same output every run,
   event ordering aside. If a test workload has a thread-reproducibility issue, the output hash should also change.

Running the hash checks locally and how to update the reference hashes
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

By default, the hash checks are not executed when the ``pytest`` command is run. To enable them, you need to set the environment
variable ``ROOT_HASH_CHECKS`` needs to be set to ``ON``, for example like:

.. code-block:: console

   ROOT_HASH_CHECKS=ON pytest

If any hash mismatches are observed, the corresponding tests will fail, and ``pytest`` will print a summary at the end that looks like

.. code-block:: console

   ------------------------------------------- RootHashAssertionErrors -----------------------------------------------------
   The ROOT files produced by tests have changed since the last recorded reference.
   This can be be expected if e.g. the underlying algorithm changed, or it can be a test failure symptom.
   Please manually check the output files listed below and make sure that their content is correct.
   If it is, you can update the test reference file Examples/Python/tests/root_file_hashes.txt with the new hashes below.

   test_seeding__estimatedparams.root: 8bbc97cb3d4777c61dd0b483a1c8268fc8411ad182c35bc731e5ed222450deca
   test_material_recording__geant4_material_tracks.root: 019ce62ce378efa5c02a94768039686ed3cdfbd60c115c1f0cab2cbc53def57b
   test_material_mapping__material-maps_tracks.root: c03215e8b53733a3a7d7a0a5f9aec5bf2df20e8e40cc0492a8fa22400491d216
   test_material_mapping__propagation-material.root: a15a5c1e92fc3b848efb232eea1d40c422ee3a1d9ef1f7140294415621a04ce5
   test_ckf_tracks_example_full_seeding__tracksummary_ckf.root: 9e4d14169f20961be38d0305853a7cf7eeea4a647f0c94a48aada22c3c2c7a51
   test_ckf_tracks_example_truth_estimate__tracksummary_ckf.root: 3d56b26788163852e2c1f7288920f60a505bd14deeabb6f9189b680fcd90bfc5
   test_ckf_tracks_example_truth_smeared__tracksummary_ckf.root: ca2ce4069d2a2388c3d3c826dec8bea9f9d1e622239a20f8b985784d6c546c6e
   =========================================== short test summary info =====================================================
   FAILED Examples/Python/tests/test_examples.py::test_seeding
   FAILED Examples/Python/tests/test_examples.py::test_material_recording
   FAILED Examples/Python/tests/test_examples.py::test_material_mapping
   FAILED Examples/Python/tests/test_examples.py::test_ckf_tracks_example_full_seeding
   FAILED Examples/Python/tests/test_examples.py::test_ckf_tracks_example_truth_estimate
   FAILED Examples/Python/tests/test_examples.py::test_ckf_tracks_example_truth_smeared
   ================================== 6 failed, 183 passed in 199.82s (0:03:19) ============================================

Here, we see that 7 hash checks have failed. The error output conveniently has the same format as the reference hashes found in ``root_file_hashes.txt``.
To update the reference hashes, simply replace the corresponding entries in ``root_file_hashes.txt`` with the output from the ``pytest`` run.

.. note:: The CI runs the ROOT hash checks. However, we have observed the hashes to change between different machines.
          This is believed to be due to differences in math libraries producing slightly different outputs. As a consequence,
          locally obtained file hashes might cause CI failures, as the CI hashes are different.

          For local testing, it is therefore advisable to use ``ROOT_HASH_FILE`` to use a different file for the reference hashes
          and populated it with known-good reference hashes from the ``main`` branch, before testing your developments.

          To make the CI succeed if it obtains different hashes than you get locally: make sure that the output is correct, and then
          update the central ``root_file_hashes.txt`` with the hashes reported in the failed CI job.
