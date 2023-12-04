/* This code is Free Software licensed under the GPL version 3 or newer.
   Some functions originate from other authors, as noted. */


// SD card file timestamp callback function
void FileDateTime(uint16_t *date, uint16_t *time) {
  if (GNSS.getDateValid()) {
    *date = FAT_DATE(GNSS.getYear(), GNSS.getMonth(), GNSS.getDay());
  }

  if (GNSS.getTimeValid()) {
    *time = FAT_TIME(GNSS.getHour(), GNSS.getMinute(), GNSS.getSecond());
  }
}



// make the world blink, forever
void endless_error_blinks(int together) {
  while (1) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GRN_LED_PIN, together ? LOW : HIGH);
    delay(100);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GRN_LED_PIN, together ? HIGH : LOW);
    delay(100);
  }
}



/* More of less adapted from Paul Clark's DataLoggingExample3_RXM_SFRBX_and_RAWX (MIT) */
void flush_and_close_logfile(void)
{
  // Check if there are any bytes remaining in the file buffer
  uint16_t remainingBytes = GNSS.fileBufferAvailable();

  while (remainingBytes > 0) {  // While there is still data in the file buffer
    digitalWrite(GRN_LED_PIN, HIGH);  // Flash LED_BUILTIN while we write to the SD card

    // Write the remaining bytes to SD card sdWriteSize bytes at a time
    uint16_t bytesToWrite = remainingBytes;
    if (bytesToWrite > sdWriteSize) {
      bytesToWrite = sdWriteSize;
    }

    // Extract bytesToWrite bytes from the UBX file buffer and put them into Buffer
    GNSS.extractFileBufferData(Buffer, bytesToWrite);

    // Write bytesToWrite bytes from Buffer to the ubxDataFile on the SD card
    rawfile.write(Buffer, bytesToWrite);

    remainingBytes -= bytesToWrite;  // Decrement remainingBytes
  }

  digitalWrite(GRN_LED_PIN, LOW);  // Turn LED_BUILTIN off

  rawfile.flush();
  rawfile.close(); // Close the data file
}



/// daily date code rotate ///
void open_new_filename(void) {
  // Create or open a file called "IIDDjjjn.UBX" on the SD card.
  //  = ID+day of year+hour
  char dirname[12];   // = "2023/12"
  char filename_raw[16];  // = "IDxxdddh.UBX";
  char fullpath_raw[32];  // = "dirname/filename"

  //FIXME: what if GNSS.getConfirmedDate() fails?

  // first set the directory name
  sprintf(dirname, "%d/%02d", GNSS.getYear(), GNSS.getMonth());
  if (!SD.exists(dirname)) {
    Serial.println(F("Directory doesn't exist, making it."));
    if (!SD.mkdir(dirname))
      Serial.println(F("mkdir failed"));
  }

  // then set the file name in NOAA NGS style (sort of, as we may exceed
  //   60 minutes per file)
  sprintf(filename_raw, "%s%03d%c.UBX", LOGGER_ID, DayOfYear(), HourLetter());

  // finally put them together
  sprintf(fullpath_raw, "%s/%s", dirname, filename_raw);

  if (SD.exists(filename_raw))
    Serial.println(F("The new file already exists. Will append to it."));

  // default write mode is append. Do we need to seek to end?  NO
  if (rawfile)
    Serial.println(F("???: rawfile already exists."));

  rawfile = SD.open(fullpath_raw, FILE_WRITE);

  Serial.print(F("Logging to "));
  Serial.println(filename_raw);
  statfile.print(F("Logging to "));
  statfile.println(fullpath_raw);

  delay(75);  // give it a chance to catch up before testing if it's ok.

  // do we really want to permanently give up?
  if (! rawfile) {
    Serial.println(F("SD card file creation failed. Try again tomorrow?"));
    //endless_error_blinks(1);
  }

  last_rotate = millis();
}



