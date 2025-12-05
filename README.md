# Metratec UHF Readers SDK C {#mainpage}

Public SDK to use Metratec UHF RFID-Readers from C code

[Metratec Homepage](https://www.metratec.com)

## Building the documentation

- Call ```doxygen Doxyfile```
- Find the resulting documentation in folder 'docs'

## Building the library and example projects (for PLRM)

- Call ```cmake -B build -S ./ -DCMAKE_BUILD_TYPE=Release -DMT_UHF_DEVICE_TYPE=PLRM -DEXAMPLE_POSIX_BASIC=1```
- Call ```make -C build/ mt_uhf_sdk_c``` to only build the library
  - Generated library file is build/code/libmt_uhf_sdk_c.a
- Call ```make -C build/ sample_posix_basic_PLRM``` to build the basic example
  - Generates executable POSIX into build/examples/posix_basic

If you have a different device type change the value to match yours.

The following device types are currently supported:

- PLRM
- QRG2_ETSI
- QRG2_FCC

The following devices are not officially supported but will likely work. They need to be added to the list 
of supported devices in CMakeLists.txt first:

- DESKID_UHF_V2_ETSI
- DESKID_UHF_V2_FCC

## Start your own project

### Basic

If CMake is used in the users project the library can be included in build as part of the project. 
If not build the static library as stated above for the device used and add the library to your project. 
Use ```make -C build/ mt_uhf_sdk_c``` to make only the library without the example code.

The library is not compiler specific and doesn't use compiler specific extensions. 
It is tested with gcc. The example posix code uses c99 and is tested with gcc, too.

To access the libraries functions add the uhf_reader_sdk.h
```
#include <metratec/uhf_reader_sdk.h>
```
This gives access to all API functions. 

If memory is an issue or big amounts of tag data is handled there are settings for tags, parser and receive buffer sizes. They are set in the
CMake to default values and can be changed by giving them to CMake -D[]

### Extern functions (HAL)

The library relies on a number of functions to do it's work. These can be found in 
code/include/uhf_reader/hal.h

- uint32_t mt_rfid_reader_get_time(void);
  - extern supplied (by the user)
  - needs to return a monoton time that should tick up by 1 per millisecond
- int mt_rfid_reader_tx(const uint8_t *data, size_t data_len);
  - extern supplied (by the user)
  - sending data, return the number of bytes sent (or negative value as error)
- void mt_cmd_wait(uint32_t time_ms);
  - A waiting function
  - Used so polling mode won't take all the processor
  - if it returns earlier this won't be a problem, to the extreme of bare metal just returning
- int mt_rfid_reader_rx(void *data, size_t data_len);
  - provided by the library
  - The function to put received data to
- int mt_rfid_reader_rx_remaining_empty(void);
  - Gives the amount of data that can be pushed into mt_rfid_reader_rx()
  - This allows the user to only need a bit of local stack buffer in the receiver

### Init

- Prepare the communication interface and everything else the host system needs
- if the device header include is not handled by CMake include the matching device header file
- call ```uhf_init(...)``` with parameters
  - unknown_frame_cb is a function to call if any data is received that's false formated (this can be logged and used for debugging)
  - polling_cb is a function to call to get more data. If this is NULL the user has to use ```mt_rfid_reader_rx()``` to push the data
    from a different thread or ISR
  - blocking_cb is called if the SDK is in a blocking state so the user might do things during this periods
    especially if there is no polling_cb set
- Call struct mt_uhf_reader_identification *id = mt_uhf_get_identification();
  to get the information (like firmware revision) from the device
- Use setting functions as needed, for example 
  - mt_uhf_set_multiplex_antennas()
  - mt_uhf_set_power()
  - mt_uhf_set_q_value()
  - mt_uhf_set_inventory_mask()

### The first inventory  
- Set up an inventory buffer or an inventory callback
- Call an inventory function, usually start with ```mt_uhf_inventory()```
- Printout the inventory data from the callback or the buffer (or both) the way your system supports
