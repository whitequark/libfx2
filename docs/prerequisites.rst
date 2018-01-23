Prerequisites
=============

*libfx2* has the following dependencies:

  * make and sdcc_ for building the firmware code,
  * `Python 3.6+ <python_>`_ and `Python libusb1 wrapper <python-libusb1_>`_ for interacting with the device.

On a Debian system, these can be installed with:

.. code-block:: sh

    apt-get install make sdcc python3 python3-libusb1

Then, compile all firmware components:

.. code-block:: sh

    make -C firmware

Then, install the Python components to ``~/.local``:

.. code-block:: sh

    python3 software/setup.py develop --user

The last step is not required if you just want to upload example firmware.

.. _sdcc: http://sdcc.sourceforge.net
.. _python: https://www.python.org/
.. _python-libusb1: https://pypi.python.org/pypi/libusb1
