import numpy as np

import sympy as sym
from sympy import Symbol, Matrix, ImmutableMatrix, MatrixSymbol
from sympy.codegen.ast import Assignment

from sympy_common import (
    NamedExpr,
    name_expr,
    find_by_name,
    my_subs,
    cxx_printer,
    my_expression_print,
)


# lambda for q/p
l = Symbol("lambda", real=True)

# h for step length
h = Symbol("h", real=True)

# p for position
p = MatrixSymbol("p", 3, 1)

# d for direction
d = MatrixSymbol("d", 3, 1)

# time
t = Symbol("t", real=True)

# mass
m = Symbol("m", real=True)

# absolute momentum
p_abs = Symbol("p_abs", real=True, positive=True)

# magnetic field
B1 = MatrixSymbol("B1", 3, 1)
B2 = MatrixSymbol("B2", 3, 1)
B3 = MatrixSymbol("B3", 3, 1)


def rk4_full_math():
    k1 = name_expr("k1", d.as_explicit().cross(l * B1))
    p2 = name_expr("p2", p + h / 2 * d + h**2 / 8 * k1.expr)

    k2 = name_expr("k2", (d + h / 2 * k1.expr).as_explicit().cross(l * B2))
    k3 = name_expr("k3", (d + h / 2 * k2.expr).as_explicit().cross(l * B2))
    p3 = name_expr("p3", p + h * d + h**2 / 2 * k3.expr)

    k4 = name_expr("k4", (d + h * k3.expr).as_explicit().cross(l * B3))

    err = name_expr("err", h**2 * (k1.expr - k2.expr - k3.expr + k4.expr).norm(1))

    new_p = name_expr("new_p", p + h * d + h**2 / 6 * (k1.expr + k2.expr + k3.expr))
    new_d_tmp = name_expr(
        "new_d_tmp", d + h / 6 * (k1.expr + 2 * (k2.expr + k3.expr) + k4.expr)
    )
    new_d = name_expr("new_d", new_d_tmp.expr / new_d_tmp.expr.as_explicit().norm())

    dtds = name_expr("dtds", sym.sqrt(1 + m**2 / p_abs**2))
    new_time = name_expr("new_time", t + h * dtds.expr)

    path_derivatives = name_expr("path_derivatives", sym.zeros(8, 1))
    path_derivatives.expr[0:3, 0] = new_d.expr.as_explicit()
    path_derivatives.expr[3, 0] = dtds.expr
    path_derivatives.expr[4:7, 0] = k4.expr.as_explicit()

    D = sym.eye(8)
    D[0:3, :] = new_p.expr.as_explicit().jacobian([p, t, d, l])
    D[4:7, :] = new_d_tmp.expr.as_explicit().jacobian([p, t, d, l])
    D[3, 7] = h * m**2 * l / dtds.expr

    J = MatrixSymbol("J", 8, 8).as_explicit().as_mutable()
    for indices in np.ndindex(J.shape):
        if D[indices] in [0, 1]:
            J[indices] = D[indices]
    J = ImmutableMatrix(J)

    new_J = name_expr("new_J", J * D)

    return [p2, p3, err, new_p, new_d, new_time, path_derivatives, new_J]


def rk4_short_math():
    k1 = name_expr("k1", d.as_explicit().cross(l * B1))
    p2 = name_expr("p2", p + h / 2 * d + h**2 / 8 * k1.name)

    k2 = name_expr("k2", (d + h / 2 * k1.name).as_explicit().cross(l * B2))
    k3 = name_expr("k3", (d + h / 2 * k2.name).as_explicit().cross(l * B2))
    p3 = name_expr("p3", p + h * d + h**2 / 2 * k3.name)

    k4 = name_expr("k4", (d + h * k3.name).as_explicit().cross(l * B3))

    err = name_expr(
        "err", h**2 * (k1.name - k2.name - k3.name + k4.name).as_explicit().norm(1)
    )

    new_p = name_expr("new_p", p + h * d + h**2 / 6 * (k1.name + k2.name + k3.name))
    new_d_tmp = name_expr(
        "new_d_tmp", d + h / 6 * (k1.name + 2 * (k2.name + k3.name) + k4.name)
    )
    new_d = name_expr("new_d", new_d_tmp.name / new_d_tmp.name.as_explicit().norm())

    dtds = name_expr("dtds", sym.sqrt(1 + m**2 / p_abs**2))
    new_time = name_expr("new_time", t + h * dtds.name)

    path_derivatives = name_expr("path_derivatives", sym.zeros(8, 1))
    path_derivatives.expr[0:3, 0] = new_d.name.as_explicit()
    path_derivatives.expr[3, 0] = dtds.name
    path_derivatives.expr[4:7, 0] = k4.name.as_explicit()

    dk1dTL = name_expr("dk1dTL", k1.expr.jacobian([d, l]))
    dk2dTL = name_expr(
        "dk2dTL", k2.expr.jacobian([d, l]) + k2.expr.jacobian(k1.name) * dk1dTL.expr
    )
    dk3dTL = name_expr(
        "dk3dTL",
        k3.expr.jacobian([d, l])
        + k3.expr.jacobian(k2.name) * dk2dTL.name.as_explicit(),
    )
    dk4dTL = name_expr(
        "dk4dTL",
        k4.expr.jacobian([d, l])
        + k4.expr.jacobian(k3.name) * dk3dTL.name.as_explicit(),
    )

    dFdTL = name_expr(
        "dFdTL",
        new_p.expr.as_explicit().jacobian([d, l])
        + new_p.expr.as_explicit().jacobian(k1.name) * dk1dTL.expr
        + new_p.expr.as_explicit().jacobian(k2.name) * dk2dTL.name.as_explicit()
        + new_p.expr.as_explicit().jacobian(k3.name) * dk3dTL.name.as_explicit(),
    )
    dGdTL = name_expr(
        "dGdTL",
        new_d_tmp.expr.as_explicit().jacobian([d, l])
        + new_d_tmp.expr.as_explicit().jacobian(k1.name) * dk1dTL.expr
        + new_d_tmp.expr.as_explicit().jacobian(k2.name) * dk2dTL.name.as_explicit()
        + new_d_tmp.expr.as_explicit().jacobian(k3.name) * dk3dTL.name.as_explicit()
        + new_d_tmp.expr.as_explicit().jacobian(k4.name) * dk4dTL.name.as_explicit(),
    )

    D = sym.eye(8)
    D[0:3, 4:8] = dFdTL.name.as_explicit()
    D[4:7, 4:8] = dGdTL.name.as_explicit()
    D[3, 7] = h * m**2 * l / dtds.name

    J = Matrix(MatrixSymbol("J", 8, 8).as_explicit())
    for indices in np.ndindex(J.shape):
        if D[indices] in [0, 1]:
            J[indices] = D[indices]
    J = ImmutableMatrix(J)
    new_J = name_expr("new_J", J * D)

    return [
        k1,
        p2,
        k2,
        k3,
        p3,
        k4,
        err,
        new_p,
        new_d_tmp,
        new_d,
        dtds,
        new_time,
        path_derivatives,
        dk2dTL,
        dk3dTL,
        dk4dTL,
        dFdTL,
        dGdTL,
        new_J,
    ]


