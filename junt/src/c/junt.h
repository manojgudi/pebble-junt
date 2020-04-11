#pragma once

typedef struct AccelReadingStruct {
  int16_t x;
  int16_t y;
  int16_t z;
  // timestamp array for comparision
  uint64_t epochTimeMS;
  bool isEmptyReading;
   } AccelReading;

// Constructor for AccelReading
AccelReading* newAccelReading(uint64_t epochTimeMS,
	int16_t x, int16_t y, int16_t z) {
    AccelReading* accelReading = malloc(sizeof(AccelReading));
    accelReading->x = x;
    accelReading->y = y;
    accelReading->z = z;
    accelReading->epochTimeMS = epochTimeMS;
    
    // Empty Reading is used later for analysis
    if ((x == 0) && (y == 0) && (z == 0))
        accelReading->isEmptyReading = true;
    else
        accelReading->isEmptyReading = false;

    return accelReading;
} 


// Object* p1 = Object_new(id++, myValue);
