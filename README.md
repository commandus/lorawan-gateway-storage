# LoRaWAN gateway storage

This is a simple example of a gateway database service for a tlns project.

## Requisites

### Linux

- CMake or Autotools/Automake
- C++ compiler and development tools bundle

### Windows

- CMake
- Visual Studio
- vcpkg

### ESP32

- SDK idf tools or
- Visual Studio Code with Espressif IDF plugin

#### Tools

Make sure you have automake or CMake installed:

```
apt install autoconf libtool build-essential 
```

or 

```
apt install cmake build-essential 
```

## Build

- autotools
- CMake

### Autotools

First install dependencies (see below) and then configure and make project using Autotools:

```
cd lorawan-storage
./autogen.sh
./configure
make
```

Set ./configure command line options:

- --enable-libuv use libuv
- --enable-debug debug print on
- --enable-gen enable key generator
- --enable-sqlite enable SQLite3 backend
- --enable-ipv6 enable IPv6 (reserved for future use)
- --enable-tests enable tests

For instance

```
cd lorawan-storage
./autogen.sh
./configure --enable-sqlite=yes --enable-gen=no 
make
```

enables SQLite backend

### CMake

Configure by default: 
```
cd lorawan-storage
mkdir build
cd build
cmake ..
make
```

Enable/disable options:
```
cd lorawan-storage
mkdir build
cd build
cmake -DENABLE_LIBUV=off -DENABLE_DEBUG=off -DENABLE_GEN=on -DENABLE_SQLITE=off -DENABLE_IPV6=off ..
make
```

Options are:

- -DENABLE_LIBUV use libuv
- -DENABLE_DEBUG debug print on
- -DENABLE_SQLITE enable SQLite backend
- -DENABLE_GEN enable key generator
- -DENABLE_IPV6 enable IPv6 (reserved for future use)

#### clang instead of gcc

Export CC and CXX environment variables points to specific compiler binaries.
For instance, you can use Clang instead of gcc:

```
cd lorawan-gateway-storage
mkdir build
cd build
export CC=/usr/bin/clang;export CXX=/usr/bin/clang++;cmake ..
export CC=/usr/bin/clang;export CXX=/usr/bin/clang++;cmake -DENABLE_WS=on -DENABLE_JWT=on -DENABLE_PKT2=on -DENABLE_MQTT=on -DENABLE_LOGGER_HUFFMAN=on ..
make
```

### Windows

- Visual Studio with C++ profile installed
- CMake
- vcpkg

### ESP32

SDK

Install:
```
cd ~/esp/esp-idf
./install.sh
```

Set up environment
```
cd ~/esp/esp-idf
. ./export.sh
```

Build

In "idf.py menuconfig" step go to 

- Compiler options, Optimization Level, select "Optimize for size (-Os)"
- Component config, Log output, Default log verbosity, Select "No output"
- Component config, ESP-MQTT Configurations, disable all
- LoRaWAN storage, set settings

Save sdkconfig file and quit.

