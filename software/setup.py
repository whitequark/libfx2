import os
from os import path

from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext
from setuptools.command.bdist_egg import bdist_egg
from setuptools.command.sdist import sdist

from distutils import log
from distutils.spawn import spawn
from distutils.dir_util import mkpath
from distutils.errors import DistutilsExecError


class Fx2BuildExt(build_ext):
    def run(self):
        try:
            firmware_dir = path.join("..", "firmware")
            spawn(["make", "-C", path.join(firmware_dir, "library")], dry_run=self.dry_run)
            spawn(["make", "-C", path.join(firmware_dir, "boot-cypress")], dry_run=self.dry_run)

            bootloader_ihex = path.join("..", "firmware", "boot-cypress",
                                        "build", "boot-cypress.ihex")
            self.copy_file(bootloader_ihex, "fx2")
        except DistutilsExecError as e:
            if os.access(path.join("fx2", "boot-cypress.ihex"), os.R_OK):
                log.info("using prebuilt bootloader")
            else:
                raise


class Fx2BdistEgg(bdist_egg):
    def run(self):
        # Allow installing as a dependency via pip.
        self.run_command("build_ext")
        bdist_egg.run(self)


class Fx2Sdist(sdist):
    def run(self):
        # Make sure the included ihex files are up to date.
        self.run_command("build_ext")
        sdist.run(self)


setup(
    name="fx2",
    version="0.9",
    author="whitequark",
    author_email="whitequark@whitequark.org",
    description="A Python package for interacting with Cypress EZ-USB FX2 series chips",
    long_description="""
The *fx2* Python package allows interacting with Cypress EZ-USB FX2 series microcontrollers.
It provides:

  * *fx2*, a Python library for interacting with the bootloader,
  * *fx2tool*, a tool for programming and debugging the chips.

See the documentation for details.
""",
    license="0-clause BSD License",
    install_requires=["libusb1", "crcmod"],
    packages=find_packages(),
    package_data={"": ["*.ihex"]},
    entry_points={
        "console_scripts": [
            "fx2tool = fx2.fx2tool:main"
        ],
    },
    classifiers=[
        'Development Status :: 4 - Beta',
        'License :: OSI Approved', # ' :: 0-clause BSD License', (not in PyPI)
        'Topic :: Software Development :: Embedded Systems',
        'Topic :: System :: Hardware',
    ],
    cmdclass={
        "build_ext": Fx2BuildExt,
        "bdist_egg": Fx2BdistEgg,
        "sdist": Fx2Sdist,
    },
    project_urls={
        "Documentation": "https://libfx2.readthedocs.io/",
        "Source Code": "https://github.com/whitequark/libfx2",
        "Bug Tracker": "https://github.com/whitequark/libfx2/issues",
    }
)
