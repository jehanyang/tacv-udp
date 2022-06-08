# Linux UDP :rocket:

We want to use Windows UDP running the Tactilus API to communicate sensor signals to Linux UDP because UDP is fast. Without waiting or buffering with dropped packets, it should also serve our purposes of giving a high frequency of signals that come fairly consistently. 

The code in `tacv-udp/LinuxUDP/` will have the code that is running on Linux to communicate with the Windows code. The Windows code to be run with this code is in `tacv-udp`.

