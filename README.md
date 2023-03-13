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
 $ sudo make install
```
To build documentation, add option `-DBUILD_DOC=1` to `cmake`.

## Differences between older firmwares and latest version
As of firmware version M7350(EU)_V5_201019, TP-Link has introduced some sort of encryption to comply with GDPR. While being relatively insecure (see (this)[https://hex.fish/2021/05/10/tp-link-gdpr/] for an overview), it nonetheless breaks compatibility with older versions. The data format remains unchanged, but is now encrypted with AES and signed with RSA. The encryption/decryption method seems to be the same between models with different data formats.

I implemented the appropriate routines to encrypt/decrypt messages. Since I don't currently have my M3750 at hand, I didn't test them with an actual device. Therefore, the new code is not activated by default. If you want to compile for the latest version, add option `-DNEW_FIRMWARE=1` to `cmake`.

# Usage of TPLink_M7350 C++ class
For details on class methods and features, see documentation in *doc* folder.

To use any of the methods transferring data to the modem, you must first devise yourself what the required fields are. For instance, if you want to set LAN settings, you need to populate a JSON object to feed the `set_lan_settings` method. You can use the `get_lan_settings` method to obtain what the modem would expect.

When building a project that requires `tplinkpp`, remember to add it to the list of dependencies. Header files are installed in the subdirectory `tplinkpp` of the *include* folder.

# Usage of example program
` $ ./send_sms -a modem_address -p password -n phone_number -m message`
