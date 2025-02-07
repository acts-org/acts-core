# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import math
import acts, acts.examples


from acts import (
    logging,
    Binning,
    Extent,
    ProtoBinning,
    LayerStructureBuilder,
    VolumeBoundsType,
    VolumeStructureBuilder,
    DetectorVolumeBuilder,
    DetectorBuilder,
    CylindricalContainerBuilder,
    Transform3,
)


def phiBinning(phiBins, extraBins=1):
    """helper method to create phi binning

    :param phiBins: number of phi bins
    :param extraBins: number of phi bins

    """
    return ProtoBinning(
        Binning.phi, Binning.closed, -math.pi, math.pi, phiBins, extraBins
    )


class CylindricalDetectorVolume:
    def __init__(
        self,
        name,
        extent,
        provider=None,
        binnings=None,
        supports=[],
        loglevel=logging.INFO,
    ):
        """Create a cylindrical, concentric volume builder

        :param name: name of the volume
        :param extent: extent of the volume
        :param provider: surface provider for the volume
        :param binnings: binning of surfces in this volume
        :param supports: support surface description
        :param loglevel: logging level
        """
        self._name = name
        self._extent = extent
        self._provider = provider
        self._binnings = binnings
        self._supports = supports
        self._loglevel = loglevel

    def builder(self):
        "Return the associated builder"

        # Get r, z, phi range
        rRange = self._extent.range(acts.Binning.r)
        zRange = self._extent.range(acts.Binning.z)

        # Set up the shape builder: external builder
        shapeConfig = VolumeStructureBuilder.Config()
        shapeConfig.boundsType = VolumeBoundsType.Cylinder
        shapeConfig.boundValues = [
            rRange[0],
            rRange[1],
            0.5 * (zRange[1] - zRange[0]),
            math.pi,
            0,
        ]
        shapeConfig.transform = Transform3([0, 0, 0.5 * (zRange[1] + zRange[0])])
        shapeConfig.auxiliary = "Shape[" + self._name + "]"

        # Set up the volume builder
        volConfig = acts.DetectorVolumeBuilder.Config()
        volConfig.name = self._name
        volConfig.auxiliary = "Volume[" + self._name + "]"
        volConfig.externalsBuilder = VolumeStructureBuilder(
            shapeConfig, shapeConfig.auxiliary, self._loglevel
        )
        if self._provider is not None:
            layerConfig = LayerStructureBuilder.Config()
            layerConfig.surfacesProvider = self._provider
            layerConfig.binnings = self._binnings
            layerConfig.supports = self._supports
            layerConfig.auxiliary = "Layer[" + self._name + "]"
            volConfig.internalsBuilder = LayerStructureBuilder(
                layerConfig, layerConfig.auxiliary, self._loglevel
            )
        # Return the builder
        return DetectorVolumeBuilder(volConfig, self._name, self._loglevel)

    def prependName(self, parent):
        """Allows to set the name from a parent"""
        self._name = parent + "_" + self._name

    def extent(self):
        """Return the extent of the volume in order to create gap volumes"""
        return self._extent


class CylindricalDetectorContainer:
    def __init__(
        self,
        name,
        extent,
        volumes,
        layers=None,
        binning=[],
        rootbuilder=None,
        geoidgenerator=None,
        reversegeoids=False,
        loglevel=logging.INFO,
    ):
        """Create a cylindrical container builder from  volumes or layer definitions

        :param name: name of the container
        :param extent: extent of the container
        :param volumes: list of volumes
        :param layers: list of layers [ [extent, provider, binnings, supports], ... ]
        :param binning: binning of surfces in this container
        :param rootbuilder: root volume finder builder
        :param geoidgenerator: geoid generator for setting geo ids
        :param reversegeoids: reverse the geo id order
        :param loglevel: logging level

        """
        self._name = name
        self._extent = extent
        self._layers = layers
        self._volumes = volumes
        self._binning = binning
        self._rootbuilder = rootbuilder
        self._geoidgenerator = geoidgenerator
        self._reversegeoids = reversegeoids
        self._loglevel = loglevel

    def builder(self):
        "Return the associated builder"
        orthogonal = Binning.r if self._binning == Binning.r else Binning.z

        builders = []
        # If the container is defined by volumes, just fill the builders
        if self._volumes is not None:
            builders = [volume.builder() for volume in self._volumes]
        # Otherwise, build the container from the layers
        else:
            bReference = self._extent.range(self._binning)[0]
            oRange = self._extent.range(orthogonal)
            # Builders to be constructed
            il = 0
            # Sub layer loop
            for layer in self._layers:
                bRange = layer.extent().range(self._binning)
                # Low gap volume insertion
                if bReference < bRange[0]:
                    gExtent = Extent(
                        [[self._binning, [bReference, bRange[0]]], [orthogonal, oRange]]
                    )
                    builders += [
                        CylindricalDetectorVolume(
                            self._name + "_gap_" + str(il), gExtent
                        ).builder()
                    ]
                # Layer volume insertion
                layer.prependName(self._name)

                builders += [layer.builder()]
                # Update reference and increment the counter
                bReference = bRange[1]
                il = il + 1

            # Last gap volume insertion
            if bReference < self._extent.range(self._binning)[1]:
                gExtent = Extent(
                    [
                        [
                            self._binning,
                            [bReference, self._extent.range(self._binning)[1]],
                        ],
                        [orthogonal, oRange],
                    ]
                )
                builders += [
                    CylindricalDetectorVolume(
                        self._name + "_gap_" + str(il), gExtent
                    ).builder()
                ]

        # The container builder
        containerConfig = CylindricalContainerBuilder.Config()
        containerConfig.builders = builders
        containerConfig.binning = [self._binning]
        containerConfig.rootVolumeFinderBuilder = self._rootbuilder
        containerConfig.geoIdGenerator = self._geoidgenerator
        containerConfig.geoIdReverseGen = self._reversegeoids
        containerConfig.auxiliary = "Container[" + self._name + "]"
        return CylindricalContainerBuilder(containerConfig, self._name, self._loglevel)