void rotate_statfile(bool just_booted) {
  char dirname[12];   // e.g. = "2023/12"
  char fullpath_stat[32];  // = "dirname/STATUS.LOG"

  if (statfile) {
    statfile.flush();
    statfile.close(); // Close the status log file
  }

  // first set the directory name
  //FIXME: what if GNSS.getConfirmedDate() fails?
  sprintf(dirname, "%d/%02d", GNSS.getYear(), GNSS.getMonth());
  if (!SD.exists(dirname)) {
    Serial.println(F("Directory doesn't exist, making it."));
    if (!SD.mkdir(dirname)) {
      Serial.println(F("mkdir failed"));
      return;
    }
  }

  sprintf(fullpath_stat, "%s/STATUS.LOG", dirname);
  statfile = SD.open(fullpath_stat, FILE_WRITE);

  Serial.print(F("Writing status messages to "));
  Serial.println(fullpath_stat);

  delay(75);  // give it a chance to catch up before testing if it's ok.

  // do we really want to give up?
  if (! statfile) {
    Serial.println(F("SD card file creation failed. Try again tomorrow?."));
    return;
  }

  //if (!just_booted)
  //  print_hourly_status(DEST_FILE);
}


// print boot info to STATUS.LOG file
void write_boot_info(void)
{
  statfile.println(F("%%"));
  statfile.print(F("Otago University Geology uBlox logger booted "));
  print_GNSS_timedate(DEST_FILE);
  statfile.print(F(" UTC ("));
  // timecode:
  statfile.print(GNSS.getYear());
  statfile.print(F("/"));
  statfile.print(DayOfYear());
  statfile.print(HourLetter());
  statfile.println(F(")"));
  statfile.print(F("Logger: "));
  statfile.print(LOGGER_ID);
  statfile.print(F(", uBlox firmware version: "));
  byte versionHigh = GNSS.getProtocolVersionHigh();
  statfile.print(versionHigh);
  statfile.print(F("."));
  byte versionLow = GNSS.getProtocolVersionLow();
  statfile.println(versionLow);

  statfile.print(F("Starting at: "));
  //if (GNSS.getGnssFixOk())    // often it isn't ok even when it says it is
  if (GNSS.getFixType() >= 3)
    print_latlon(DEST_FILE);
  else
    statfile.print(F("?,?"));

  statfile.print(F(", SV: "));
  print_GNSS_SIV(DEST_FILE);

  statfile.print(F(", CPU temperature: "));
  statfile.print(SAMD21_degC());
  statfile.println(F(" C"));

  statfile.print(F("Battery voltage: "));
  statfile.println(battery_voltage());

  //statfile.println(F("date time, lat, lon, z_ellipsoid, z_MSL, SatsInView, pDOP, hAcc, Vacc, degC_SAMD21, battV"));
  statfile.println(F("date time, lat, lon, z_ellipsoid, SatsInView, 3Daccuracy, degC_SAMD21, battV"));
}



