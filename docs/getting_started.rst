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

Consider the code of the bootloader if you want to see how simple USB functionality can be implemented:

.. literalinclude:: ../firmware/bootloader/main.c
   :caption: main.c
   :language: c

.. literalinclude:: ../firmware/bootloader/Makefile
   :caption: Makefile
   :language: make
