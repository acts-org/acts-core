# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import sys, inspect
from pathlib import Path
from typing import Optional, Protocol, Union, List, Dict, Tuple
import os
import re

from acts.ActsPythonBindings._examples import *
from acts import ActsPythonBindings
import acts
from acts._adapter import _patch_config, _patchKwargsConstructor

_propagators = []
_concrete_propagators = []
for stepper in ("Eigen", "Atlas", "StraightLine"):
    for navigator in ("", "Detector"):
        _propagators.append(
            getattr(ActsPythonBindings._propagator, f"{stepper}{navigator}Propagator")
        )
        _concrete_propagators.append(
            getattr(
                ActsPythonBindings._propagator,
                f"{stepper}{navigator}ConcretePropagator",
            )
        )


def ConcretePropagator(propagator):
    for prop, prop_if in zip(_propagators, _concrete_propagators):
        if isinstance(propagator, prop):
            return prop_if(propagator)

    raise TypeError(f"Unknown propagator {type(propagator).__name__}")


_patch_config(ActsPythonBindings._examples)

# Manually patch ExaTrkX constructors
# Need to do it this way, since they are not always present
for module in [
    "TorchMetricLearning",
    "OnnxMetricLearning",
    "TorchEdgeClassifier",
    "OnnxEdgeClassifier",
]:
    if hasattr(ActsPythonBindings._examples, module):
        _patchKwargsConstructor(getattr(ActsPythonBindings._examples, module))


def _makeLayerTriplet(*args, **kwargs):
    if len(args) == 1:
        _type = type(args[0])
        negative = central = positive = args[0]
    else:
        negative = kwargs.get("negative")
        central = kwargs.get("central")
        positive = kwargs.get("positive")
        types = []
        if negative is not None:
            types.append(type(negative))
        if central is not None:
            types.append(type(central))
        if positive is not None:
            types.append(type(positive))

        assert all(types[0] == t for t in types), "Inconsistent types"

        _type = types[0]

    def fill(obj):
        if negative is not None:
            obj.negative = negative
        if central is not None:
            obj.central = central
        if positive is not None:
            obj.positive = positive
        return obj

    if _type == bool:
        return fill(TGeoDetector.Config.LayerTripletBool())
    elif _type == list:
        if all(
            all(isinstance(v, str) for v in vv)
            for vv in (negative, central, positive)
            if vv is not None
        ):
            return fill(TGeoDetector.Config.LayerTripletVectorString())
        elif all(
            all(
                (isinstance(v, tuple) or isinstance(v, list))
                and isinstance(v[0], int)
                and isinstance(v[1], inspect.unwrap(TGeoDetector.Config.BinningType))
                for v in vv
            )
            for vv in (negative, central, positive)
            if vv is not None
        ):
            return fill(TGeoDetector.Config.LayerTripletVectorBinning())
        else:
            raise TypeError("Invalid types for list input")
    elif _type == tuple:
        if all(
            all(isinstance(v, float) for v in vv)
            for vv in (negative, central, positive)
            if vv is not None
        ):
            negative = Interval(*negative) if negative is not None else None
            central = Interval(*central) if central is not None else None
            positive = Interval(*positive) if positive is not None else None
            return fill(TGeoDetector.Config.LayerTripletInterval())
        else:
            raise TypeError("Invalid types for tuple input")
    elif _type == Interval:
        return fill(TGeoDetector.Config.LayerTripletInterval())
    elif _type == str:
        return fill(TGeoDetector.Config.LayerTripletString())
    elif _type == float:
        return fill(TGeoDetector.Config.LayerTripletDouble())
    else:
        raise TypeError("Unknown type given")


TGeoDetector.Config.LayerTriplet = _makeLayerTriplet


def _process_volume_intervals(kwargs):
    if len(kwargs) == 0:
        return kwargs  # prevent infinite recursion
    _kwargs = kwargs.copy()

    v = TGeoDetector.Config.Volume()
    for name, value in inspect.getmembers(inspect.unwrap(TGeoDetector.Config.Volume)):
        if not isinstance(getattr(v, name), inspect.unwrap(Interval)):
            continue
        if not name in _kwargs:
            continue
        if not isinstance(_kwargs[name], tuple):
            continue
        _kwargs[name] = Interval(*_kwargs[name])

    return _kwargs