// write out status info:
//  date time, lat, lon, z_ellipsoid (m), z_MSL (m), SatsInView, hAcc, Vacc, 3DAcc, pDOP, degC_SAMD21, battV
void print_hourly_status(short int destination)
{
  GNSS.getPVT();
  GNSS.checkUblox();  // Check for the arrival of new data and process it.
  GNSS.checkCallbacks();  // Check if any callbacks are waiting to be processed.

  if (destination == DEST_FILE) {
    //statfile.println(F("some o'clock and all's well."));
    print_GNSS_timedate(DEST_FILE);

    GNSS.checkUblox();  // Keep checking.
    GNSS.checkCallbacks();

    statfile.print(F(","));
    print_latlon(DEST_FILE);

    GNSS.checkUblox();  // Querying can take a while so keep checking
    GNSS.checkCallbacks();  // and keep dealing with any results

    statfile.print(F(","));
    statfile.print(GNSS.getElipsoid() / 1000.0, 3);  // measured in millimeters, -> m
    statfile.print(F(","));
    //statfile.print(GNSS.getMeanSeaLevel() / 1000.0, 3);  // measured in millimeters, -> m
    //statfile.print(F(","));
    print_GNSS_SIV(DEST_FILE);
    statfile.print(F(","));

    GNSS.checkUblox();  // Keep checking.
    GNSS.checkCallbacks();
    /*
    statfile.print(GNSS.getHorizontalAccuracy());
    statfile.print(F(","));
    statfile.print(GNSS.getVerticalAccuracy());
    statfile.print(F(","));

    GNSS.checkUblox();  // Keep checking.
    GNSS.checkCallbacks();
    */
    statfile.print(GNSS.getPositionAccuracy() / 1000.0, 3);  // 3D, measured in mm --> logged in meters
    statfile.print(F(","));
    //statfile.print(GNSS.getPDOP());  // positional dillution of precision * 10^-2 (dimensionless)
    //statfile.print(F(","));

    statfile.print(SAMD21_degC(), 1);
    statfile.print(F(","));
    statfile.println(battery_voltage());
  }
  else {
    print_GNSS_timedate(DEST_SERIAL);

    GNSS.checkUblox();  // Keep checking.
    GNSS.checkCallbacks();

    Serial.print(F(","));
    print_latlon(DEST_SRL);

    GNSS.checkUblox();  // Keep checking.
    GNSS.checkCallbacks();

    Serial.print(F(","));
    Serial.print(GNSS.getElipsoid() / 1000.0, 3);  // measured in millimeters, -> m
    Serial.print(F(","));
    // the uBlox ZED-F9P stores the EGM96 geoid model with limited resolution
    //  so internal calc that converts ellips to MSL may not be the best.
    //Serial.print(GNSS.getMeanSeaLevel() / 1000.0, 3);  // measured in millimeters, -> m
    //Serial.print(F(","));
    /* "High precision component of height above ellipsoid.
      Must be in the range -9..+9. Precise height in mm = height + (heightHp * 0.1)."
    Serial.println(GNSS.getElipsoid() + (GNSS.getElipsoidHp() * 0.1));  // in millimeters
    */
    Serial.print(GNSS.getSIV());
    Serial.print(F(","));

    GNSS.checkUblox();  // Keep checking.
    GNSS.checkCallbacks();
    /*
    Serial.print(GNSS.getHorizontalAccuracy());
    Serial.print(F(","));
    Serial.print(GNSS.getVerticalAccuracy());
    Serial.print(F(","));

    GNSS.checkUblox();  // Keep checking.
    GNSS.checkCallbacks();
    */
    Serial.print(GNSS.getPositionAccuracy() / 1000.0, 3);  // 3D, in mm, -> m
    Serial.print(F(","));
    //Serial.print(GNSS.getPDOP());  // positional dillution of precision * 10^-2 (dimensionless)
    //Serial.print(F(","));

    Serial.print(SAMD21_degC(), 1);
    Serial.print(F(","));
    Serial.println(battery_voltage());
  }
}



// SAMD21 or SAMD51 processor temperature  (it's low enough power so that
//  this is roughly the ambient air temperature)
float SAMD21_degC(void)
{
  TempZero.init();     // wake it up?
  float degC = TempZero.readInternalTemperature();
  TempZero.disable();  // saves ~60uA in standby
  return degC;
}


// calc 12v battery voltage from battery divider
float battery_voltage(void)
{
  unsigned int i;
  long int reading_sum = 0;
  float mean_reading;

  for (i = 0; i < NUM_SAMPS; i++) {
    reading_sum += analogRead(VOLTDIV_PIN);
  }

  mean_reading = (float)reading_sum / NUM_SAMPS;

  return VDIV_M * (mean_reading * VREF / 1023.0) + VDIV_B;  // for 12 bit use 4095, but not as stable a reading?
}



/* From Paul Clark's DataLoggingExample3_RXM_SFRBX_and_RAWX (MIT) */
// Callback: newSFRBX will be called when new RXM SFRBX data arrives
void newSFRBX(UBX_RXM_SFRBX_data_t *ubxDataStruct)
{
  numSFRBX++; // Increment the running count of messages that have arrived
}

// Callback: newRAWX will be called when new RXM RAWX data arrives
void newRAWX(UBX_RXM_RAWX_data_t *ubxDataStruct)
{
  numRAWX++; // Increment the count
}



// Pretty-print the fractional part with leading zeros - without using printf
// (Only works with positive numbers)
//   destination: 0 = Serial; 1 = logfile
/* From SparkFun's Example10_GetHighPrecisionPositionAndAccuracy by Nathan Seidle,
    Steven Rowland, and Paul Clark. (MIT) */
