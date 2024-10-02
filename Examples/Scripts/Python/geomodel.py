import acts
import argparse
from acts import (
    logging,
    GeometryContext,
    CylindricalContainerBuilder,
    DetectorBuilder,
    GeometryIdGenerator,
)
from acts import geomodel as gm
from acts import examples


def main():
    p = argparse.ArgumentParser()

    p.add_argument("-i", "--input", type=str, default="", help="Input SQL file")

    p.add_argument(
        "-q",
        "--queries",
        type=str,
        nargs="+",
        default="GeoModelXML",
        help="List of Queries for Published full phys volumes",
    )

    p.add_argument(
        "-n",
        "--name-list",
        type=str,
        nargs="+",
        default=[],
        help="List of Name List for the Surface Factory",
    )

    p.add_argument(
        "-ml",
        "--material-list",
        type=str,
        nargs="+",
        default=[],
        help="List of Material List for the Surface Factory",
    )

    p.add_argument(
        "--table-name",
        type=str,
        default="ActsBlueprint",
        help="Name of the blueprint table",
    )

    p.add_argument(
        "-t",
        "--top-node",
        type=str,
        default="",
        help="Name of the top node in the blueprint tree",
    )

    p.add_argument(
        "-b",
        "--top-node-bounds",
        type=str,
        default="",
        help="Table entry string overriding the top node bounds",
    )

    p.add_argument(
        "-m", "--map", type=str, default="", help="Input file for the material map"
    )

    p.add_argument(
        "--convert-subvols",
        help="Convert the children of the top level full phys vol",
        action="store_true",
        default=False,
    )

    p.add_argument(
        "--enable-blueprint",
        help="Enable the usage of the blueprint",
        action="store_true",
        default=False,
    )

    args = p.parse_args()

    gContext = acts.GeometryContext()
    logLevel = logging.INFO

    materialDecorator = None
    if args.map != "":
        print("Loading a material decorator from file:", args.map)
        materialDecorator = acts.IMaterialDecorator.fromFile(args.map)

    # Read the geometry model from the database
    gmTree = acts.geomodel.readFromDb(args.input)

    gmFactoryConfig = gm.GeoModelDetectorObjectFactory.Config()
    gmFactoryConfig.materialList = args.material_list
    gmFactoryConfig.nameList = args.name_list
    gmFactoryConfig.convertSubVolumes = args.convert_subvols
    gmFactory = gm.GeoModelDetectorObjectFactory(gmFactoryConfig, logLevel)
    # The options
    gmFactoryOptions = gm.GeoModelDetectorObjectFactory.Options()
    gmFactoryOptions.queries = args.queries
    # The Cache & construct call
    gmFactoryCache = gm.GeoModelDetectorObjectFactory.Cache()
    gmFactory.construct(gmFactoryCache, gContext, gmTree, gmFactoryOptions)

    # All surfaces from GeoModel
    gmSurfaces = [ss[1] for ss in gmFactoryCache.sensitiveSurfaces]

    # Construct the building hierarchy
    gmBlueprintConfig = gm.GeoModelBlueprintCreater.Config()
    gmBlueprintConfig.detectorSurfaces = gmSurfaces
    gmBlueprintConfig.kdtBinning = [acts.Binning.z, acts.Binning.r]

    gmBlueprintOptions = gm.GeoModelBlueprintCreater.Options()
    gmBlueprintOptions.table = args.table_name
    gmBlueprintOptions.topEntry = args.top_node
    if len(args.top_node_bounds) > 0:
        gmBlueprintOptions.topBoundsOverride = args.top_node_bounds

    gmBlueprintCreater = gm.GeoModelBlueprintCreater(gmBlueprintConfig, logLevel)
    gmBlueprint = gmBlueprintCreater.create(gContext, gmTree, gmBlueprintOptions)

    gmCylindricalBuilder = gmBlueprint.convertToBuilder(logLevel)

    # Top level geo id generator
    gmGeoIdConfig = GeometryIdGenerator.Config()
    gmGeoIdGenerator = GeometryIdGenerator(
        gmGeoIdConfig, "GeoModelGeoIdGenerator", logLevel
    )

    # Create the detector builder
    gmDetectorConfig = DetectorBuilder.Config()
    gmDetectorConfig.name = args.top_node + "_DetectorBuilder"
    gmDetectorConfig.builder = gmCylindricalBuilder
    gmDetectorConfig.geoIdGenerator = gmGeoIdGenerator
    gmDetectorConfig.materialDecorator = materialDecorator
    gmDetectorConfig.auxiliary = (
        "GeoModel based Acts::Detector from '" + args.input + "'"
    )

    gmDetectorBuilder = DetectorBuilder(gmDetectorConfig, args.top_node, logLevel)
    detector = gmDetectorBuilder.construct(gContext)

    return


if "__main__" == __name__:
    main()
