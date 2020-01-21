# TP-Link M7350 v5 C++ driver
This is a minimal C++ driver to communicate with TP-Link M7350 v5 modems. It comes with an example program showing how to send SMS from the command line.

Note that it may work with other TP-Link models, but I only have that one to test. Returns on experience (with hardware and firmware versions) are of course welcome.

# Requirements
- RapidJSON - https://rapidjson.org
- CURL - https://curl.haxx.se/libcurl/
- OpenSSL - https://www.openssl.org
- CMake (for compilation, optional) - https://cmake.org
- Doxygen (for docs, optional) - http://doxygen.nl

# Compiling
This can be done easily with CMake using the provided CMakeLists.txt file.
On command line, in source folder, type:
```
 $ mkdir build && cd build
 $ cmake ..
 $ make
```

# Usage of TPLink_M7350 C++ class
For details on class methods and features, see documentation in *doc* folder.

When building a project that requires `tplinkpp`, remember to add it to the list of dependencies.

# Usage of example program
` $ ./send_sms -a modem_address -p password -n phone_number -m message`