void printFractional(int32_t fractional, uint8_t places, short int destination)
{
  if (places > 1)
  {
    for (uint8_t place = places - 1; place > 0; place--)
    {
      if (fractional < pow(10, place))
      {
        if (destination == DEST_FILE)
          statfile.print(F("0"));
        else
          Serial.print(F("0"));
      }
    }
  }
  if (destination == DEST_FILE)
    statfile.print(fractional);
  else
    Serial.print(fractional);
}



/*  formats and prints the position as lat,lon
    destination: 0 = Serial;  1 = logfile;  2: Serial, but brief
    no newline is added. */
void print_latlon(short int destination)
{
  GNSS.getPVT();
  GNSS.checkUblox();  // Keep checking.
  GNSS.checkCallbacks();

  /* Adapted from SparkFun's Example10_GetHighPrecisionPositionAndAccuracy
      by Nathan Seidle, Steven Rowland, and Paul Clark. (MIT) */
  int32_t latitude = GNSS.getHighResLatitude();
  int8_t latitudeHp = GNSS.getHighResLatitudeHp();
  int32_t longitude = GNSS.getHighResLongitude();
  int8_t longitudeHp = GNSS.getHighResLongitudeHp();

  int32_t lat_int; // Integer part of the latitude in degrees
  int32_t lat_frac; // Fractional part of the latitude
  int32_t lon_int; // Integer part of the longitude in degrees
  int32_t lon_frac; // Fractional part of the longitude

  // Calculate the latitude and longitude integer and fractional parts
  lat_int = latitude / 10000000; // Convert latitude from degrees * 10^-7 to Degrees
  lat_frac = latitude - (lat_int * 10000000); // Calculate the fractional part of the latitude
  lat_frac = (lat_frac * 100) + latitudeHp; // Now add the high resolution component
  if (lat_frac < 0) // If the fractional part is negative, remove the minus sign
  {
    lat_frac = 0 - lat_frac;
  }
  lon_int = longitude / 10000000; // Convert latitude from degrees * 10^-7 to Degrees
  lon_frac = longitude - (lon_int * 10000000); // Calculate the fractional part of the longitude
  lon_frac = (lon_frac * 100) + longitudeHp; // Now add the high resolution component
  if (lon_frac < 0) // If the fractional part is negative, remove the minus sign
  {
    lon_frac = 0 - lon_frac;
  }

  if (destination == DEST_FILE) { // logfile
    statfile.print(lat_int);
    statfile.print(F("."));
    printFractional(lat_frac, 9, DEST_FILE);
    statfile.print(F(","));
    statfile.print(lon_int);
    statfile.print(F("."));
    printFractional(lon_frac, 9, DEST_FILE);
  } else if (destination == DEST_SERIAL) {
    // Print the lat and lon
    Serial.print(F("Lat (deg): "));
    Serial.print(lat_int); // Print the integer part of the latitude
    Serial.print(F("."));
    printFractional(lat_frac, 9, DEST_SERIAL); // Print the fractional part of the latitude with leading zeros
    Serial.print(F(", Lon (deg): "));
    Serial.print(lon_int); // Print the integer part of the latitude
    Serial.print(F("."));
    printFractional(lon_frac, 9, DEST_SERIAL); // Print the fractional part of the latitude with leading zeros
    Serial.println();
  } else {   // to serial, but keep it brief
    Serial.print(lat_int); // Print the integer part of the latitude
    Serial.print(F("."));
    printFractional(lat_frac, 9, DEST_SERIAL); // Print the fractional part of the latitude with leading zeros
    Serial.print(F(","));
    Serial.print(lon_int); // Print the integer part of the latitude
    Serial.print(F("."));
    printFractional(lon_frac, 9, DEST_SERIAL); // Print the fractional part of the latitude with leading zeros
  }
}



/*  formats and prints the time as hh:mm:ss
    destination: 0 = Serial; 1 = logfile
    no newline is added. */
