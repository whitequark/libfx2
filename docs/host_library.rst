Host-side library reference
===========================

.. automodule:: fx2

   .. autoclass:: FX2Config

      .. automethod:: append
      .. automethod:: encode
      .. automethod:: decode

   .. autoclass:: FX2Device

      .. automethod:: control_read
      .. automethod:: control_write
      .. automethod:: bulk_read
      .. automethod:: bulk_write
      .. automethod:: create_poller
      .. automethod:: get_poller
      .. automethod:: poll

      .. automethod:: read_ram
      .. automethod:: write_ram
      .. automethod:: load_ram
      .. automethod:: cpu_reset
      .. automethod:: read_boot_eeprom
      .. automethod:: write_boot_eeprom
      .. automethod:: reenumerate

   .. autoexception:: FX2DeviceError

.. automodule:: fx2.format

   .. autofunction:: autodetect
   .. autofunction:: input_data
   .. autofunction:: output_data
   .. autofunction:: flatten_data
   .. autofunction:: diff_data
