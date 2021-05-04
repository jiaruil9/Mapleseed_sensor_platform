//
//  gpsParser.h
//  GPS Parser
//

#ifndef gpsParser_h
#define gpsParser_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SENTENCE_LENGTH 83 // 82 + 1 = sentence + null terminator


// NMEA sentence types
typedef enum { GPGGA, GPGSA, GPRMC, GPGSV, unknownNMEA } NMEAType;

typedef struct {
    char sentence[SENTENCE_LENGTH];
    NMEAType msgType;

} NMEASentence;

typedef struct {

    NMEASentence nmeaData;

    double latitude;
    char latDirection;

    double longitude;
    char longDirection;

    double time;

    double altitude;

    double groundSpeed;

    double trueCourse;

} GPSData;

void nmeaDataInit(GPSData * data);

uint8_t nmeaReceiveSentence(GPSData * data, char * sentIn);

uint8_t nmeaParse(GPSData * data);

void nmeaToString(GPSData * data, char * strOut);


#ifdef __cplusplus
}
#endif

#endif /* gpsParser_h */