void print_GNSS_timedate(short int destination)
{
  uint8_t Month = GNSS.getMonth();
  uint8_t Day = GNSS.getDay();
  uint8_t Hour = GNSS.getHour();
  uint8_t Minute = GNSS.getMinute();
  uint8_t Second = GNSS.getSecond();

  if (destination == DEST_FILE) { // logfile
    statfile.print(GNSS.getYear());
    statfile.print(F("/"));
    if (Month < 10)
      statfile.print(F("0"));
    statfile.print(Month);
    statfile.print(F("/"));
    if (Day < 10)
      statfile.print(F("0"));
    statfile.print(Day);
    statfile.print(F(" "));
    if (Hour < 10)
      statfile.print(F("0"));
    statfile.print(Hour);
    statfile.print(F(":"));
    if (Minute < 10)
      statfile.print(F("0"));
    statfile.print(Minute);
    statfile.print(F(":"));
    if (Second < 10)
      statfile.print(F("0"));
    statfile.print(Second);
  } else {
    Serial.print(GNSS.getYear());
    Serial.print(F("/"));
    if (Month < 10)
      Serial.print(F("0"));
    Serial.print(Month);
    Serial.print(F("/"));
    if (Day < 10)
      Serial.print(F("0"));
    Serial.print(Day);
    Serial.print(F(" "));
    if (Hour < 10)
      Serial.print(F("0"));
    Serial.print(Hour);
    Serial.print(F(":"));
    if (Minute < 10)
      Serial.print(F("0"));
    Serial.print(Minute);
    Serial.print(F(":"));
    if (Second < 10)
      Serial.print(F("0"));
    Serial.print(Second);
    /*
    Serial.print(F("  Time is "));
    if (GNSS.getTimeValid() == false)
    {
      Serial.print(F("not "));
    }
    Serial.print(F("valid.  Date is "));
    if (GNSS.getDateValid() == false)
    {
      Serial.print(F("not "));
    }
    Serial.print(F("valid."));
    Serial.println();
    */
  }
}



// print number of satellites used in fix
//    destination: 0 = Serial; 1 = logfile
void print_GNSS_SIV(short int destination)
{
  if (destination == DEST_FILE) { // logfile
    statfile.print(GNSS.getSIV());
  } else {
    Serial.print(F("SIV: "));
    Serial.print(GNSS.getSIV());
    Serial.println();
  }
}



// from SparkFun_u-blox_GNSS_Arduino_Library.h (MIT)
const uint16_t DAYS_SINCE_MONTH[2][12] =
{
  {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}, // Leap Year (Year % 4 == 0)
  {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334}  // Normal Year
};

// Returns day of year, or 0 if the GPS lost the plot
uint16_t DayOfYear(void)
{
  if (!GNSS.getConfirmedDate())
    return 0;

  uint16_t doy = DAYS_SINCE_MONTH[GNSS.getYear() % 4 == 0 ? 0 : 1][GNSS.getMonth() - 1];
  doy += GNSS.getDay();

  return doy;
}



// return A for 00:00, B for 01:00, ... X for 23:00. Or 'Z' if the GPS is lost
char HourLetter(void)
{
  if (!GNSS.getConfirmedTime())
    return 'Z';

  return 'A' + GNSS.getHour();
}



// loops until getGnssFixOk() returns true, slowly blinking the red LED
void wait_for_fix(void)
{
  while (!GNSS.getGnssFixOk()) {
    digitalWrite(RED_LED_PIN, HIGH);
    Serial.print(F("waiting for fix... "));
    GNSS.checkUblox();  // Check for the arrival of new data and process it.
    //GNSS.checkCallbacks();  // no valid messages coming in, so no need to check here
    Serial.println(GNSS.getConfirmedDate() ? "ConfirmedDate" : "no ConfirmedDate");
    delay(2000);  // checks take a couple seconds
    digitalWrite(RED_LED_PIN, LOW);
    delay(2000);
  }
}

// loops until getConfirmedDate() returns true, slowly blinking the green LED
void wait_for_clock(void)
{
  while (!GNSS.getConfirmedDate()) {
    digitalWrite(GRN_LED_PIN, HIGH);
    Serial.print(F("waiting for clock... "));
    GNSS.checkUblox();
    Serial.println(GNSS.getConfirmedDate() ? "ConfirmedDate" : "no ConfirmedDate");
    delay(1000);  // checks take a couple seconds
    digitalWrite(GRN_LED_PIN, LOW);
    delay(1000);
  }
}
