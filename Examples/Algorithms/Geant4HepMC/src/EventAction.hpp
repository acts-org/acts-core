// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <map>
#include <memory>
#include <string>

#include <G4UserEventAction.hh>
#include <HepMC3/GenEvent.h>
#include <globals.hh>

namespace ActsExamples::Geant4::HepMC3 {

/// The EventAction class is the realization of the Geant4 class
/// G4UserEventAction and is writing out the collected RecordedMaterialTrack
/// entities needed for material mapping once per event.
///
class EventAction final : public G4UserEventAction {
 public:
  /// Static access method
  static EventAction* instance();

  /// Construct the action and ensure singleton usage.
  explicit EventAction(std::vector<std::string> processFilter);
  ~EventAction() override;

  /// Interface method for begin of the event
  /// @param event is the G4Event to be processed
  /// @note resets the event and step action
  void BeginOfEventAction(const G4Event* event) override;

  /// Interface method for end of event
  /// @param event is the G4Event to be processed
  void EndOfEventAction(const G4Event* event) override;

  /// Clear the recorded data.
  void clear();

  /// Getter of the created HepMC3 event
  ::HepMC3::GenEvent& event();

 private:
  /// Instance of the EventAction
  static EventAction* s_instance;
  /// The current HepMC3 event
  ::HepMC3::GenEvent m_event;
  /// List of processes that can be combined to a single vertex
  std::vector<std::string> m_processFilter;
};
}  // namespace ActsExamples::Geant4::HepMC3
