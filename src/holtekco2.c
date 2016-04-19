/* holtekco2.c -- Implementation of API to work with Holtek ZyAura CO2 monitor
 *
 * Copyright (C) 2016 Victor Shmoylov <victor.shmoylov@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <holtekco2.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

static uint8_t key[8] = {0};//bad idea with global var, also it isn't thread-safe

CO2_LIB_EXPORT struct hid_device_info * co2_enumerate() {
    return hid_enumerate(HOLTEK_CO2_VID, HOLTEK_CO2_PID);
}

CO2_LIB_EXPORT void co2_free_enumeration(struct hid_device_info *devs){
    hid_free_enumeration(devs);
}

CO2_LIB_EXPORT co2_device * co2_raw_open_device_path(const char *path){
    return hid_open_path(path);
}

CO2_LIB_EXPORT co2_device * co2_open_device_path(const char *path){
    co2_device *dev = hid_open_path(path);
    if (dev!=NULL){//send initializing packet
        int res = co2_send_init_packet(dev);
        if (res != 9){//something went wrong
            hid_close(dev);
            return NULL;
        }
    }
    return dev;
}


CO2_LIB_EXPORT co2_device * co2_raw_open_first_device(){
    return hid_open(HOLTEK_CO2_VID, HOLTEK_CO2_PID, NULL);
}

CO2_LIB_EXPORT co2_device * co2_open_first_device(){
    co2_device *dev = hid_open(HOLTEK_CO2_VID, HOLTEK_CO2_PID, NULL);
    if (dev!=NULL){//send initializing packet
        int res = co2_send_init_packet(dev);
        if (res != 9){//something went wrong
            hid_close(dev);
            return NULL;
        }
    }
    return dev;
}

CO2_LIB_EXPORT void co2_close(co2_device *device){
    hid_close(device);
}

CO2_LIB_EXPORT void co2_gen_usb_enc_key(uint8_t usb_enc_key[8]){ //should be same as ZG.exe software generates
    uint8_t next_month;
    uint8_t time_sec;
    time_t current_time;
    struct tm *local_time;

    time(&current_time);
    local_time = localtime(&current_time);//return value points to internal var, so no need to free mem

    usb_enc_key[0] = local_time->tm_mday + local_time->tm_sec + 66; //First byte of struct is the addition month to current time's seconds + 66
    usb_enc_key[1] = ((uint16_t)((uint16_t)(local_time->tm_year) + 1900) >> 8) - 104;
    usb_enc_key[2] = local_time->tm_hour + local_time->tm_min + 90;//hour + min + 90
    usb_enc_key[3] = 8 * local_time->tm_sec - 34;//8*sec-34
    usb_enc_key[4] = local_time->tm_min - 60;//min-60
    usb_enc_key[5] = local_time->tm_year + 108;//year + 108
    usb_enc_key[6] = 4 * (local_time->tm_sec + 51); //4*sec+51
    next_month = local_time->tm_mon + 1;//next month
    time_sec = local_time->tm_sec;//
    time_sec = next_month + time_sec;
    time_sec = time_sec - 95;
    usb_enc_key[7] = time_sec;
    usb_enc_key[0] = time_sec ^ usb_enc_key[5] ^ usb_enc_key[2];
    time_sec = usb_enc_key[2] ^ usb_enc_key[6] ^ time_sec;
    usb_enc_key[1] = time_sec;
    usb_enc_key[2] = usb_enc_key[5] ^ usb_enc_key[6] ^ time_sec;
    time_sec = usb_enc_key[6] ^ usb_enc_key[3];
    time_sec = usb_enc_key[4] ^ time_sec;
    usb_enc_key[3] = time_sec;
    next_month = usb_enc_key[5] ^ time_sec ^ usb_enc_key[0];
    usb_enc_key[5] = usb_enc_key[2] ^ usb_enc_key[0] ^ usb_enc_key [7];
    usb_enc_key[4] = next_month;
    usb_enc_key[6] = next_month;
    usb_enc_key[7] = next_month ^ usb_enc_key[0] ^ usb_enc_key[3];
}

CO2_LIB_EXPORT int co2_send_init_packet(co2_device *device){
    uint8_t enc_key_buf[9]={0};
    co2_gen_usb_enc_key(key);
    memcpy(enc_key_buf+1, key, 8);//first byte should be 0x00
    return hid_send_feature_report(device, enc_key_buf, 9);
}

CO2_LIB_EXPORT void co2_decrypt_buf(const uint8_t key[8], uint8_t buffer[8]){
    uint8_t salt[8] = { 0x48,  0x74,  0x65,  0x6D,  0x70,  0x39,  0x39,  0x65 }; //Htemp99e
    uint8_t shuffle[8] =  { 2, 4, 0, 7, 1, 6, 5, 3 };

    int i;
    uint8_t phase1[8] = {0};
    for (i=0; i<8; i++){//phase1: shuffle
        phase1[shuffle[i]]=buffer[i];
    }

    uint8_t phase2[8] = {0};
    for (i=0; i<8; i++){ //phase2: XOR
        phase2[i]=phase1[i]^key[i];
    }

    uint8_t phase3[8] = {0};
    for (i=0; i<8; i++){ //phase3: shift
        phase3[i] = ( (phase2[i] >> 3) | (phase2[ (i-1+8)%8 ] << 5) ) & 0xff;
    }

    uint8_t ctmp[8] = {0};
    for (i=0; i<8; i++){
        ctmp[i] = ( (salt[i] >> 4) | (salt[i]<<4) ) & 0xff;
    }

    for (i=0; i<8; i++){
        buffer[i] =  (0x100 + phase3[i] - ctmp[i]) & 0xff;
    }
}

CO2_LIB_EXPORT co2_device_data co2_read_data(co2_device *device){
    co2_device_data result = {0};
    uint8_t buf[8] = {0};
    int res = hid_read(device, buf, 8);
    if (res == 8){
        co2_decrypt_buf(key, buf);
        bool valid = true;
        valid &= buf[4]==0x0d;
        uint8_t sum = buf[0] + buf[1] + buf[2];
        valid &= sum==buf[3];

        result.tag = buf[0];
        result.value = ((uint16_t)buf[1])<<8|buf[2];
        result.checksum = buf[3];
        result.valid = valid;

//        fprintf(stderr, "%02hhx %02hhx %02hhx %02hhx %02hhx\n", buf[0],buf[1],buf[2],buf[3],buf[4]); //debug output
//        fprintf(stderr, "Tag: %02hhx; Value: %04hx\n",  result.tag, result.value);                   //debug output
    }
    return result;
}

CO2_LIB_EXPORT int co2_raw_read_decode_data(co2_device *device, uint8_t buffer[8]){
    int res = hid_read(device, buffer, 8);
    co2_decrypt_buf(key, buffer);
    return res;
}

CO2_LIB_EXPORT int co2_raw_read_data(co2_device *device, uint8_t buffer[8]){
    return hid_read(device, buffer, 8);
}

CO2_LIB_EXPORT double co2_get_celsius_temp(uint16_t value){
    return (double)value/16.0-273.15;
}

CO2_LIB_EXPORT double co2_get_fahrenheit_temp(uint16_t value){
    return ((double)value/16.0-273.15)*1.8 + 32;
}

CO2_LIB_EXPORT double co2_get_relative_humidity(uint16_t value){
    return (double)value/100.0;
}

#ifdef __cplusplus
} //end of extern "C"
#endif
