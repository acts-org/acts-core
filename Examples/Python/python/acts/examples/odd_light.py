import acts
import argparse
import acts.examples
import acts.examples.detector as detector
from acts.examples import geant4 as acts_g4

from acts import (
    Binning,
    DetectorBuilder,
    GeometryIdGenerator,
    Extent,
    GeometryContext,
    KdtSurfaces2D,
    KdtSurfacesProvider2D,
    IndexedRootVolumeFinderBuilder,
    ProtoAxis,
)


def necBarrelPec(
    name,
    rRange,
    zDivisions,
    sensitivesKdt,
    necPositionsZ,
    necHalfThickness,
    necBinning,
    barrelPositionsR,
    barrelHalfThickness,
    barrelBinnings,
    containerIds=[],
    llevel=acts.logging.INFO,
):
    # Negative Endcap
    necEndcapExtent = Extent([[Binning.r, rRange], [Binning.z, zDivisions[0:2]]])

    necLayers = []
    for zPos in necPositionsZ:
        ilay = len(necLayers)
        lExtent = Extent(
            [
                [Binning.r, rRange],
                [Binning.z, [zPos - necHalfThickness, zPos + necHalfThickness]],
            ]
        )
        provider = KdtSurfacesProvider2D(sensitivesKdt, lExtent)
        necLayers += [
            detector.CylindricalDetectorVolume(
                "lay_" + str(ilay), lExtent, provider, necBinning, [], llevel
            )
        ]

    necGig = None
    if len(containerIds) > 0:
        necGigConf = GeometryIdGenerator.Config()
        necGigConf.containerMode = True
        necGigConf.containerId = containerIds[0]
        necGig = GeometryIdGenerator(necGigConf, name + "_nec_gig", llevel)

    nec = detector.CylindricalDetectorContainer(
        name=name + "_nec",
        extent=necEndcapExtent,
        volumes=None,
        layers=necLayers,
        binning=Binning.z,
        rootbuilder=None,
        geoidgenerator=necGig,
        reversegeoids=True,
        loglevel=llevel,
    )

    # Barrel
    barrelExtent = Extent([[Binning.r, rRange], [Binning.z, zDivisions[1:3]]])

    # Create the barrel layers
    barrelLayers = []
    for layerR in barrelPositionsR:
        ilay = len(barrelLayers)
        lExtent = Extent(
            [
                [
                    Binning.r,
                    [layerR - barrelHalfThickness, layerR + barrelHalfThickness],
                ],
                [Binning.z, zDivisions[1:3]],
            ]
        )
        provider = KdtSurfacesProvider2D(sensitivesKdt, lExtent)
        binning = barrelBinnings[ilay]
        barrelLayers += [
            detector.CylindricalDetectorVolume(
                "lay_" + str(ilay), lExtent, provider, binning, [], llevel
            )
        ]

    barrelGig = None
    if len(containerIds) > 1:
        barrelGiConfg = GeometryIdGenerator.Config()
        barrelGiConfg.containerMode = True
        barrelGiConfg.containerId = containerIds[1]
        barrelGig = GeometryIdGenerator(barrelGiConfg, name + "_barrel_gig", llevel)

    barrel = detector.CylindricalDetectorContainer(
        name=name + "_barrel",
        extent=barrelExtent,
        volumes=None,
        layers=barrelLayers,
        binning=Binning.r,
        rootbuilder=None,
        geoidgenerator=barrelGig,
        reversegeoids=False,
        loglevel=llevel,
    )

    # Positive Endcap
    pecEndcapExtent = Extent([[Binning.r, rRange], [Binning.z, zDivisions[2:4]]])
    necPositionsZ.reverse()

    pecLayers = []
    for zPos in necPositionsZ:
        ilay = len(pecLayers)
        lExtent = Extent(
            [
                [Binning.r, rRange],
                [
                    Binning.z,
                    [-1 * zPos - necHalfThickness, -1 * zPos + necHalfThickness],
                ],
            ]
        )
        provider = KdtSurfacesProvider2D(sensitivesKdt, lExtent)
        pecLayers += [
            detector.CylindricalDetectorVolume(
                "lay_" + str(ilay), lExtent, provider, necBinning, [], llevel
            )
        ]

    pecGig = None
    if len(containerIds) > 2:
        pecGigConf = GeometryIdGenerator.Config()
        pecGigConf.containerMode = True
        pecGigConf.containerId = containerIds[2]
        pecGig = GeometryIdGenerator(pecGigConf, name + "_pec_gig", llevel)

    pec = detector.CylindricalDetectorContainer(
        name=name + "_pec",
        extent=pecEndcapExtent,
        volumes=None,
        layers=pecLayers,
        binning=Binning.z,
        rootbuilder=None,
        geoidgenerator=pecGig,
        reversegeoids=False,
        loglevel=llevel,
    )

    # Build the nec - barrel - pec container
    return detector.CylindricalDetectorContainer(
        name=name,
        extent=None,
        volumes=[nec, barrel, pec],
        layers=None,
        binning=Binning.z,
        rootbuilder=None,
        geoidgenerator=None,
        loglevel=llevel,
    )


