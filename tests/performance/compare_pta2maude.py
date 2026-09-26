#!/usr/bin/env python3
"""Compare the same PTA2Maude query through Python and native Maude-SE."""

import argparse
import hashlib
import json
import os
import re
import statistics
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "tests/data/pta2maude"
CASES = {
    "coffee": ("coffee.maude", "solution"),
    "ex-fig3a": ("ex-fig3a.maude", "solution"),
    "ex-fig3b": ("ex-fig3b.maude", "no-solution"),
}
ANSI = re.compile(r"\x1b\[[0-9;]*m")
SEARCH_TIME = re.compile(
    r"rewrites:\s*(\d+)\s+in\s+([0-9.]+)ms cpu\s+\(([0-9.]+)ms real\)"
)
BAD_OUTPUT = re.compile(r"Warning:|Parse error|Maude internal error|\berror:", re.I)


def python_maude_lib(cli: Path) -> Path:
    interpreter = cli.parent / "python"
    if not interpreter.is_file():
        raise ValueError("--python-lib is required when the CLI has no sibling python")
    command = [
        str(interpreter),
        "-c",
        "from pathlib import Path; import maudeSE; "
        "print(Path(maudeSE.__file__).parent / 'maude')",
    ]
    result = subprocess.run(command, text=True, capture_output=True, check=True)
    return Path(result.stdout.strip()).resolve()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run_once(kind: str, executable: Path, library: Path, solver: str,
             case: str, timeout: float) -> dict:
    filename, expected = CASES[case]
    model = DATA / filename
    command = [str(executable), str(model)]
    if kind == "python":
        command += ["-s", solver]
    env = os.environ.copy()
    env["MAUDE_LIB"] = os.pathsep.join((str(DATA), str(library)))
    start = time.perf_counter_ns()
    try:
        completed = subprocess.run(
            command, input="quit\n", text=True, capture_output=True,
            cwd=DATA, env=env, timeout=timeout,
        )
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError(f"{case}/{kind} timed out after {timeout}s") from exc
    wall_ms = (time.perf_counter_ns() - start) / 1_000_000
    output = ANSI.sub("", completed.stdout + completed.stderr)
    if completed.returncode or BAD_OUTPUT.search(output):
        raise RuntimeError(
            f"{case}/{kind} failed (exit {completed.returncode}):\n"
            + output[-3000:]
        )

    found = bool(re.search(r"\bSolution 1\b", output))
    absent = bool(re.search(r"\bNo (?:more )?solutions?\.", output))
    actual = "solution" if found and not absent else "no-solution" if absent and not found else "invalid"
    if actual != expected:
        raise RuntimeError(
            f"{case}/{kind}: expected {expected}, got {actual}:\n" + output[-3000:]
        )

    timing = SEARCH_TIME.search(output)
    if timing is None:
        raise RuntimeError(f"{case}/{kind}: Maude search timing was not found:\n" + output[-3000:])
    return {
        "wall_ms": wall_ms,
        "rewrites": int(timing.group(1)),
        "search_cpu_ms": float(timing.group(2)),
        "search_real_ms": float(timing.group(3)),
        "result": actual,
    }


def median(samples: list[dict], field: str) -> float:
    return statistics.median(sample[field] for sample in samples)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--python", required=True, type=Path, help="maude-se CLI from an installed wheel")
    parser.add_argument("--native", required=True, type=Path, help="maude-se-<solver> inside its bundle")
    parser.add_argument("--python-lib", type=Path, help="directory containing the wheel's Maude files")
    parser.add_argument("--solver", required=True, choices=("z3", "yices", "cvc5"))
    parser.add_argument("--case", action="append", choices=tuple(CASES), dest="cases")
    parser.add_argument("--repeats", type=int, default=20)
    parser.add_argument("--warmups", type=int, default=2)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--json", action="store_true", help="write JSON to stdout")
    args = parser.parse_args()
    if args.repeats < 1 or args.warmups < 0 or args.timeout <= 0:
        parser.error("repeats and timeout must be positive; warmups must be nonnegative")

    python_cli = args.python.expanduser().resolve()
    native_cli = args.native.expanduser().resolve()
    for path in (python_cli, native_cli):
        if not path.is_file() or not os.access(path, os.X_OK):
            parser.error(f"executable not found: {path}")
    if native_cli.name != f"maude-se-{args.solver}":
        parser.error("the native executable name must match --solver")
    python_lib = (args.python_lib.expanduser().resolve() if args.python_lib
                  else python_maude_lib(python_cli))
    native_lib = native_cli.parent
    for path in (python_lib, native_lib):
        for filename in ("smt.maude", "smt-check.maude", "maude-se-meta.maude"):
            if not (path / filename).is_file():
                parser.error(f"required Maude file missing: {path / filename}")

    cases = args.cases or list(CASES)
    report = {
        "solver": args.solver,
        "python_cli": str(python_cli),
        "python_maude_lib": str(python_lib),
        "native_cli": str(native_cli),
        "python_cli_sha256": sha256(python_cli),
        "native_sha256": sha256(native_cli),
        "repeats": args.repeats,
        "warmups": args.warmups,
        "timeout_s": args.timeout,
        "cases": {},
    }
    for case in cases:
        for _ in range(args.warmups):
            for kind, executable, library in (
                ("python", python_cli, python_lib),
                ("native", native_cli, native_lib),
            ):
                run_once(kind, executable, library, args.solver, case, args.timeout)
        samples = {"python": [], "native": []}
        for iteration in range(args.repeats):
            order = ("python", "native") if iteration % 2 == 0 else ("native", "python")
            for kind in order:
                executable, library = ((python_cli, python_lib) if kind == "python"
                                       else (native_cli, native_lib))
                samples[kind].append(
                    run_once(kind, executable, library, args.solver, case, args.timeout)
                )
        rewrite_counts = {
            sample["rewrites"] for kind in ("python", "native")
            for sample in samples[kind]
        }
        if len(rewrite_counts) != 1:
            raise RuntimeError(
                f"{case}: search rewrite counts differ across runs or execution paths: "
                f"{sorted(rewrite_counts)}"
            )
        report["cases"][case] = {
            "expected": CASES[case][1],
            "model_sha256": sha256(DATA / CASES[case][0]),
            "rewrites": rewrite_counts.pop(),
            "python": samples["python"],
            "native": samples["native"],
            "median_python_wall_ms": median(samples["python"], "wall_ms"),
            "median_native_wall_ms": median(samples["native"], "wall_ms"),
            "median_python_search_ms": median(samples["python"], "search_real_ms"),
            "median_native_search_ms": median(samples["native"], "search_real_ms"),
        }

    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"solver: {args.solver}  repeats: {args.repeats}  warmups: {args.warmups}")
        print("case         result       wall ms (Python/native)   search ms (Python/native)")
        for case, data in report["cases"].items():
            print(
                f"{case:<12} {data['expected']:<12} "
                f"{data['median_python_wall_ms']:>7.1f}/{data['median_native_wall_ms']:<7.1f} "
                f"{data['median_python_search_ms']:>7.1f}/{data['median_native_search_ms']:<7.1f}"
            )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