_patchKwargsConstructor(TGeoDetector.Config.Volume, proc=_process_volume_intervals)


def NamedTypeArgs(**namedTypeArgs):
    """Decorator to move args of a named type (e.g. `namedtuple` or `Enum`) to kwargs based on type, so user doesn't need to specify the key name.
    Also allows the keyword argument to be converted from a built-in type (eg. `tuple` or `int`).
    """

    namedTypeClasses = {c: a for a, c in namedTypeArgs.items()}

    def NamedTypeArgsDecorator(func):
        from functools import wraps

        @wraps(func)
        def NamedTypeArgsWrapper(*args, **kwargs):
            from collections.abc import Iterable

            for k, v in kwargs.items():
                cls = namedTypeArgs.get(k)
                if (
                    cls is not None
                    and v is not None
                    and type(v).__module__ == int.__module__  # is v a 'builtins'?
                    and not (
                        issubclass(type(v), Iterable) and all(type(e) is cls for e in v)
                    )  # not [cls]
                ):
                    if issubclass(cls, Iterable):
                        kwargs[k] = cls(*v)
                    else:
                        kwargs[k] = cls(v)

            newargs = []
            for i, a in enumerate(args):
                k = namedTypeClasses.get(type(a))
                if k is None:
                    newargs.append(a)
                    if i > len(newargs):
                        types = [type(a).__name__ for a in args]
                        raise TypeError(
                            f"{func.__name__}() positional argument {i} of type {type(a)} follows named-type arguments, which were converted to keyword arguments. All argument types: {types}"
                        )
                elif k in kwargs:
                    raise TypeError(f"{func.__name__}() keyword argument repeated: {k}")
                else:
                    kwargs[k] = a
            return func(*newargs, **kwargs)

        return NamedTypeArgsWrapper

    return NamedTypeArgsDecorator


def defaultKWArgs(**kwargs) -> dict:
    """Removes keyword arguments that are None or a list of all None (eg. [None,None]).
    This keeps the called function's defaults."""
    from collections.abc import Iterable

    return {
        k: v
        for k, v in kwargs.items()
        if not (
            v is None or (isinstance(v, Iterable) and all([vv is None for vv in v]))
        )
    }


def dump_func_args(func, *args, **kwargs):
    def valstr(v, d=set()):
        from collections.abc import Callable

        if re.match(r"^<[\w.]+ object at 0x[\da-f]+>$", repr(v)):
            name = type(v).__module__ + "." + type(v).__qualname__
            if len(d) < 10 and name not in d and type(v).__name__ != "Sequencer":
                try:
                    a = [
                        k + " = " + valstr(getattr(v, k), set(d | {name}))
                        for k in dir(v)
                        if not (
                            k.startswith("__") or isinstance(getattr(v, k), Callable)
                        )
                    ]
                except:
                    a = []
            else:
                a = []
            if a:
                return name + "{ " + ", ".join(a) + " }"
            else:
                return name + "{}"
        else:
            return repr(v)

    def keyvalstr(kv):
        return "{0} = {1}".format(kv[0], valstr(kv[1]))

    try:
        func_kwargs = inspect.signature(func).bind(*args, **kwargs).arguments
        func_args = func_kwargs.pop("args", [])
        func_args.count(None)  # raise AttributeError if not tuple/list
        func_kwargs.update(func_kwargs.pop("kwargs", {}))
    except (ValueError, AttributeError):
        func_kwargs = kwargs
        func_args = args
    func_args_str = ", ".join(
        list(map(valstr, func_args)) + list(map(keyvalstr, func_kwargs.items()))
    )
    if not (
        func_args_str == ""
        and any([a == "Config" for a in func.__qualname__.split(".")])
    ):
        print(f"{func.__module__}.{func.__qualname__} ( {func_args_str} )")