def get_detector(geoContext, ssurfaces, psurfaces, llevel=acts.logging.DEBUG):
    # Build the geometry context & a kdtree for the surfaces
    sensitivesKdt = KdtSurfaces2D(geoContext, ssurfaces, [Binning.z, Binning.r])
    passivesKdt = KdtSurfaces2D(geoContext, psurfaces, [Binning.z, Binning.r])

    # Detector section ############################################################
    detRrange = [0, 1100]
    detZrange = [-3100, 3100]

    # Beam pipe section ###########################################################
    bpRrange = [detRrange[0], 25.0]
    bpExtent = Extent([[Binning.r, bpRrange], [Binning.z, detZrange]])
    bpProvider = KdtSurfacesProvider2D(passivesKdt, bpExtent)
    bpBinning = [ProtoAxis(Binning.r, Binning.bound, 0, 25, 1, 0)]
    bp = detector.CylindricalDetectorVolume(
        "ODD_beampipe", bpExtent, bpProvider, bpBinning, [], llevel
    )

    # Pixel section ###############################################################
    pixRrange = [25, 190]
    pixZdivisions = [detZrange[0], -590, 590, detZrange[1]]
    pixNecPositionsZ = [-1520, -1320, -1120, -980, -840, -720, -620]
    pixNecHalfThickness = 5
    pixNecBinning = [
        ProtoAxis(Binning.r, Binning.bound, 40, 175, 2, 1),
        detector.phiBinning(56, 1),
    ]
    pixBarrelPositionsR = [34, 70, 116, 172]
    pixBarrelHalfThickness = 5
    pixBarrelZbinning = ProtoAxis(Binning.z, Binning.bound, -500, 500, 14, 1)
    pixBarrelPhiBinnings = [[16, 1], [32, 1], [52, 1], [78, 1]]
    pixBarrelBinning = [
        [pixBarrelZbinning, detector.phiBinning(phiBinning[0], phiBinning[1])]
        for phiBinning in pixBarrelPhiBinnings
    ]

    pix = necBarrelPec(
        "ODD_pixel",
        pixRrange,
        pixZdivisions,
        sensitivesKdt,
        pixNecPositionsZ,
        pixNecHalfThickness,
        pixNecBinning,
        pixBarrelPositionsR,
        pixBarrelHalfThickness,
        pixBarrelBinning,
        [16, 17, 18],
        llevel,
    )

    # PST section #################################################################
    pstRrange = [pixRrange[1], 220]
    pstExtent = Extent([[Binning.r, pstRrange], [Binning.z, detZrange]])
    pstProvider = KdtSurfacesProvider2D(passivesKdt, pstExtent)
    pstBinning = [ProtoAxis(Binning.r, Binning.bound, pixRrange[1], 220, 1, 0)]
    pst = detector.CylindricalDetectorVolume(
        "ODD_pst", pstExtent, pstProvider, pstBinning, [], llevel
    )

    # Short strip section #########################################################
    sstripRrange = [pstRrange[1], 720]
    sstripZdivisions = [detZrange[0], -1250, 1250, detZrange[1]]

    sstripNecPositionsZ = [-2950, -2550, -2200, -1850, -1550, -1300]
    sstripNecHalfThickness = 15
    sstripNecBinning = [
        ProtoAxis(Binning.r, Binning.bound, 230, 710, 3, 1),
        detector.phiBinning(40, 1),
    ]
    sstripBarrelPositionsR = [260, 360, 500, 660]
    sstripBarrelHalfThickness = 25
    sstripBarrelZbinning = ProtoAxis(Binning.z, Binning.bound, -1100, 1100, 21, 1)
    sstripBarrelPhiBinnings = [[40, 1], [56, 1], [78, 1], [102, 1]]
    sstripBarrelBinning = [
        [sstripBarrelZbinning, detector.phiBinning(phiBinning[0], phiBinning[1])]
        for phiBinning in sstripBarrelPhiBinnings
    ]

    sstrip = necBarrelPec(
        "ODD_sstrip",
        sstripRrange,
        sstripZdivisions,
        sensitivesKdt,
        sstripNecPositionsZ,
        sstripNecHalfThickness,
        sstripNecBinning,
        sstripBarrelPositionsR,
        sstripBarrelHalfThickness,
        sstripBarrelBinning,
        [23, 24, 25],
        llevel,
    )

    # Long strip section ##########################################################

    lstripRrange = [sstripRrange[1], detRrange[1]]
    lstripZdivisions = [detZrange[0], -1250, 1250, detZrange[1]]

    lstripNecPositionsZ = [-3000, -2600, -2250, -1900, -1600, -1300]
    lstripNecHalfThickness = 15
    lstripNecBinning = [
        ProtoAxis(Binning.r, Binning.bound, 720, 1020, 2, 1),
        detector.phiBinning(40, 1),
    ]
    lstripBarrelPositionsR = [830, 1030]
    lstripBarrelHalfThickness = 35
    lstripBarrelZbinning = ProtoAxis(Binning.z, Binning.bound, -1100, 1100, 21, 1)
    lstripBarrelPhiBinnings = [[40, 1], [56, 1]]
    lstripBarrelBinning = [
        [lstripBarrelZbinning, detector.phiBinning(phiBinning[0], phiBinning[1])]
        for phiBinning in lstripBarrelPhiBinnings
    ]

    lstrip = necBarrelPec(
        "ODD_lstrip",
        lstripRrange,
        lstripZdivisions,
        sensitivesKdt,
        lstripNecPositionsZ,
        lstripNecHalfThickness,
        lstripNecBinning,
        lstripBarrelPositionsR,
        lstripBarrelHalfThickness,
        lstripBarrelBinning,
        [28, 29, 30],
        llevel,
    )

    # Root builder from the detector container
    rootBuilder = IndexedRootVolumeFinderBuilder([Binning.z, Binning.r])

    det = detector.CylindricalDetectorContainer(
        name="ODD",
        extent=None,
        volumes=[bp, pix, pst, sstrip, lstrip],
        layers=None,
        binning=Binning.r,
        rootbuilder=rootBuilder,
        geoidgenerator=None,
        loglevel=llevel,
    )

    # All objects that do not have a geometry id will be assigned one
    gigConfig = GeometryIdGenerator.Config()
    gigConfig.overrideExistingIds = False
    gigConfig.resetSubCounters = True
    gig = GeometryIdGenerator(gigConfig, "GeometryIdGenerator", llevel)

    # Full detector configuration
    detConfig = DetectorBuilder.Config()
    detConfig.name = "ODD"
    detConfig.builder = det.builder()
    detConfig.geoIdGenerator = gig
    detConfig.auxiliary = "Detector[" + detConfig.name + "]"

    detBuilder = DetectorBuilder(detConfig, detConfig.auxiliary, llevel)
    return detBuilder.construct(geoContext)


def main():
    # Parse the command line arguments
    p = argparse.ArgumentParser()
    p.add_argument(
        "-i",
        "--input",
        type=str,
        default="odd-light.gdml",
        help="GDML input file.",
    )
    p.add_argument(
        "-s",
        "--sensitives",
        type=str,
        default="phys_vol",
        help="Match string for sensitive surfaces",
    )
    p.add_argument(
        "-p",
        "--passives",
        type=str,
        default="pass_vol",
        help="Match string for passive surfaces",
    )
    args = p.parse_args()
    geoContext = GeometryContext()

    # Convert the detector surfaces to GDML
    [_, ssurfaces, psurfaces] = acts_g4.convertSurfaces(
        args.input, [args.sensitives], [args.passives]
    )
    get_detector(geoContext, ssurfaces, psurfaces, acts.logging.DEBUG)


if "__main__" == __name__:
    main()
