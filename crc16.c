/* 
 * File:   crc16.c
 * Author: aece-engineering
 *
 * Created on December 4, 2014, 9:07 AM
 */

#include "main.h"


#define POLY 0x8408

/*
//                                      16   12   5
// this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
// This works out to be 0x1021, but the way the algorithm works
// lets us use 0x8408 (the reverse of the bit pattern).  The high
// bit is always assumed to be set, thus we only use 16 bits to
// represent the 17 bit value.
*/

unsigned short CRC16(char *data_p, unsigned short length)
{
      unsigned char i;
      unsigned int data;
      unsigned int crc = 0xffff;

      if (length == 0)
            return (~crc);

      do
      {
            for (i=0, data=(unsigned int)0xff & *data_p++;
                 i < 8;
                 i++, data >>= 1)
            {
                  if ((crc & 0x0001) ^ (data & 0x0001))
                        crc = (crc >> 1) ^ POLY;
                  else  crc >>= 1;
            }
      } while (--length);

      crc = ~crc;
      data = crc;
      crc = (crc << 8) | (data >> 8 & 0xff);

      return (crc);
}

//unsigned char calculateAB1CRC(void){
//    unsigned int crc = 0;
//    unsigned char *dataPtr;
//    for (int i = 1; i <= ABB_1.dets_length; i++){
//        crc += ABB_1.det_arrays.UIDs[i].UID[i%4];
//        crc += ABB_1.det_arrays.info[i].delay;
//        dataPtr = &ABB_1.det_arrays.info[i].data;
//        crc += *dataPtr;
//    }
//    return unsigned char (crc & 0xFF);
//}