```
cd ~/src/lorawan-storage
idf_get
idf.py fullclean
idf.py menuconfig
idf.py build
...
Total sizes:
Used static DRAM:   35532 bytes ( 145204 remain, 19.7% used)
      .data size:   13188 bytes
      .bss  size:   22344 bytes
Used static IRAM:   80562 bytes (  50510 remain, 61.5% used)
      .text size:   79535 bytes
   .vectors size:    1027 bytes
Used Flash size :  857611 bytes
           .text:  679975 bytes
         .rodata:  177380 bytes
Total image size:  951361 bytes (.bin may be padded larger)```
```

Flash
```
idf.py flash -p /dev/ttyUSB0
```
Visual Studio Code
- Press F1; select ESP-IDF: Set Espressif device target; select lorawan-storage; select ESP32; select ESP32 chip (via ESP USB bridge)
- Press F1; select ESP-IDF: Build your project
- Press F1; select ESP-IDF: Flush your project

### Dependencies

- gettext (except ESP32)
- libuv (optional)

```
sudo apt install libuv1-dev gettext 
```

## Usage

Executables:

- lorawan-service
- lorawan-query
- lorawan-query-direct
- lorawan-print

Static library:

-liblorawan.a

### lorawan-service

### lorawan-query

Manipulate device records by commands:

- address \<identifier\>
- identifier \<address\>
- assign \{\<record\>\}
- list
- remove \<address> | \<identifier\>

record is a comma-separated string consists of

- address
- activation type: ABP or OTAA
- device class: A, B or C
- device EUI identifier (up to 16 hexadecimal digits, 8 bytes)
- nwkSKey- shared session key, 16 bytes
- appSKey- private key, 16 bytes
- LoRaWAN version, e.g. 1.0.0
- appEUI OTAA application identifier
- appKey OTAA application private key
- nwkKey OTAA network key
- devNonce last device nonce, 2 bytes
- joinNonce last Join nonce, 3 bytes
- device name (up to 8 ASCII characters)

e.g.
```
aabbccdd,OTAA,A,2233445566778899,112233445566778899aabbccddeeff00,55000000000000000000000000000066,1.0.0,ff000000000000ff,cc0000000000000000000000000000dd,77000000000000000000000000000088,1a2b,112233,DeviceNam
```
is a record of device with assigned address 'aabbccdd'. 

Create an empty record with reserved address 11aa22bb: 

```
./lorawan-query assign 11aa22bb
```

In the example above all properties except address skipped.  

Query device record by address:
```
./lorawan-query identifier aabbccdd
```

Remove record by address: 
```
./lorawan-query remove aabbccdd
```

List records
```
./lorawan-query list
```

Manipulate gateway records by commands:

- gw-address \<identifier\>
- gw-identifier \<address\> 
- gw-assign \{\<address> \<identifier\>\}
- gw-list
- gw-remove \<address\> | \<identifier\> 

gw-remove remove record by address or identifier

Options:

- -c code (account#)
- -a access code hexadecimal 64-bit number e.g. 2A0000002A
- -s service address:port e.g. 10.2.104.51:4242
- -o offset default 0. Applicable for gw-list only
- -z size default 10. Applicable for gw-list only

Examples:

```
./lorawan-query gw-assign 11 1.2.3.4:5 12 1.2.3.4:5 13 1.2.3.4:5 14 1.2.3.4:5 15 1.2.3.4:5 16 1.2.3.4:5 17 1.2.3.4:5 18 1.2.3.4:5 19 1.2.3.4:5 20 1.2.3.4:5 21 1.2.3.4:5 22 1.2.3.4:5 23 1.2.3.4:5 24 1.2.3.4:5 25 1.2.3.4:5 26 1.2.3.4:5 27 1.2.3.4:5 28 1.2.3.4:5 29 1.2.3.4:5 30 1.2.3.4:5 -v
./lorawan-query gw-list
./lorawan-query gw-list -o 9
./lorawan-query gw-remove 12
./lorawan-query gw-list
./lorawan-query gw-address 11
./lorawan-query gw-address 1.2.3.4:5
./lorawan-query gw-identifier 1.2.3.4:5
```

### lorawan-query-direct

lorawan-query-direct dynamically load shared libraries with exported function like:

```
IdentityService* makeMemoryIdentityService();
GatewayService* makeMemoryGatewayService();
```
where Memory is a "class name" (actually name must also have "make" prefix and "IdentityService" or "GatewayService" suffix).

IdentityService and GatewayService are interfaces to work with storage.

Plugin files are:

- storage-json (libstorage-json.so or libstorage-json.DLL)
- storage-gen (libstorage-gen.so or libstorage-gen.DLL)
- storage-mem (libstorage-mem.so or libstorage-mem.DLL)
- storage-sqlite (libstorage-sqlite.so or libstorage-sqlite.DLL)
 
Option -s set plugin file name and class prefix for device identities and gateways e.g.

- -s storage-json:Json
- -s storage-gen:Gen:Memory
- -s storage-mem:Memory
- -s storage-sqlite:Sqlite (if configured with ENABLE_SQLITE option)

where storage-gen translated to libstorage-gen.so or libstorage-gen.DLL name according to system name considerations.

There are same statically linked plugins:

- -s json
- -s gen
- -s mem
- -s sqlite (if configured with ENABLE_SQLITE option)

Because plugin use direct calls there no --code --accesscode options available.

### lorawan-print

```
./lorawan-print -p json 4030034501807b000239058672800d394af6863bf99148f63bec91543c086c171be37f3953
./lorawan-print -p json 4030034501807d00029139bff7333583847518599d50e3900b53d0e64c0d9eabda8ebc2aca
```

### Static library usage examples

There are some example source code in the examples/ subdirectory:

- example-mem.cpp in-memory device storage
- example-gen.cpp device generator based on pass phrase 
- example-gw-mem.cpp gateway in-memory storage

shows how to compile source with liblorawan.a static library, for instance using gcc:

```
g++ -o example-direct -I.. example-direct.cpp -L../cmake-build-debug -llorawan
```

where -L../cmake-build-debug is a path to the directory where liblorawan.a resides. 

Another sample code 

- example-direct.cpp

shows how to choose different storages: "gen", "mem", "sqlite" in your code.

## Tests

### ESP32 

Connect ESP32 to your PC, open Visual Studio Code, Press F1, Select "ESP-IDF: Monitor your device"

Wait for message:

```
I (4488) lorawan-storage: 10.2.104.109:4242 master key: masterkey, net: 0, code: 42, access code: 42
```

Open console in PC. Run:

```
./lorawan-query list -s 10.2.104.109:4242
```


### Test gateway storage

UDP
```
echo '4c0000002a000000000000002a000000000a' | xxd -r -p | nc -u 127.0.0.1 4244 -w 1 | xxd -p
```

TCP 
```
echo '4c0000002a000000000000002a000000000a' | xxd -r -p | nc 127.0.0.1 4244 -w 1 | xxd -p
```
