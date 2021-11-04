# ITSCAM Protocol to WebService REST JSON

This plugin will connects directly on ITSCAM protocol port (50.000) on ITSCAM 600 and will forward all recognitions received using the standard ITSCAM protocol to a WebService REST request using JSON payload/body.

## Build

This plugin was implemented using C and C++ language and it will compile using CMake/Make inside the itscam600-plugin-builder (see the step by step to generate the itscam600-plugin-builder on main README).

The development environment is a Linux based and we already created a base build script and you can build the plugin using the follow command:

```bash
./build.sh
```