def dump_args(func):
    """
    Decorator to print function call details.
    This includes parameters names and effective values.
    https://stackoverflow.com/questions/6200270/decorator-that-prints-function-call-details-parameters-names-and-effective-valu
    """
    from functools import wraps

    @wraps(func)
    def dump_args_wrapper(*args, **kwargs):
        dump_func_args(func, *args, **kwargs)
        return func(*args, **kwargs)

    # fix up any attributes broken by the wrapping
    for name in dir(func):
        if not name.startswith("__"):
            obj = getattr(func, name)
            wrapped = getattr(dump_args_wrapper, name, None)
            if type(obj) is not type(wrapped):
                setattr(dump_args_wrapper, name, obj)

    return dump_args_wrapper


def dump_args_calls(myLocal=None, mods=None, quiet=False):
    """
    Wrap all Python bindings calls to acts and its submodules in dump_args.
    Specify myLocal=locals() to include imported symbols too.
    """
    from collections.abc import Callable

    def _allmods(mod, base, found):
        import types

        mods = [mod]
        found.add(mod)
        for name, obj in sorted(
            vars(mod).items(),
            key=lambda m: (
                (2, m[0])
                if m[0] == "ActsPythonBindings"
                else (1, m[0]) if m[0].startswith("_") else (0, m[0])
            ),
        ):
            if (
                not name.startswith("__")
                and type(obj) is types.ModuleType
                and obj.__name__.startswith(base)
                and f"{mod.__name__}.{name}" in sys.modules
                and obj not in found
            ):
                mods += _allmods(obj, base, found)
        return mods

    if mods is None:
        mods = _allmods(acts, "acts.", set())
    elif not isinstance(mods, list):
        mods = [mods]

    donemods = []
    alldone = 0
    for mod in mods:
        done = 0
        for name in dir(mod):
            # if name in {"Config", "Interval", "IMaterialDecorator"}: continue  # skip here if we don't fix up attributes in dump_args and unwrap classes elsewhere
            obj = getattr(mod, name, None)
            if not (
                not name.startswith("__")
                and isinstance(obj, Callable)
                and hasattr(obj, "__module__")
                and obj.__module__.startswith("acts.ActsPythonBindings")
                and obj.__qualname__ != "_Sequencer.Config"
                and not hasattr(obj, "__wrapped__")
            ):
                continue
            # wrap class's contained methods
            done += dump_args_calls(myLocal, [obj], True)
            wrapped = dump_args(obj)
            setattr(mod, name, wrapped)
            if myLocal and hasattr(myLocal, name):
                setattr(myLocal, name, wrapped)
            done += 1
        if done:
            alldone += done
            donemods.append(f"{mod.__name__}:{done}")
    if not quiet and donemods:
        print("dump_args for module functions:", ", ".join(donemods))
    return alldone


class CustomLogLevel(Protocol):
    def __call__(
        self,
        minLevel: acts.logging.Level = acts.logging.VERBOSE,
        maxLevel: acts.logging.Level = acts.logging.FATAL,
    ) -> acts.logging.Level: ...


def defaultLogging(
    s=None,
    logLevel: Optional[acts.logging.Level] = None,
) -> CustomLogLevel:
    """
    Establishes a default logging strategy for the python examples interface.

    Returns a function that determines the log level in the following schema:
    - if `logLevel` is set use it otherwise use the log level of the sequencer `s.config.logLevel`
    - the returned log level is bound between `minLevel` and `maxLevel` provided to `customLogLevel`

    Examples:
    - `customLogLevel(minLevel=acts.logging.INFO)` to get a log level that is INFO or higher
      (depending on the sequencer and `logLevel` param) which is useful to suppress a component which
      produces a bunch of logs below INFO and you are actually more interested in another component
    - `customLogLevel(maxLevel=acts.logging.INFO)` to get a log level that is INFO or lower
      (depending on the sequencer and `logLevel` param) which is useful to get more details from a
      component that will produce logs of interest below the default level
    - in summary `minLevel` defines the maximum amount of logging and `maxLevel` defines the minimum amount of logging
    """

    def customLogLevel(
        minLevel: acts.logging.Level = acts.logging.VERBOSE,
        maxLevel: acts.logging.Level = acts.logging.FATAL,
    ) -> acts.logging.Level:
        l = logLevel if logLevel is not None else s.config.logLevel
        return acts.logging.Level(min(maxLevel.value, max(minLevel.value, l.value)))

    return customLogLevel


