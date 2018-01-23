from setuptools import setup, find_packages, Command
import os

setup(
    name="fx2",
    version="0.1",
    author="whitequark",
    author_email="whitequark@whitequark.org",
    url="https://github.com/whitequark/libfx2",
    description="A Python library for interacting with Cypress EZ-USB FX2 series chips",
    #long_description=open("README.md").read(),
    license="0-clause BSD License",
    install_requires=["libusb1"],
    packages=find_packages(),
    entry_points={
        "console_scripts": [
            "fx2tool = fx2.fx2tool:main"
        ],
    },
    classifiers=[
        'Development Status :: 4 - Beta',
        'License :: OSI Approved :: 0-clause BSD License',
        'Topic :: Software Development :: Embedded Systems',
        'Topic :: System :: Hardware',
    ]
)
