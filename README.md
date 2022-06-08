# :walking:tacv-udp: A library to use with the Tactilus API 

tacv-udp is a library to use with the Tactilus insole pressure sensor API in order to use two sensors at once; calculate the force/moments from the array of pressure readings that are received using the Tactilus API; and send force, moment, or pressure via UDP from a Windows computer.

## :zap: installation
tacv-udp supports Windows sending and Linux receiving via UDP. This repo is for the Windows portion. 
0. Receive the Tactilus SDK, which contains 2 `.dll` files, 1 `.lib` file, a `.h` file which describes the original available functions and their uses, and a simple `test.cpp` file for testing the library. 
1. First install Visual Studio for Windows.
2. Then run this in `cmd` terminal in the folder you want. 
```
git clone git@github.com:jehanyang/tacv-udp.git
cd tacv-udp/SDK
cmake CMakeLists.txt -G "Visual Studio [your version here]" -A Win32
```
This generates all the Visual Studio project builds.

3. Open `SensorProd.sln`. 

4. Optional: Put in "Release" mode if you want it to run fast.

5. In right sidebar, first double click `testTwoSensors`. Then `Build->Build Solution` and the code will run.

6. At the bottom of VS you should see it say succeed. It will drop it into a "Release" folder, but because the libraries need to be in the same folder, just cut and paste it into the root (I'm sure you can manually specify the build location somewhere to not have to do this every time)

7. You can then open a `cmd` terminal, navigate to the folder, and just type `testTwoSensors.exe` to run it.