class Sequencer(ActsPythonBindings._examples._Sequencer):
    _autoFpeMasks: Optional[List["FpeMask"]] = None

    def __init__(self, *args, **kwargs):
        # if we have the argument already in kwargs, we optionally convert them from tuples
        if "fpeMasks" in kwargs:
            m = kwargs["fpeMasks"]
            if isinstance(m, list) and len(m) > 0 and isinstance(m[0], tuple):
                n = []
                for loc, fpe, count in m:
                    file, lines = self.FpeMask.parse_loc(loc)
                    t = _fpe_types_to_enum[fpe] if isinstance(fpe, str) else fpe
                    n.append(self.FpeMask(file, lines, t, count))
                kwargs["fpeMasks"] = n

        kwargs["fpeMasks"] = kwargs.get("fpeMasks", []) + self._getAutoFpeMasks()

        self._printFpeSummary(kwargs["fpeMasks"])

        cfg = self.Config()
        if len(args) == 1 and isinstance(args[0], self.Config):
            cfg = args[0]
        if "config" in kwargs:
            cfg = kwargs.pop("config")

        for k, v in kwargs.items():
            if not hasattr(cfg, k):
                raise ValueError(f"Sequencer.Config does not have field {k}")
            if isinstance(v, Path):
                v = str(v)

            setattr(cfg, k, v)

        if hasattr(ActsPythonBindings._examples._Sequencer, "__wrapped__"):
            dump_func_args(Sequencer, cfg)
        super().__init__(cfg)

    class FpeMask(ActsPythonBindings._examples._Sequencer._FpeMask):
        @classmethod
        def fromFile(cls, file: Union[str, Path]) -> List["FpeMask"]:
            if isinstance(file, str):
                file = Path(file)

            if file.suffix in (".yml", ".yaml"):
                try:
                    return cls.fromYaml(file)
                except ImportError:
                    print("FPE mask input file is YAML, but PyYAML is not installed")
                    raise

        @classmethod
        def fromYaml(cls, file: Union[str, Path]) -> List["FpeMask"]:
            import yaml

            with file.open() as fh:
                d = yaml.safe_load(fh)

            return cls.fromDict(d)

        _fpe_types_to_enum = {v.name: v for v in acts.FpeType.values}

        @staticmethod
        def toDict(
            masks: List["FpeMask"],
        ) -> Dict[str, Dict[str, int]]:
            out = {}
            for mask in masks:
                loc_str = f"{mask.file}:"
                start, end = mask.lines
                if start == end - 1:
                    loc_str += str(start)
                else:
                    loc_str += f"({start}, {end}]"
                out.setdefault(loc_str, {})
                out[loc_str][mask.type.name] = mask.count

                return out

        @staticmethod
        def parse_loc(loc: str) -> Tuple[str, Tuple[int, int]]:
            file, lines = loc.split(":", 1)

            if m := re.match(r"^\((\d+) ?, ?(\d+)\]$", lines.strip()):
                start, end = map(int, m.groups())
            elif m := re.match(r"^(\d+) ?- ?(\d+)$", lines.strip()):
                start, end = map(int, m.groups())
                end += 1  # assumption here is that it's inclusive
            else:
                start = int(lines)
                end = start + 1

            return file, (start, end)

        @classmethod
        def fromDict(cls, d: Dict[str, Dict[str, int]]) -> List["FpeMask"]:
            out = []
            for loc, types in d.items():
                file, lines = cls.parse_loc(loc)

                for fpe, count in types.items():
                    out.append(cls(file, lines, cls._fpe_types_to_enum[fpe], count))
            return out

    @classmethod
    def srcdir(cls) -> Path:
        return Path(cls._sourceLocation).parent.parent.parent.parent

    @classmethod
    def _getAutoFpeMasks(cls) -> List[FpeMask]:
        if cls._autoFpeMasks is not None:
            return cls._autoFpeMasks

        srcdir = cls.srcdir()

        cls._autoFpeMasks = []

        for root, _, files in os.walk(srcdir):
            root = Path(root)
            for f in files:
                if (
                    not f.endswith(".hpp")
                    and not f.endswith(".cpp")
                    and not f.endswith(".ipp")
                ):
                    continue
                f = root / f
                #  print(f)
                with f.open("r") as fh:
                    lines = fh.readlines()
                for i, line in enumerate(lines):
                    if m := re.match(r".*\/\/ ?MARK: ?(fpeMask\(.*)$", line):
                        exp = m.group(1)
                        for m in re.findall(
                            r"fpeMask\( ?(\w+), ?(\d+) ?, ?#(\d+) ?\)", exp
                        ):
                            fpeType, count, _ = m
                            count = int(count)
                            rel = f.relative_to(srcdir)
                            cls._autoFpeMasks.append(
                                cls.FpeMask(
                                    str(rel),
                                    (i + 1, i + 2),
                                    cls.FpeMask._fpe_types_to_enum[fpeType],
                                    count,
                                )
                            )

                    if m := re.match(
                        r".*\/\/ ?MARK: ?fpeMaskBegin\( ?(\w+), ?(\d+) ?, ?#?(\d+) ?\)",
                        line,
                    ):
                        fpeType, count, _ = m.groups()
                        count = int(count)
                        rel = f.relative_to(srcdir)

                        start = i + 1
                        end = None

                        # look for end marker
                        for j, line2 in enumerate(lines[i:]):
                            if m := re.match(
                                r".*\/\/ ?MARK: ?fpeMaskEnd\( ?(\w+) ?\)$", line2
                            ):
                                endType = m.group(1)
                                if endType == fpeType:
                                    end = i + j + 1
                                    break

                        if end is None:
                            raise ValueError(
                                f"Found fpeMaskBegin but no fpeMaskEnd for {rel}:{start}"
                            )
                        cls._autoFpeMasks.append(
                            cls.FpeMask(
                                str(rel),
                                (start, end + 1),
                                cls.FpeMask._fpe_types_to_enum[fpeType],
                                count,
                            )
                        )

        return cls._autoFpeMasks

    @classmethod
    def _printFpeSummary(cls, masks: List[FpeMask]):
        if len(masks) == 0 or "ACTS_SEQUENCER_DISABLE_FPEMON" in os.environ:
            return

        # Try to make a nice summary with rich, or fallback to a plain text one
        try:
            import rich

            have_rich = True
        except ImportError:
            have_rich = False

        error = False
        srcdir = cls.srcdir()

        if not have_rich or not sys.stdout.isatty():
            print("FPE masks:")
            for mask in masks:
                s = f"{mask.file}:{mask.lines[0]}: {mask.type.name}: {mask.count}"

                full_path = srcdir / mask.file
                if not full_path.exists():
                    print(f"- {s}\n  [File at {full_path} does not exist!]")
                    error = True
                else:
                    print(f"- {s}")

        else:
            import rich
            import rich.rule
            import rich.panel
            from rich.markdown import Markdown as md
            import rich.syntax
            import rich.table
            import rich.text

            rich.print(rich.rule.Rule("FPE masks"))

            for i, mask in enumerate(masks):
                if i > 0:
                    rich.print(rich.rule.Rule())
                full_path = srcdir / mask.file
                if not full_path.exists():
                    rich.print(
                        rich.panel.Panel(
                            md(f"File at **{full_path}** does not exist"),
                            title=f"{mask}",
                            style="red",
                        )
                    )
                    error = True
                    continue

                start, end = mask.lines
                start = max(0, start - 2)
                end += 2
                rich.print(
                    rich.panel.Panel(
                        rich.syntax.Syntax.from_path(
                            full_path,
                            line_numbers=True,
                            line_range=(start, end),
                            highlight_lines=list(range(*mask.lines)),
                        ),
                        title=f"{mask}",
                        subtitle=f"{full_path}",
                    )
                )

            rich.print(rich.rule.Rule())

            table = rich.table.Table(title="FPE Summary", expand=True)
            table.add_column("File")
            table.add_column("Lines")
            table.add_column("FPE type")
            table.add_column("Mask limit")

            for mask in masks:
                start, end = mask.lines
                if start + 1 == end:
                    line_str = str(start)
                else:
                    line_str = f"({start}-{end}]"

                full_path = srcdir / mask.file

                table.add_row(
                    str(mask.file),
                    line_str,
                    mask.type.name,
                    str(mask.count),
                    style="red" if not full_path.exists() else None,
                )

            rich.print(table)

        if error:
            raise RuntimeError("Sequencer FPE masking configuration has errors")
