Getting started
===============

Read configuration EEPROM:

.. code-block:: sh

  fx2tool -S firmware/bootloader/bootloader.ihex read_eeprom 0 7

Load the Blinky example (a LED should be attached to PA0):

.. code-block:: sh

  make -C firmware/blinky load

Blinking a LED
--------------

Read the code for the *blinky* example if you're looking for something minimal:

.. literalinclude:: ../firmware/blinky/main.c
   :caption: main.c
   :language: c

.. literalinclude:: ../firmware/blinky/Makefile
   :caption: Makefile
   :language: make

Interacting over USB
--------------------

Consider the code of the Cypress bootloader if you want to see how simple USB functionality can be implemented:

.. literalinclude:: ../firmware/boot-cypress/main.c
   :caption: main.c
   :language: c

.. literalinclude:: ../firmware/boot-cypress/Makefile
   :caption: Makefile
   :language: make

Adding an DFU bootloader
------------------------

It is easy to integrate a standards-compliant and OS-agnostic Device Firmware Upgrade bootloader as *libfx2* provides all necessary infrastructure, and it only needs to be configured for a specific board and integrated into a target application:

.. literalinclude:: ../firmware/boot-dfu/main.c
   :caption: main.c
   :language: c

.. literalinclude:: ../firmware/boot-dfu/Makefile
   :caption: Makefile
   :language: make

The DFU images suitable for flashing can be generated from Intel HEX firmware images using
the ``dfu`` subcommand of the :ref:`command-line tool <bootloader-tool>`.

Adding an UF2 bootloader
------------------------

It is easy to integrate a very versatile and OS-agnostic `UF2 compliant <uf2_>`_ bootloader as *libfx2* provides all necessary infrastructure, and it only needs to be configured for a specific board and integrated into a target application:

.. _uf2: https://github.com/Microsoft/uf2

.. literalinclude:: ../firmware/boot-uf2/main.c
   :caption: main.c
   :language: c

.. literalinclude:: ../firmware/boot-uf2/Makefile
   :caption: Makefile
   :language: make

The UF2 images suitable for flashing can be generated from Intel HEX firmware images using
the ``uf2`` subcommand of the :ref:`command-line tool <bootloader-tool>`.