def my_step_function_print(name_exprs, run_cse=True):
    printer = cxx_printer
    outputs = [
        find_by_name(name_exprs, name)[0]
        for name in [
            "p2",
            "p3",
            "err",
            "new_p",
            "new_d",
            "new_time",
            "path_derivatives",
            "new_J",
        ]
    ]

    lines = []

    head = "template <typename T, typename GetB> Acts::Result<bool> rk4(const T* p, const T* d, const T t, const T h, const T lambda, const T m, const T p_abs, GetB getB, T* err, const T errTol, T* new_p, T* new_d, T* new_time, T* path_derivatives, T* J) {"
    lines.append(head)

    lines.append("  const auto B1res = getB(p);")
    lines.append(
        "  if (!B1res.ok()) {\n    return Acts::Result<bool>::failure(B1res.error());\n  }"
    )
    lines.append("  const auto B1 = *B1res;")

    def pre_expr_hook(var):
        if str(var) == "p2":
            return "T p2[3];"
        if str(var) == "p3":
            return "T p3[3];"
        if str(var) == "new_J":
            return "T new_J[64];"
        return None

    def post_expr_hook(var):
        if str(var) == "p2":
            return "const auto B2res = getB(p2);\n  if (!B2res.ok()) {\n    return Acts::Result<bool>::failure(B2res.error());\n  }\n  const auto B2 = *B2res;"
        if str(var) == "p3":
            return "const auto B3res = getB(p3);\n  if (!B3res.ok()) {\n    return Acts::Result<bool>::failure(B3res.error());\n  }\n  const auto B3 = *B3res;"
        if str(var) == "err":
            return (
                "if (*err > errTol) {\n  return Acts::Result<bool>::success(false);\n}"
            )
        if str(var) == "new_time":
            return "if (J == nullptr) {\n  return Acts::Result<bool>::success(true);\n}"
        if str(var) == "new_J":
            return printer.doprint(Assignment(MatrixSymbol("J", 8, 8), var))
        return None

    code = my_expression_print(
        printer,
        name_exprs,
        outputs,
        run_cse=run_cse,
        pre_expr_hook=pre_expr_hook,
        post_expr_hook=post_expr_hook,
    )
    lines.extend([f"  {l}" for l in code.split("\n")])

    lines.append("  return Acts::Result<bool>::success(true);")

    lines.append("}")

    return "\n".join(lines)


all_name_exprs = rk4_short_math()

# attempted manual CSE which turns out a bit slower
#
# sub_name_exprs = [
#     name_expr("hlB1", h * l * B1),
#     name_expr("hlB2", h * l * B2),
#     name_expr("hlB3", h * l * B3),
#     name_expr("lB1", l * B1),
#     name_expr("lB2", l * B2),
#     name_expr("lB3", l * B3),
#     name_expr("h2_2", h**2 / 2),
#     name_expr("h_8", h / 8),
#     name_expr("h_6", h / 6),
#     name_expr("h_2", h / 2),
# ]
# all_name_exprs = [
#     NamedExpr(name, my_subs(expr, sub_name_exprs)) for name, expr in all_name_exprs
# ]
# all_name_exprs.extend(sub_name_exprs)

code = my_step_function_print(
    all_name_exprs,
    run_cse=True,
)

print(
    """// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Note: This file is generated by generate_sympy_stepper.py
//       Do not modify it manually.

#pragma once

#include <Acts/Utilities/Result.hpp>

#include <cmath>
"""
)
print(code)
