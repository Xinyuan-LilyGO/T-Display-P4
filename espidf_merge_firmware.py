#!/usr/bin/env python3
"""Merge the current ESP-IDF build outputs into one 0x0 flash image.

This script reads the existing build/flash_args file and calls esptool directly.
It does not invoke idf.py, so it will not trigger a project build.
"""

from __future__ import annotations

import argparse
import json
import shlex
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Any


PROJECT_DIR = Path(__file__).resolve().parent
PROJECT_NAME = "edge_agent"


def default_output_name() -> str:
    timestamp = datetime.now().strftime("%Y%m%d%H%M")
    return f"[{PROJECT_NAME}]_firmware_{timestamp}.bin"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Merge existing ESP-IDF build binaries into one firmware image flashed at 0x0."
    )
    parser.add_argument(
        "-B",
        "--build-dir",
        type=Path,
        default=PROJECT_DIR / "build",
        help="ESP-IDF build directory. Defaults to this project's build directory.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Output firmware file. Defaults to this script's directory.",
    )
    parser.add_argument(
        "--chip",
        help="Override chip name. Defaults to extra_esptool_args.chip in flasher_args.json.",
    )
    parser.add_argument(
        "--flash-offset",
        default="0x0",
        help="Base flash offset for the merged raw image. Defaults to 0x0.",
    )
    parser.add_argument(
        "--fill-flash-size",
        help="Pad the output to this flash size. Defaults to flash_settings.flash_size.",
    )
    parser.add_argument(
        "--no-fill",
        action="store_true",
        help="Do not pad the output image to the configured flash size.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the esptool command without running it.",
    )
    return parser.parse_args()


def load_json(path: Path) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as file:
            data = json.load(file)
    except FileNotFoundError:
        raise SystemExit(f"Missing {path}. Build the project once before merging firmware.")
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON in {path}: {exc}") from exc

    if not isinstance(data, dict):
        raise SystemExit(f"Invalid {path}: expected a JSON object.")
    return data


def resolve_path(path: Path) -> Path:
    if path.is_absolute():
        return path.resolve()
    return (Path.cwd() / path).resolve()


def format_command(command: list[str]) -> str:
    if sys.platform == "win32":
        return subprocess.list2cmdline(command)
    return " ".join(shlex.quote(part) for part in command)


def validate_build_files(build_dir: Path, flasher_args: dict[str, Any]) -> None:
    flash_args_path = build_dir / "flash_args"
    if not flash_args_path.is_file():
        raise SystemExit(f"Missing {flash_args_path}. Build the project once before merging firmware.")

    flash_files = flasher_args.get("flash_files", {})
    if not isinstance(flash_files, dict):
        raise SystemExit("Invalid flasher_args.json: flash_files must be an object.")

    missing_files: list[str] = []
    for offset, file_name in sorted(flash_files.items(), key=lambda item: int(item[0], 0)):
        file_path = build_dir / str(file_name)
        if not file_path.is_file():
            missing_files.append(f"  {offset}: {file_path}")

    if missing_files:
        missing = "\n".join(missing_files)
        raise SystemExit(f"Missing build output files:\n{missing}\nBuild the project once before merging firmware.")


def main() -> int:
    args = parse_args()
    build_dir = resolve_path(args.build_dir)
    flasher_args = load_json(build_dir / "flasher_args.json")
    validate_build_files(build_dir, flasher_args)

    output = args.output
    if output is None:
        output = PROJECT_DIR / default_output_name()
    else:
        output = resolve_path(output)

    extra_esptool_args = flasher_args.get("extra_esptool_args", {})
    flash_settings = flasher_args.get("flash_settings", {})
    chip = args.chip or extra_esptool_args.get("chip")
    if not chip:
        raise SystemExit("Unable to determine chip. Pass --chip, for example: --chip esp32p4")

    fill_flash_size = None
    if not args.no_fill:
        fill_flash_size = args.fill_flash_size or flash_settings.get("flash_size")

    command = [
        sys.executable,
        "-m",
        "esptool",
        "--chip",
        str(chip),
        "merge_bin",
        "-o",
        str(output),
        "-f",
        "raw",
        "-t",
        args.flash_offset,
    ]
    if fill_flash_size:
        command.extend(["--fill-flash-size", str(fill_flash_size)])
    command.append("@flash_args")

    print(f"Build dir: {build_dir}")
    print(f"Output:    {output}")
    print(f"Chip:      {chip}")
    if fill_flash_size:
        print(f"Fill size: {fill_flash_size}")
    else:
        print("Fill size: disabled")
    print()
    print("Command:")
    print(f"  cd {build_dir}")
    print(f"  {format_command(command)}")

    if args.dry_run:
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    try:
        subprocess.run(command, cwd=build_dir, check=True)
    except FileNotFoundError as exc:
        raise SystemExit(f"Failed to run Python: {exc}") from exc
    except subprocess.CalledProcessError as exc:
        return exc.returncode

    print()
    print(f"Merged firmware created: {output}")
    print(f"Flash this file at offset: {args.flash_offset}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
