#! /usr/bin/env python3

import argparse
import datetime
import hashlib
import os
import pathlib
import shutil
import string
import subprocess
import tempfile


def _repo_dir() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[1]


def _labelbuddy_version() -> str:
    return (_repo_dir() / "data" / "VERSION.txt").read_text("utf-8").strip()


def _build_executable(work_dir: pathlib.Path) -> pathlib.Path:
    cmake_build_dir = work_dir / "cmake_build"
    cmake_build_dir.mkdir()
    subprocess.run(
        ["qmake", str(_repo_dir() / "labelbuddy.pro"), "-config", "release"],
        check=True,
        cwd=cmake_build_dir,
    )
    subprocess.run(["nmake"], check=True, cwd=cmake_build_dir)
    return (cmake_build_dir / "release" / "labelbuddy.exe").resolve()


def _build_package(
    executable: pathlib.Path, work_dir: pathlib.Path
) -> pathlib.Path:
    package_dir = work_dir / f"labelbuddy-{_labelbuddy_version()}"
    package_dir.mkdir()
    subprocess.run(
        [
            "windeployqt",
            "--dir",
            str(package_dir),
            str(executable),
            "--release",
        ],
        check=True,
    )
    shutil.copy(executable, package_dir)
    shutil.copy(_repo_dir() / "docs" / "documentation.html", package_dir)
    return package_dir.resolve()


def _zip_package(package_dir: pathlib.Path) -> pathlib.Path:
    archive = shutil.make_archive(
        str(package_dir),
        "zip",
        root_dir=package_dir.parent,
        base_dir=package_dir.name,
    )
    return pathlib.Path(archive).resolve()


def _installer_framework() -> pathlib.Path:
    try:
        installer_framework = pathlib.Path(
            os.environ["QT_INSTALLER_FRAMEWORK"]
        )
        assert installer_framework.is_file()
        return installer_framework
    except (KeyError, AssertionError):
        pass
    try:
        installer_framework = pathlib.Path(
            os.environ["IQTA_TOOLS"],
            "QtInstallerFramework",
            "4.5",
            "bin",
            "binarycreator.exe",
        )
        assert installer_framework.is_file()
        return installer_framework
    except (KeyError, AssertionError):
        pass
    installer_framework = (
        list(pathlib.Path("C:", "Qt").glob("QtIFW-*"))[0]
        / "bin"
        / "binarycreator"
    )
    assert installer_framework.is_file()
    return installer_framework


def _build_installer(
    package: pathlib.Path, work_dir: pathlib.Path
) -> pathlib.Path:
    installer_dir = (
        work_dir / f"labelbuddy-${_labelbuddy_version()}-windows-installer"
    )
    installer_dir.mkdir()
    shutil.copytree(
        _repo_dir() / "scripts" / "data" / "windows_installer", installer_dir
    )
    today = datetime.datetime.today().strftime("%Y-%m-%d")
    for template_file in (
        installer_dir / "config" / "config.xml",
        installer_dir / "packages" / "labelbuddy" / "meta" / "package.xml",
    ):
        text = string.Template(
            template_file.read_text("UTF-8")
        ).safe_substitute(
            {"labelbuddy_version": _labelbuddy_version(), "today": today}
        )
        template_file.write_text(text, "UTF-8")
    data_dir = installer_dir / "packages" / "labelbuddy" / "data"
    shutil.copytree(package, data_dir)
    shutil.copy(_repo_dir() / "docs" / "documentation.html", data_dir)
    installer_name = (
        f"labelbuddy-{_labelbuddy_version()}-windows-installer.exe"
    )
    subprocess.run(
        [
            _installer_framework(),
            "--offline-only",
            "-c",
            str(installer_dir / "config" / "config.xml"),
            "-p",
            str(installer_dir / "packages"),
            installer_name,
        ]
    )
    return pathlib.Path(installer_name).resolve()


def _md5sum(file_path: pathlib.Path):
    checksum = hashlib.md5(file_path.read_bytes()).hexdigest()
    file_path.with_name(f"{file_path.name}.md5").write_text(
        f"{checksum}  {file_path.name}\n", newline="\n"
    )


def _build(
    executable: pathlib.Path | None = None, build_installer: bool = False
) -> None:
    with tempfile.TemporaryDirectory() as work_dir_path:
        work_dir = pathlib.Path(work_dir_path)
        if executable is None:
            executable = _build_executable(work_dir)
        package = _build_package(executable, work_dir)
        package_zip = _zip_package(package)
        shutil.copy(package_zip, ".")
        _md5sum(pathlib.Path(".") / package_zip.name)
        if build_installer:
            installer = _build_installer(package, work_dir)
            shutil.copy(installer, ".")
            _md5sum(pathlib.Path(".") / installer.name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--exe", type=str, default=None)
    parser.add_argument("-i", "--installer", action="store_true")
    args = parser.parse_args()
    _build(args.exe, args.installer)
