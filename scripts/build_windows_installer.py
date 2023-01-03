#! /usr/bin/env python3

import datetime
import os
import pathlib
import shutil
import string
import subprocess
import tempfile

repo_dir = pathlib.Path(__file__).resolve().parents[1]
installer_template_dir = repo_dir / "data" / "windows_installer"
labelbuddy_version = (
    (repo_dir / "data" / "VERSION.txt").read_text("UTF-8").strip()
)
today = datetime.datetime.today().strftime("%Y-%m-%d")
windeployqt_dir = pathlib.Path(os.environ["WINDEPLOYQT_DIR"])
installer_framework = os.environ.get("QT_INSTALLER_FRAMEWORK")
if installer_framework is None:
    try:
        installer_framework = pathlib.Path(
            os.environ["IQTA_TOOLS"],
            "QtInstallerFramework",
            "4.5",
            "bin",
            "binarycreator.exe",
        )
        assert installer_framework.is_file()
    except Exception:
        installer_framework = None

if installer_framework is None:
    installer_framework = (
        list(pathlib.Path("C:", "Qt").glob("QtIFQ-*"))[0]
        / "bin"
        / "binarycreator"
    )
    assert installer_framework.is_file()

with tempfile.TemporaryDirectory() as tmp_dir:
    tmp_dir = pathlib.Path(tmp_dir)
    installer_dir = tmp_dir / "labelbuddy"
    shutil.copytree(installer_template_dir, installer_dir)
    for template_file in (
        installer_dir / "config" / "config.xml",
        installer_dir / "packages" / "labelbuddy" / "meta" / "package.xml",
    ):
        text = string.Template(
            template_file.read_text("UTF-8")
        ).safe_substitute(
            {"labelbuddy_version": labelbuddy_version, "today": today}
        )
        template_file.write_text(text, "UTF-8")
    package_data_dir = installer_dir / "packages" / "labelbuddy" / "data"
    shutil.rmtree(package_data_dir)
    shutil.copytree(windeployqt_dir, package_data_dir)
    shutil.copy(repo_dir / "docs" / "documentation.html", package_data_dir)
    placeholder = package_data_dir / ".dll_and_executable"
    placeholder.unlink()
    installer_name = f"labelbuddy-{labelbuddy_version}_windows_installer.exe"
    subprocess.run(
        [
            installer_framework,
            "--offline-only",
            "-c",
            str(installer_dir / "config" / "config.xml"),
            "-p",
            "packages",
            installer_name,
        ]
    )

print(installer_name)
