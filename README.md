# libfx2

_libfx2_ is a chip support package for Cypress EZ-USB FX2 series microcontrollers.

Build examples to play with this library

```
cd libfx2
make -C firmware
make -C examples
```

Install EZ-USB FX3 SDK and open Cypress USBSuite 
<img width="698" alt="image" src="https://github.com/sidd-kishan/libfx2/assets/1007208/e643e66e-d809-42ee-bf2b-af4cf82ac621">

Every example has a build folder
<img width="371" alt="image" src="https://github.com/sidd-kishan/libfx2/assets/1007208/f883442e-ea83-4232-a35f-fc862782bf5d">

Every build folder has a .ihex file for example in blinky 
<img width="557" alt="image" src="https://github.com/sidd-kishan/libfx2/assets/1007208/cb78498a-bcf9-4768-b952-763a4a72a929">

just rename to .hex that can be programmed in on the RAM as example in blinky
<img width="578" alt="image" src="https://github.com/sidd-kishan/libfx2/assets/1007208/b5eca1b3-1b00-458f-9869-c98a22f04e47">


Program the RAM to start the demo code autmatically 
<img width="502" alt="image" src="https://github.com/sidd-kishan/libfx2/assets/1007208/b56dacf6-74f9-418a-b87d-26fd2a582f5f">


Select the hex file to program
<img width="414" alt="image" src="https://github.com/sidd-kishan/libfx2/assets/1007208/e84252fe-373c-4f2c-90d0-ef7e11b672c1">

will find the status programmin succeeded
<img width="506" alt="image" src="https://github.com/sidd-kishan/libfx2/assets/1007208/123357ad-79f6-4b82-a996-6a1f87334b16">


See the [complete documentation](https://libfx2.readthedocs.io) for details.

## License

[0-clause BSD](LICENSE-0BSD.txt)
