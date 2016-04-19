/* holtekco2.h -- API to work with Holtek ZyAura CO2 monitor
 *
 * Copyright (C) 2016 Victor Shmoylov <victor.shmoylov@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef HOLTEKCO2_H
#define HOLTEKCO2_H

#include <hidapi.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
    #define CO2_LIB_EXPORT __declspec(dllexport)
#else
    #define CO2_LIB_EXPORT
#endif

#define HOLTEK_CO2_VID 0x04D9 /**< Holtek CO2 Monitor Vendor ID */
#define HOLTEK_CO2_PID 0xA052 /**< Holtek CO2 Monitor Product ID */

#ifdef __cplusplus
extern "C" {
#endif

    typedef hid_device co2_device; ///< type definition for convenience

    struct tag_co2_device_data {
        enum { CO2 = 0x50, TEMP = 0x42, HUMIDITY = 0x44 } ;///< may produce warning, but it's ok
        uint8_t tag;
        uint16_t value;
        uint8_t checksum;
        bool valid;
    };
    typedef struct tag_co2_device_data co2_device_data;


    /** @brief Enumerate all available CO2 Devices.
     *
     *  This is the simple wrapper to HIDAPI's hid_enumerate()
     *  function with CO2 monitor's VID & PID passed to it.
     *
     *  @returns
     *      Pointer to HIDAPI's hid_device_info structure which is
     *      linked list containing information about available devices.
     */
    CO2_LIB_EXPORT struct hid_device_info * co2_enumerate();


    /** @brief Free an enumeration Linked List
     *
     *  This function frees memory allocated to a linked list created by co2_enumerate().
     *  This is just a simple wrapper to HIDAPI's hid_free_enumeration() function.
     *
     *  @param devs Pointer to a list returned from co2_enumerate().
     */
    CO2_LIB_EXPORT void co2_free_enumeration(struct hid_device_info *devs);


    /** @brief Raw open CO2 monitor device by specifying its path.
     *
     *  This function opens certain HID device by specifying its @p path.
     *  Initializing packet is not being sent.
     *  @p Path can be retrieved by calling co2_enumerate() function.
     *  Just like other raw functions related to connect/initialization
     *  this is just a wrapper to the HIDAPI's function hid_open_path().
     *
     *  @param path System-dependent device path. Can be obtained by calling co2_enumerate() function.
     *
     *  @returns
     *      This function returns a pointer to a #co2_device object on
     *      success or NULL on failure.
     */
    CO2_LIB_EXPORT co2_device * co2_raw_open_device_path(const char *path);

    /** @brief Open CO2 monitor device by specifying its path and send initializing packet.
     *
     *  This function opens certain HID device by specifying its @p path.
     *  Then, it initializes usb encode key and sends feature report for device to start the data sending.
     *  @p Path can be retrieved by calling co2_enumerate() function.
     *
     *  @param path System-dependent device path. Can be obtained by calling co2_enumerate() function.
     *
     *  @returns
     *      This function returns a pointer to a #co2_device object on
     *      success or NULL on failure.
     */
    CO2_LIB_EXPORT co2_device * co2_open_device_path(const char *path);


    /** @brief Raw open first avaiable CO2 monitor device.
     *
     *  This function opens first available CO2 device.
     *  Initializing packet is not being sent.
     *  This is a wrapper to hid_open() function.
     *
     *  @returns
     *      This function returns a pointer to a #co2_device object on
     *      success or NULL on failure.
     */
    CO2_LIB_EXPORT co2_device * co2_raw_open_first_device();

    /** @brief Raw open first avaiable CO2 monitor device.
     *
     *  This function opens first available CO2 device.
     *  Then, it initializes usb encode key and sends feature report for device to start the data sending.
     *
     *  @returns
     *      This function returns a pointer to a #co2_device object on
     *      success or NULL on failure.
     */
    CO2_LIB_EXPORT co2_device * co2_open_first_device();


    /** @brief Close CO2 device handle.
     *
     *  This function is a wrapper to hid_close() function.
     *
     *  @param device handler returned by one of co2_open_* functions.
     */
    CO2_LIB_EXPORT void co2_close(co2_device *device);


    /** @brief Generate encryption key.
     *
     *  Generates encryption key and puts it to @p key buffer.
     *  This function generates same values as original chinese software's HIDApi.dll
     *  Actually, encryption key may contain any values, even random. I've developed
     *  that just for completeness and compatibility to original protocol.
     *
     *  @param[out] key Buffer to store generated key
     */
    CO2_LIB_EXPORT void co2_gen_usb_enc_key(uint8_t key[8]);

    /** @brief Send initializing packet to device
     *
     *  Generates encoding key (by calling #co2_gen_usb_enc_key) and sends it to device.
     *  This will init encoding key and causes device to start sending data to host.
     *  If device was already previously initialized, you may need to wait some time(2s)
     *  to new key be applied to newly encoded data.
     *
     *  May be required to be call only when using co2_raw_* functions.
     *
     *  @param[in] device Device handler returned by one of co2_open_* functions.
     */
    CO2_LIB_EXPORT int co2_send_init_packet(co2_device *device);

    /** @brief Decrypt buffer using specified key
     *
     *  Performing decrypting of buffer that was read from device.
     *
     *  @param[in] key Encoding key buffer
     *  @param[in,out] buffer Data buffer, decoded data will be stored here
     */
    CO2_LIB_EXPORT void co2_decrypt_buf(const uint8_t key[8], uint8_t buffer[8]);

    /** @brief Reads/decodes/validates data from device
     *
     *  This function reads data from device, then decodes buffer using global key
     *  that was initialized by co2_send_init_packet or one of co2_open_* functions.
     *  Then, function performs simple validation and parsing of data buffer.
     *
     *  @param[in] device Device handler.
     *
     *  @returns #co2_device_data structure containing data/operation tag, value(as is), checksum and valid flag
     */
    CO2_LIB_EXPORT co2_device_data co2_read_data(co2_device *device);

    CO2_LIB_EXPORT int co2_raw_read_decode_data(co2_device *device, uint8_t buffer[8]);
    CO2_LIB_EXPORT int co2_raw_read_data(co2_device *device, uint8_t buffer[8]);


    CO2_LIB_EXPORT double co2_get_celsius_temp(uint16_t value);
    CO2_LIB_EXPORT double co2_get_fahrenheit_temp(uint16_t value);
    CO2_LIB_EXPORT double co2_get_relative_humidity(uint16_t value);





#ifdef __cplusplus
} //end of extern "C"
#endif

#endif // HOLTEKCO2_H
