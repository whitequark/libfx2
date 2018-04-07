from os import path

from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext
from setuptools.command.bdist_egg import bdist_egg

from distutils.spawn import spawn
from distutils.dir_util import mkpath


class Fx2BuildExt(build_ext):
    def run(self):
        firmware_dir = path.join("..", "firmware")
        spawn(["make", "-C", path.join(firmware_dir, "library")], dry_run=self.dry_run)
        spawn(["make", "-C", path.join(firmware_dir, "bootloader")], dry_run=self.dry_run)

        bootloader_ihex = path.join("..", "firmware", "bootloader", "build", "bootloader.ihex")
        self.copy_file(bootloader_ihex, "fx2")


class Fx2BdistEgg(bdist_egg):
    def run(self):
        # Allow installing as a dependency via pip.
        self.run_command("build_ext")
        bdist_egg.run(self)


setup(
    name="fx2",
    version="0.2",
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
    install_requires=["libusb1"],
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
    },
    project_urls={
        "Documentation": "https://libfx2.readthedocs.io/",
        "Source Code": "https://github.com/whitequark/libfx2",
        "Bug Tracker": "https://github.com/whitequark/libfx2/issues",
    }
)
