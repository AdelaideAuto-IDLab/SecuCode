/*
 * crc64.c
 *
 *  Created on: 2019Äê5ÔÂ14ÈÕ
 *      Author: Yang
 */

#include "crc64.h"

uint64_t crc64(uint64_t crc, const unsigned char *data, uint64_t data_size) {
    uint64_t j;

    for (j = 0; j < data_size; j++) {
        uint8_t byte = data[j];
        crc = crc64_tab[(uint8_t)crc ^ byte] ^ (crc >> 8);
    }
    return crc;
}
