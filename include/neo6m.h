#ifndef NEO6M_H
#define NEO6M_H

#include <TinyGPS++.h>
#include <Arduino.h>

// UTC offset configuration
extern const int UTC_OFFSET; // set to -8 in winter

// Global GPS variables
extern double g_latitude;
extern double g_longitude;
extern int g_year;
extern int g_month;    // 1-12 format for easier use
extern int g_day;
extern int g_hour;     // 24-hour format
extern int g_minute;
extern int g_second;
extern bool g_location_valid;
extern bool g_datetime_valid;

// TinyGPS++ object
extern TinyGPSPlus gps;

// Function declarations
void initGPS();
void updateGlobalGPSData();
void displayGPSInfo();
bool processGPSData();
int daysInMonth(int month, int year);
void applyUtcOffset(int &year, int &month, int &day, int &hour, int offsetHours);

// Helper functions for GPS data access
double getLatitude();
double getLongitude();
bool isLocationValid();
bool isDateTimeValid();
int getGPSYear();
int getGPSMonth();
int getGPSDay();
int getGPSHour();
int getGPSMinute();
int getGPSSecond();

// Format helpers
String getFormattedTime12Hour();
String getFormattedTime24Hour();
String getFormattedDate();
String getFormattedCoordinates();

#endif // NEO6M_H