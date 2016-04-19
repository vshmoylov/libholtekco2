# ZyAura CO2 monitor library
This is a multiplatform library which allows to initialize device and read data
from ZyAura CO2 monitor device via USB HID interface.

## Building
This library only depends on [hidapi](https://github.com/signal11/hidapi),
which should be downloaded/installed separately.  
Windows build (DLL, mingw32) can be downloaded [here](https://www.dropbox.com/s/3jv2rht2oz84mn6/hidapi-mingw32.zip).

Sorry, Makefile isn't available yet.  
But if you (decided) write in C/C++, I believe you have enough skills to build
this lib or add it to your project.

## Usage
Typically, if you have only one device connected to your system, usage could be following:  
```c
co2_device * devHanler = co2_open_first_device();
//you can wait some time (about 2s) to (re-)initialize device, but it's not required
while (!finishReading) { //usually there's 80+ms delay between device sending data
    co2_device_data data = co2_read_data(devHandler);
    if (data.valid){
        switch (data.tag) {
          case co2_device_data::CO2:
              printf("CO2 concentration: %hd PPM\n", data.value);
              break;
          case co2_device_data::TEMP:
              printf("Ambient temparature: %lf C\n", co2_get_celsius_temp(data.value));
              break;
          case co2_device_data::HUMIDITY:
              printf("Relative humidity: %lf %%\n", co2_get_relative_humidity(data.value)); //or simply divide by 100.0: data.value/100.0
              break;
          default:
              //ignore this, device may send some ghost values
              //looks like some kind of generic framework on this device sending different values
        }
    }
    //some other code
}
co2_close(devHandler);//properly finish work with device
```

If you have more than one device connected to your system, you should
retrieve list of devices by calling `co2_enumerate()` function. It is simple
wrapper to hidapi's `hid_enumerate()` function and returns same linked list structure.  
Then, select device and open it via `co2_open_device_path(device->path)`.

## License
This library is released under [MIT License](https://tldrlegal.com/license/mit-license).  
Information about copying/modifying/using of this code can be found in file named `LICENSE`.  
Please note, this library provided "AS IS".

## Thanks
* [Henryk Plötz](@henryk) (https://hackaday.io/project/5301)
