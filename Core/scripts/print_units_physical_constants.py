#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

#
# Compute physical constants

from decimal import Decimal


# circle constant beyond double precision floating point
# from https://oeis.org/A000796
pi_digits = (
    3,
    1,
    4,
    1,
    5,
    9,
    2,
    6,
    5,
    3,
    5,
    8,
    9,
    7,
    9,
    3,
    2,
    3,
    8,
    4,
    6,
    2,
    6,
    4,
    3,
    3,
    8,
    3,
    2,
    7,
    9,
    5,
    0,
    2,
    8,
    8,
    4,
    1,
    9,
    7,
    1,
    6,
    9,
    3,
    9,
    9,
    3,
    7,
    5,
    1,
    0,
    5,
    8,
    2,
    0,
    9,
    7,
    4,
    9,
    4,
    4,
    5,
    9,
    2,
    3,
    0,
    7,
    8,
    1,
    6,
    4,
    0,
    6,
    2,
    8,
    6,
    2,
    0,
    8,
    9,
    9,
    8,
    6,
    2,
    8,
    0,
    3,
    4,
    8,
    2,
    5,
    3,
    4,
    2,
    1,
    1,
    7,
    0,
    6,
    7,
    9,
    8,
    2,
    1,
    4,
)
pi = Decimal((0, pi_digits, 1 - len(pi_digits)))

# values are taken from Review of Particle Physics 2020
# see https://pdg.lbl.gov/2020/reviews/rpp2020-rev-phys-constants.pdf

# vacuum speed-of-light, exact
c_m_s = Decimal((0, (2, 9, 9, 7, 9, 2, 4, 5, 8), 0))
# electron charge, exact
e_C = Decimal((0, (1, 6, 0, 2, 1, 7, 6, 6, 3, 4), -28))
# Planck constant, exact
h_Js = Decimal((0, (6, 6, 2, 6, 0, 7, 0, 1, 5), -42))
h_eVs = h_Js / e_C
h_MeVs = h_eVs / Decimal((0, (1,), 6))
h_GeVs = h_eVs / Decimal((0, (1,), 9))
# reduced Planck constant
hbar_Js = h_Js / (2 * pi)
hbar_eVs = h_eVs / (2 * pi)
hbar_MeVs = h_MeVs / (2 * pi)
hbar_GeVs = h_GeVs / (2 * pi)

# unit conversions
degree_radian = pi / Decimal((0, (1, 8, 0), 0))
J_eV = 1 / e_C
J_GeV = J_eV / Decimal((0, (1,), 9))

full_constants = [
    ("pi", pi, ""),
    ("speed of light in vacuum", c_m_s, "m/s"),
    ("electron charge", e_C, "C"),
    ("Planck constant", h_Js, "J*s"),
    ("Planck constant", h_eVs, "eV*s"),
    ("Planck constant", h_MeVs, "MeV*s"),
    ("Planck constant", h_GeVs, "GeV*s"),
    ("reduced Planck constant", hbar_Js, "J*s"),
    ("reduced Planck constant", hbar_eVs, "eV*s"),
    ("reduced Planck constant", hbar_MeVs, "MeV*s"),
    ("reduced Planck constant", hbar_GeVs, "GeV*s"),
    ("degree", degree_radian, "radian"),
    ("Joule", J_eV, "eV"),
    ("Joule", J_GeV, "GeV"),
]
float_constants = [(n, float(v), u) for n, v, u in full_constants]


def print_constants(constants):
    # find the largest name first for consistent formatting
    max_len_name = max((len(n) for n, *_ in constants))
    line_format = f"{{:>{max_len_name}}}:  {{}} {{}}"
    for name, value, unit in constants:
        print(line_format.format(name, value, unit))


if __name__:
    print("=== high precision values:")
    print_constants(full_constants)
    print("=== double precision values:")
    print_constants(float_constants)
