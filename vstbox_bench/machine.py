from __future__ import annotations

import os
import platform
import socket
import subprocess
import sys

from .schema import MachineProfile


def _run_text(*args: str) -> str | None:
    try:
        result = subprocess.run(args, check=True, capture_output=True, text=True)
    except (OSError, subprocess.CalledProcessError):
        return None
    value = result.stdout.strip()
    return value or None


def _int_or_none(value: str | None) -> int | None:
    if value is None:
        return None
    try:
        return int(value)
    except ValueError:
        return None


def _mac_profile() -> tuple[str, int | None, int | None, int | None]:
    brand = _run_text("sysctl", "-n", "machdep.cpu.brand_string") or platform.processor() or "unknown"
    physical = _int_or_none(_run_text("sysctl", "-n", "hw.physicalcpu"))
    logical = _int_or_none(_run_text("sysctl", "-n", "hw.logicalcpu"))
    memory = _int_or_none(_run_text("sysctl", "-n", "hw.memsize"))
    return brand, physical, logical, memory


def _linux_profile() -> tuple[str, int | None, int | None, int | None]:
    brand = platform.processor() or "unknown"
    try:
        for line in open("/proc/cpuinfo", encoding="utf-8"):
            if line.lower().startswith("model name"):
                brand = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass

    logical = os.cpu_count()
    physical = None
    memory = None
    try:
        page_size = os.sysconf("SC_PAGE_SIZE")
        pages = os.sysconf("SC_PHYS_PAGES")
        memory = int(page_size * pages)
    except (AttributeError, OSError, ValueError):
        pass
    return brand, physical, logical, memory


def capture_machine_profile() -> MachineProfile:
    system = platform.system()
    if system == "Darwin":
        brand, physical, logical, memory = _mac_profile()
    else:
        brand, physical, logical, memory = _linux_profile()

    return MachineProfile.now(
        hostname=socket.gethostname(),
        os_name=system,
        os_version=platform.mac_ver()[0] if system == "Darwin" else platform.release(),
        architecture=platform.machine(),
        cpu_brand=brand,
        physical_cores=physical,
        logical_cores=logical,
        memory_bytes=memory,
        python_version=platform.python_version(),
    )
