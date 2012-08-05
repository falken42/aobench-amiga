This is a port of aobench ( http://code.google.com/p/aobench/ ) made to run on Amiga hardware.  aobench is a small ambient occlusion renderer for benchmarking real world floating point performance in various languages.  Please see the project page for more details.

The code here utilizes the WinUAE Demo ToolChain in order to build on Windows.  You will need both that toolchain, and the patches for the toolchain (available at https://github.com/bpoint/TSSWUAETCFADP-patches -- see installation instructions there) in order to build this code.

If you just want the aobench.exe, it is located in the root with a size of 3,968 bytes.  Workbench 3.1 is known to work properly on a real Amiga 1200.  Earlier versions might work too, but has not been tested.

Benchmarks:
-----------
Amiga 1200 (301): 2107sec / 35m7s

![Rendering](http://bpoint.github.com/aobench-amiga/aobench-amiga1200-301-rendering.jpg)

https://twitter.com/bpoint42/status/211416863727550464

![Time Result](http://bpoint.github.com/aobench-amiga/aobench-amiga1200-301-time.jpg)

https://twitter.com/bpoint42/status/212229752034045952

Amiga 1200 (Goatfather): 2406sec / 40m6s

![Rendering](http://bpoint.github.com/aobench-amiga/aobench-amiga1200-goatfather-rendering-small.jpg)

![Time Result](http://bpoint.github.com/aobench-amiga/aobench-amiga1200-goatfather-time-small.jpg)

WinUAE in A1200 mode: 3114sec / 51m54s

WinUAE in AROS mode: ~6sec

How to build and run:
---------------------
1) Download The Super Simple WinUAE Tool Chain For Amiga Demo Programmers (TSSWUAETCFADP) Version 4 from: http://pouet.net/prod.php?which=58703 , and patch the toolchain according to the instructions in the patches repository above.

2) Overwrite the unpacked root with the files from this repository.

3) Run the 'build-aobench.bat' file to build the code.  If everything went well, you will see a few warnings but 'Build success' will be shown.  The output 'mydemo.exe' will be located in the DH0 folder.

4) Run the 'RunAros.bat' file to execute aobench at full speed within WinUAE.  For (approximate) Amiga 1200 speeds, you will first need to run WinUAE and modify the A1200 profile to include an FPU (68881).  After saving the configuration, run the 'runa1200.bat' file.

5) Once rendering is completed, the code will wait until a mouse button is pressed.  After a button is pressed, the total rendering time (in seconds) will be displayed in the console.  The benchmark does not include the time waiting for mouse input.

If you would like the code to output a .ppm of the final framebuffer, uncomment the WRITE_PPM_OUTPUT #define, recompile, and it will be written to the root of DH0 by default.

Thanks/greets:
--------------
Big thanks to both rale/Traction and FishGuy/brainstorm for helping me get started with Amiga coding, and also to both 301 and Goatfather for providing their time to test my code on real hardware!  And one more thanks to Syoyo for creating the interesting aobench project.

Greets fly out to all of the other brainstormers!
