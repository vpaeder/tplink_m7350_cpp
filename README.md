# TP-Link M7350 v5 C++ driver
This is a minimal C++ driver to communicate with TP-Link M7350 v5 modems. It comes with an example program showing how to send SMS from the command line.

# Requirements
- RapidJSON - https://rapidjson.org
- CURL - https://curl.haxx.se/libcurl/
- OpenSSL - https://www.openssl.org
- CMake (for compilation, optional) - https://cmake.org
- Doxygen (for docs, optional) - http://doxygen.nl

# Compilation
This can be done easily with CMake using the provided CMakeLists.txt file.
On command line, in source folder, type:
```
 $ mkdir build && cd build
 $ cmake ..
 $ make
```

# Usage of TPLink_M7350 C++ class
At the moment I didn't write a compilation script to produce a library. Therefore one must take care of including the header and the cxx file in one's own source tree.
For details on class methods and features, see documentation in *doc* folder.

# Usage of example program
` $ ./send_sms -a modem_address -p password -n phone_number -m message`
