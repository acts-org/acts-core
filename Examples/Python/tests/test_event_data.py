# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import acts


def test_particle_hypothesis():
    muon = acts.ParticleHypothesis.muon
    pion = acts.ParticleHypothesis.pion
    electron = acts.ParticleHypothesis.electron
    proton = acts.ParticleHypothesis.proton
    kaon = acts.ParticleHypothesis.kaon
    geantino = acts.ParticleHypothesis.geantino
    chargedGeantino = acts.ParticleHypothesis.chargedGeantino

    # create new particle hypothesis

    # check pdg
    assert muon.absolutePdg() == acts.PdgParticle.eMuon
    assert pion.absolutePdg() == acts.PdgParticle.ePionPlus
    assert electron.absolutePdg() == acts.PdgParticle.eElectron
    assert kaon.absolutePdg() == acts.PdgParticle.eKaonPlus
    assert proton.absolutePdg() == acts.PdgParticle.eProton
    assert geantino.absolutePdg() == acts.PdgParticle.eInvalid
    assert chargedGeantino.absolutePdg() == acts.PdgParticle.eInvalid

    # check mass
    assert electron.mass() != 0
    assert electron.mass() < muon.mass()
    assert muon.mass() < pion.mass()
    assert pion.mass() < kaon.mass()
    assert kaon.mass() < proton.mass()
    assert geantino.mass() == 0
    assert chargedGeantino.mass() == 0

    # check charge
    assert electron.absoluteCharge() == 1
    assert muon.absoluteCharge() == 1
    assert pion.absoluteCharge() == 1
    assert proton.absoluteCharge() == 1
    assert geantino.absoluteCharge() == 0
    assert chargedGeantino.absoluteCharge() == 1
    assert kaon.absoluteCharge() == 1

    # printing should show something sensible
    assert str(muon) == "ParticleHypothesis{absPdg=mu, mass=0.105658, absCharge=1}"
    assert str(pion) == "ParticleHypothesis{absPdg=pi, mass=0.13957, absCharge=1}"
    assert (
        str(electron) == "ParticleHypothesis{absPdg=e, mass=0.000510999, absCharge=1}"
    )
    assert str(kaon) == "ParticleHypothesis{absPdg=K, mass=0.493677, absCharge=1}"
    assert str(proton) == "ParticleHypothesis{absPdg=p, mass=0.938272, absCharge=1}"
    assert str(geantino) == "ParticleHypothesis{absPdg=0, mass=0, absCharge=0}"
    assert str(chargedGeantino) == "ParticleHypothesis{absPdg=0, mass=0, absCharge=1}"
