#!/usr/bin/env python3

from pathlib import Path
import os
import sys
import subprocess


def file_can_be_removed(searchstring, scope):
    cmd = "grep -IR '" + searchstring + "' " + " ".join(scope)

    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    output, _ = p.communicate()
    return output == b""


def count_files(path=".", exclude_dirs=(), exclude_files=()):
    count = 0
    for root, dirs, files in os.walk(path):
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        files[:] = [f for f in files if f not in exclude_files]
        count += len(files)

    return count


def main():
    print("\033[32mINFO\033[0m Start check_unused_files.py ...")
    exclude_dirs = (
        "Scripts",
        "thirdparty",
        "CI",
        "git",
        "cmake",
        ".git",
        ".github",
        ".idea",
    )
    exclude_files = (
        "acts_logo_colored.svg",
        ".gitignore",
        "README.md",
        "CMakeLists.txt",
        # Filename not completed in source
        "vertexing_event_mu20_beamspot.csv",
        "vertexing_event_mu20_tracks.csv",
        "vertexing_event_mu20_vertices_AMVF.csv",
        "event000000001-MuonDriftCircle.csv",
        "event000000001-MuonSimHit.csv",
        # TODO Move the following files to a better place?
        "Magfield.ipynb",
        "SolenoidField.ipynb",
        # TODO Add README next to the following files?
        "default-input-config-generic.json",
        "geoSelection-openDataDetector.json",
        "alignment-geo-contextualDetector.json",
        # TODO Mention these files somewhere?
        "generate_particle_data_table.py",
        "lazy_autodoc.py",
        "codegen/src/codegen/sympy_common.py",
    )

    suffix_header = (
        ".hpp",
        ".cuh",
    )
    suffix_source = (
        ".ipp",
        ".cpp",
        ".cu",
    )
    suffix_image = (
        ".png",
        ".svg",
        ".jpg",
        ".gif",
    )
    suffix_python = (".py",)
    suffix_doc = (
        ".md",
        ".rst",
    )
    suffix_other = (
        "",
        ".csv",
        ".css",
        ".gdml",
        ".hepmc3",
        ".in",
        ".ipynb",
        ".json",
        ".j2",
        ".onnx",
        ".root",
        ".toml",
        ".txt",
        ".yml",
    )
    suffix_allowed = (
        suffix_header
        + suffix_source
        + suffix_image
        + suffix_python
        + suffix_doc
        + suffix_other
    )

    exit = 0

    dirs_base = next(os.walk("."))[1]
    dirs_base.append(".")
    dirs_base[:] = [d for d in dirs_base if d not in exclude_dirs]
    dirs_base_docs = ("docs",)
    dirs_base_code = [d for d in dirs_base if d not in dirs_base_docs]

    # Collectors
    wrong_extension = ()
    unused_files = ()

    # walk over all files
    for root, dirs, files in os.walk("."):
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        files[:] = [f for f in files if f not in exclude_files]

        # Skip base-directory
        if str(Path(root)) == ".":
            continue

        # Print progress
        if root[2:] in dirs_base:
            processed_files = 0
            current_base_dir = root
            number_files = count_files(root, exclude_dirs, exclude_files)
            # print empty to start a new line
            print("")

        # Skip "white-paper-figures"
        # TODO Find a more elegant way
        if str(root).find("white_papers/figures") != -1:
            processed_files += count_files(root, exclude_dirs, exclude_files)
            continue

        # Skip "DD4hep-tests" since their cmake looks a bit different
        # TODO Find a more elegant way
        if str(root).find("Tests/UnitTests/Plugins/DD4hep") != -1:
            processed_files += count_files(root, exclude_dirs, exclude_files)
            continue

        root = Path(root)
        for filename in files:
            processed_files += 1
            # get the full path of the file
            filepath = root / filename

            # Check for wrong extensions
            if filepath.suffix not in suffix_allowed:
                wrong_extension += (str(filepath),)

            # Check header files and remove
            elif filepath.suffix in suffix_header + suffix_source:
                if file_can_be_removed(filepath.stem, dirs_base_code):
                    unused_files += (str(filepath),)
                    remove_cmd = "rm " + str(filepath)
                    os.system(remove_cmd)

            elif filepath.suffix in suffix_python:
                # Skip the python tests folder
                if str(root).find("Examples/Python") != -1:
                    continue

                if not file_can_be_removed("import .*" + filepath.stem, dirs_base):
                    continue

                if not file_can_be_removed(
                    "from " + filepath.stem + " import", dirs_base
                ):
                    continue

                if file_can_be_removed(filename, dirs_base):
                    unused_files += (str(filepath),)
                    remove_cmd = "rm " + str(filepath)
                    os.system(remove_cmd)

            # Check documentation files (weak tests)
            # TODO find more reliable test for this
            elif filepath.suffix in suffix_doc:
                if file_can_be_removed(filepath.stem, dirs_base_docs):
                    unused_files += (str(filepath),)
                    remove_cmd = "rm " + str(filepath)
                    os.system(remove_cmd)

            # Check and print other files
            elif filepath.suffix in suffix_image + suffix_other:
                if file_can_be_removed(filename, dirs_base):
                    unused_files += (str(filepath),)
                    remove_cmd = "rm " + str(filepath)
                    os.system(remove_cmd)

        # Print the progress in place
        progress = int(20 * processed_files / number_files)
        sys.stdout.write("\r")
        sys.stdout.write(
            "Checked [%-20s] %d/%d files in %s"
            % ("=" * progress, processed_files, number_files, current_base_dir)
        )
        sys.stdout.flush()

    if len(wrong_extension) != 0:
        print(
            "\n\n\033[31mERROR\033[0m "
            + f"The following {len(wrong_extension)} files have an unsupported extension:\n\n"
            + "\033[31m"
            + "\n".join(wrong_extension)
            + "\033[0m"
            + "\nCheck if you can change the format to one of the following:\n"
            + "\n".join(suffix_allowed)
            + "\nIf you really need that specific extension, add it to the list above.\n"
        )

        exit += 1

    if len(unused_files) != 0:
        print(
            "\n\n\033[31mERROR\033[0m "
            + f"The following {len(unused_files)} files seem to be unused:\n"
            + "\033[31m"
            + "\n".join(unused_files)
            + "\033[0m"
            + "\nYou have 3 options:"
            + "\n\t- Remove them"
            + "\n\t- Use them (check proper include)"
            + "\n\t- Modify the ignore list of this check\n"
        )

        exit += 1

    if exit == 0:
        print(
            "\n\n\033[32mINFO\033[0m Finished check_unused_files.py without any errors."
        )

    return exit


if "__main__" == __name__:
    sys.exit(main())
