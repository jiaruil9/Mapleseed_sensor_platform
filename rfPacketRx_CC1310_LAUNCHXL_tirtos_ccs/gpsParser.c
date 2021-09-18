//
//  gpsParser.c
//  GPS Parser
//

#include "gpsParser.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

// parses GPGGA type messages (called from nmeaParse())
uint8_t nmeaParseGPGGA(GPSData * data) {

    char *start = data->nmeaData.sentence, *end;

    uint32_t i;
    for (i = 0; i < 13 && start != NULL; ++i) {

        start = strchr(start, ',') + 1;
        end = strchr(start, ',');

        if (end - start >= 1) {

            switch (i) {

                case 0: // UTC Time
                    data->time = strtod(start, &end);
                    break;
                case 1: // Latitude
                    data->latitude = strtod(start, &end);
                    break;
                case 2: // N or S
                    data->latDirection = *start;
                    break;
                case 3: // Longitude
                    data->longitude = strtod(start, &end);
                    break;
                case 4: // E or W
                    data->longDirection = *start;
                    break;
                case 8: // Altitude
                    data->altitude = strtod(start, &end);
                    break;

            }
        }

        start = end;
    }

    return 0;
}

// parses GPRMC type messages (called from nmeaParse())
uint8_t nmeaParseGPRMC(GPSData * data) {

    char *start = data->nmeaData.sentence, *end;

    uint32_t i;
    for (i = 0; i < 13 && start != NULL; ++i) {

        start = strchr(start, ',') + 1;
        end = strchr(start, ',');

        if (end - start >= 1) {

            switch (i) {

                case 0: // UTC Time
                    data->time = strtod(start, &end);
                    break;
                case 2: // Latitude
                    data->latitude = strtod(start, &end);
                    break;
                case 3: // N or S
                    data->latDirection = *start;
                    break;
                case 4: // Longitude
                    data->longitude = strtod(start, &end);
                    break;
                case 5: // E or W
                    data->longDirection = *start;
                    break;
                case 6: // Ground Speed
                    data->groundSpeed = strtod(start, &end);
                    break;
                case 7: // True Course
                    data->trueCourse = strtod(start, &end);

            }
        }

        start = end;
    }

    return 0;
}

// initializes the data struct to default values
void nmeaDataInit(GPSData * data) {

    uint32_t i;
    for (i = 0; i < SENTENCE_LENGTH - 1; ++i) {
        data->nmeaData.sentence[i] = ' ';
    }

    data->nmeaData.sentence[ SENTENCE_LENGTH - 1 ] = '\0';

    data->latitude = 0;
    data->latDirection = ' ';

    data->longitude = 0;
    data->longDirection = ' ';

    data->time = 0;
    data->altitude = 0;
    data->groundSpeed = 0;
    data->trueCourse = 0;

}

// Takes in an NMEA sentence and places is it into the data struct if the msg is valid.
// Verifies the checksum is valid and determines the msg type
uint8_t nmeaReceiveSentence(GPSData * data, char * sentIn) {

    if (data != NULL)
        data->nmeaData.sentence[0] = '\0';

    char * start = strchr(sentIn, '$');

    if (start == NULL) {
        strcpy(data->nmeaData.sentence, "INVALID SENTENCE");
        data->nmeaData.msgType = unknownNMEA;
        return 1;
    }

    strcpy(data->nmeaData.sentence, start);

    char * checksumPtr = strchr(data->nmeaData.sentence, '*');
    uint16_t expectedChecksum = (uint16_t) strtol(checksumPtr + 1, NULL, 16);

    char * ptr = data->nmeaData.sentence + 1;
    uint16_t checksum = 0;
    while (checksumPtr && ptr != checksumPtr) {
        checksum ^= *ptr++;
    }

    if (expectedChecksum != checksum) {
        strcpy(data->nmeaData.sentence, "INVALID SENTENCE");
        data->nmeaData.msgType = unknownNMEA;
        return 1;
    }

    *checksumPtr = '\0';

    if ( memcmp(start, "$GPGGA", 6) == 0  ) {
        data->nmeaData.msgType = GPGGA;
    }
    else if ( memcmp(start, "$GPRMC", 6) == 0 ) {
        data->nmeaData.msgType = GPRMC;
    }
    else if ( memcmp(start, "$GPGSV", 6) == 0 ) {
        data->nmeaData.msgType = GPGSV;
    }
    else if ( memcmp(start, "$GPGSA", 6) == 0 ) {
        data->nmeaData.msgType = GPGSA;
    }
    else {
        data->nmeaData.msgType = unknownNMEA;
    }

    return 0;

}

///  NMEA Data Parser: returns 0 if data parsing is successful, 1 otherwise.
/// @param data address to a GPSData struct (defined in GPS_parser.h)
uint8_t nmeaParse(GPSData * data) {

    switch (data->nmeaData.msgType) {

        case (GPGGA):
            return nmeaParseGPGGA(data);
        case (GPRMC):
            return nmeaParseGPRMC(data);
        case (GPGSA):
            return 0;
        case (GPGSV):
            return 0;
        default:
            return 1;

    }

}

/* Some useful char to print */
const char  latitude[] = "latitude:\t";
const char  longitude[] = "longitude:\t";
const char  groundSpeed[] = "ground speed:\t";
const char  trueCourse[] = "true course:\t";
const char  colon[] = ":" ;
const char  period[] = ".";
const char  newline[] = "\r\n";
const char  tab[] = "\t";
const char  deg[] = " ";

void nmeaToString(GPSData * data, char * strOut) {

    char * infoptr = strOut;
    uint32_t length = 0;

    /* time */
    length += sprintf(infoptr + length, "%02d", (uint32_t)(data->time) / 10000 % 100);
    length += sprintf(infoptr + length, "%s", colon);
    length += sprintf(infoptr + length, "%02d", (uint32_t)(data->time) / 100     % 100);
    length += sprintf(infoptr + length, "%s", colon);
    length += sprintf(infoptr + length, "%02d", (uint32_t)(data->time) / 1   % 100);
    length += sprintf(infoptr + length, "%s", tab);

    /* latitude */
    length += sprintf(infoptr + length, "%s", latitude);
    length += sprintf(infoptr + length, "%d", (int32_t)(data->latitude) / 100);
    length += sprintf(infoptr + length, "%s", " ");
    length += sprintf(infoptr + length, "%lf", data->latitude - ((uint32_t)(data->latitude) / 100) * 100);
    length += sprintf(infoptr + length, "%c", data->latDirection);
    length += sprintf(infoptr + length, "%s", tab);

    /* latitude */
    length += sprintf(infoptr + length, "%s", longitude);
    length += sprintf(infoptr + length, "%d", (int32_t)(data->longitude) / 100);
    length += sprintf(infoptr + length, "%s", " ");
    length += sprintf(infoptr + length, "%lf", data->longitude - ((uint32_t)(data->longitude) / 100) * 100);
    length += sprintf(infoptr + length, "%c", data->longDirection);
    length += sprintf(infoptr + length, "%s", tab);

    /* ground speed */
    length += sprintf(infoptr + length, "%s", groundSpeed);
    length += sprintf(infoptr + length, "%lf", data->groundSpeed);
    length += sprintf(infoptr + length, "%s", tab);

    /* true course */
    length += sprintf(infoptr + length, "%s", trueCourse);
    length += sprintf(infoptr + length, "%lf", data->trueCourse);
    length += sprintf(infoptr + length, "%s", newline);

}
