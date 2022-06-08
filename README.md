# :walking:tacv-udp: A library to use with the Tactilus API 

tacv-udp is a library to use with the Tactilus insole pressure sensor API in order to use two sensors at once; calculate the force/moments from the array of pressure readings that are received using the Tactilus API; and send force, moment, or pressure via UDP from a Windows computer.

## :zap: installation
tacv-udp supports Windows sending and Linux receiving via UDP. This repo is for the Windows portion. 

0. Receive the Tactilus SDK, which contains 2 `.dll` files, 1 `.lib` file, a `.h` file which describes the original available functions and their uses, and a simple `test.cpp` file for testing the library. 
1. First install Visual Studio for Windows and Git Bash. 
2. Then run this in `git bash` terminal in the folder you want which also has the Tactilus SDK files. 
```
git clone git@github.com:jehanyang/tacv-udp.git
cd tacv-udp/
cmake CMakeLists.txt -G "Visual Studio [your version here]" -A Win32
```
This generates all the Visual Studio project builds.

3. Open `SensorProd.sln`. 

4. Optional: Put in "Release" mode if you want it to run fast.

5. In right sidebar, first double click `testTwoSensors`. Then `Build->Build Solution` and the code will build.

6. At the bottom of VS you should see it say succeed. It will drop it into a "Release" folder, but because the libraries need to be in the same folder, just cut and paste it into the root (I'm sure you can manually specify the build location somewhere to not have to do this every time)

7. Connect the Windows computer to the Linux computer via Ethernet cable (USB can also work). On Windows: 

`View Network Connections >> Right Click Ethernet 1>> Properties >> ipv4 than change the IP address to 10.7.0.1, subnet mask to 255.255.0.0, default dns address to 8.8.8.8`

On Linux set the Ethernet static IP address as 10.7.0.11. On Debian you would do this:
```
$ sudo vim /etc/network/interfaces
$  # add the following code block or adjust the already existent eth0 interface:
$  # auto eth0
$  # iface eth0 inet static
$  #     address 10.7.0.11/24
$  #     gateway 10.7.0.1
$ ifdown eth0
$ ifup eth0
```

8. You can then open a `cmd` terminal, navigate to the folder, and just type `testTwoSensors.exe` to run it.
