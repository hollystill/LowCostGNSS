/*  uBlox_PPP_logger
 *
 *   u-Blox ZED-F9P GNSS receiver raw UBX data logger for Arduino
 *
 *   Copyright (C) 2023  M. Hamish Bowman
 *     University of Otago, Dunedin, New Zealand
 *
 *   This program is free software licensed under the GPL (>=v3).
 *   Read the GPL-3.txt file that comes with this code for details.
 *
 *   Dependencies:
 *     SparkFun u-Blox GNSS Arduino library by Paul Clark  (MIT license)
 *     Electronic Cats TemperatureZero library  (MIT license)
 *
 *   Records proprietary SFRBX and RAWX u-Blox messages to a daily .UBX file
 *    on an SD card. A monthly STATUS.LOG file with hourly statistics on
 *    position, battery voltage, and temperature is also recorded.
 *
 *   Written for the u-Blox ZED-F9P high precision RTK reciever and a
 *    Cortex M0 (SAMD21) Arduino, but may also work on more powerful hardware.
 *    The M0 is just at the lower limit of what will work, it allows for
 *    a 12 kb memory buffer but 16 kb would be much better. The Arduino
 *    communicates with the receiver board using the I2C bus.
 *
 */


/* some notes from Paul Clark:
  as per Interface Description PDF, CFG-RATE-MEAS (in millisec) defines the time
    between GNSS measurements (100 = 10Hz, 1000 = 1Hz) and
  CFG-RATE-NAV is an integer multiple of -MEAS setting the ratio of # of measurements
    to number of nav solutions.
  So for a 30 sec update interval try 1Hz -MEAS =1000 and 30 sec =30 -NAV setting?
 */


#include <SPI.h>
#include <SD.h>
#include <Wire.h>  // for I2C bus
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <TemperatureZero.h>  // for SAMD21 or SAMD51 processor temperature


// logger ID becomes the first 4 characters of the 8.3 log filename
//   upper case letters only please

#define LOGGER_ID "AAAA"
#define VREF 3.283     // volts, Arduino 3.3v pin, measured with trusted multimeter

// 12v battery calibration: step through voltage range with a lab power supply
//  in place of the actual battery, then find y=mx+b in Matlab or Octave:
//  [p,S] = polyfit(x, y, 1)
//  where x is array of arduino readings, y is array of lab power supply voltages.
#define VDIV_M 6.7735
#define VDIV_B -0.4512


/* another logger board will have slightly different calibration coefficients
#define LOGGER_ID "BBBB"
#define VREF 3.283     // volts
#define VDIV_M 6.7721
#define VDIV_B -0.4506
*/


#define NUM_SAMPS 100   // average this many analog samples for 12v battery voltage reading


// Define the microSD (SPI) Chip Select pin.
#define SD_CS 4
#define SD_PRESENT 7

#define GRN_LED_PIN 8
#define RED_LED_PIN 13   // aka LED_BUILTIN
#define STOP_LOG_PIN A0  // short this pin to ground to stop logging and close data file
#define VOLTDIV_PIN A3   // voltage divider midpoint for measuring 12v battery voltage


#define DEST_SERIAL 0
#define DEST_FILE 1
#define DEST_SRL 2     // to serial, but keep it brief


SFE_UBLOX_GNSS GNSS;

TemperatureZero TempZero = TemperatureZero();

File rawfile;   // the file that the raw GNSS data is written to
File statfile;  // the file that the receiver status log is written to

volatile boolean we_be_logging = false;

#define sdWriteSize 512  // Write data to the SD card in blocks of 512 bytes
//#define fileBufferSize 16384  // Allocate 16KBytes of RAM for UBX message storage (too big for SAMD21?)
//#define fileBufferSize 8192  // Allocate 8 KBytes of RAM for UBX message storage (too small a buffer?)
#define fileBufferSize 12288  // Allocate 12 KBytes of RAM for UBX message storage (just right for the Cortex M0 SAMD21)

uint8_t *Buffer;  // Use Buffer to hold the data while we write it to SD card

unsigned long lastPrint;  // Record when the last Serial print took place
unsigned long last_write = 0;  // last time we wrote to the disk
unsigned long last_rotate = 0;  // last time we rolled over to a new filename
uint8_t last_minute = 66;  // for the are we at the top of the hour? test
uint8_t last_hour = 25;  // for the are we at the top of the day? test


/* Note from Paul Clark: we'll keep a count of how many SFRBX and RAWX
   messages arrive - but the count will not be completely accurate. If
   two or more SFRBX messages arrive together as a group and are processed
   by one call to checkUblox, the count will only increase by one. */
unsigned long numSFRBX = 0;  // Keep count of how many SFRBX message groups have been received (see note above)
unsigned long numRAWX = 0;  // Keep count of how many RAWX message groups have been received (see note above)


///////////    //////////   /////////  //////// ///////


void setup(void)
{
  pinMode(RED_LED_PIN, OUTPUT);  // Flash LED_BUILTIN each time we write to the SD card
  pinMode(GRN_LED_PIN, OUTPUT);
  pinMode(SD_PRESENT, INPUT_PULLUP);
  pinMode(STOP_LOG_PIN, INPUT_PULLUP);
  pinMode(VOLTDIV_PIN, INPUT);   // seems more stable if we keep it at 10 bit resolution

  Serial.begin(115200);
  while (!Serial) {
    if (millis() > 4000) break;
  }
  delay(500);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GRN_LED_PIN, LOW);

  Serial.println(F("== u-blox raw GNSS logger for PPP =="));

  // Start I2C communication. Default I2C address of the ZED-F9P is 0x42.
  Wire.begin();

  // probably not needed
  while (Serial.available())  // Make sure the Serial buffer is empty
    Serial.read();

  Serial.print(F("Logger ID: "));
  Serial.println(LOGGER_ID);

  Serial.print(F("Initializing SD card... "));

  // See if the card is present and can be initialized:
  if (digitalRead(SD_PRESENT) == LOW) {
    Serial.println(F("SD card not installed. Giving up."));
    endless_error_blinks(0);
  }
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD card failed. Giving up."));
    endless_error_blinks(1);
  }

  Serial.println(F("SD card initialized."));
  // set SD file date time callback function
  SdFile::dateTimeCallback(FileDateTime);


  /* Reciever setup commands courtesy of Paul Clark's code */
  //GNSS.enableDebugging(); // Uncomment this line to enable lots of helpful GNSS debug messages on Serial
  //GNSS.enableDebugging(Serial, true); // Or, uncomment this line to enable only the important GNSS debug messages on Serial

  GNSS.disableUBX7Fcheck(); // RAWX data can legitimately contain 0x7F, so we need to disable the "7F" check in checkUbloxI2C

  // RAWX messages can be over 2KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
  // SD cards can occasionally 'hiccup' and a write takes much longer than usual. The buffer needs to be big enough
  // to hold the backlog of data if/when this happens.
  // getMaxFileBufferAvail will tell us the maximum number of bytes which the file buffer has contained.
  GNSS.setFileBufferSize(fileBufferSize);  // setFileBufferSize must be called _before_ .begin

  if (GNSS.begin() == false)  // Connect to the u-blox module using I2C (Wire) port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address."));
    Serial.println(F("Please check wiring. Freezing..."));
    endless_error_blinks(1);
  }

  // Uncomment the next line if you want to reset your module back to the default settings with 1Hz navigation rate
  // (This will also disable any "auto" messages that were enabled and saved by other examples and reduce the load on the I2C bus)
  //GNSS.factoryDefault(); delay(5000);

  GNSS.setI2COutput(COM_TYPE_UBX);  // Set the I2C port to output UBX only (turn off NMEA noise)
  GNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);  // Save (only) the communications port settings to flash and BBR


  GNSS.setNavigationFrequency(1);  // Produce one navigation solution per second (that's plenty for Precise Point Positioning)
  //// FIXME: will this conflict with what follows?

  GNSS.setMeasurementRate(500);  // temporary 0.5 second measurements (RAWX rate)
  GNSS.setNavigationRate(1);     // calc NAV solutions every x measurements  (10000ms * 1 = 10 sec)

  Buffer = new uint8_t[sdWriteSize];  // Create our own buffer to hold the data while we write it to SD card

  Serial.print(F("uBlox firmware version: "));
  byte versionHigh = GNSS.getProtocolVersionHigh();
  Serial.print(versionHigh);
  Serial.print(F("."));
  byte versionLow = GNSS.getProtocolVersionLow();
  Serial.println(versionLow);


  // blink 3 times to say hello
  for (int i = 0; i < 3; i++) {
    digitalWrite(RED_LED_PIN, HIGH);
    delay(200);
    digitalWrite(RED_LED_PIN, LOW);
    delay(200);
  }

  wait_for_fix();
  Serial.println(F("Obtained fix."));
  delay(500);

  wait_for_clock();
  // check that day is not 0 and year is within 2020 -- 2060
  uint16_t year = GNSS.getYear();
  uint8_t day = DayOfYear();
  char lett = HourLetter();
  if (year > 2060 || year < 2020 || day == 0 || lett == 'Z') {
    Serial.println(F("Bad clock, wait some more.."));
    wait_for_clock();
  }


  Serial.print(F("Fix type: "));
  Serial.print(GNSS.getFixType());
  Serial.print(F("  "));
  Serial.print(GNSS.getConfirmedDate() ? " " : "no ");
  Serial.print(F("ConfirmedDate.  "));
  Serial.print(F("Time to fix: "));
  Serial.print(millis() / 1000.0, 0);
  Serial.print(F(" sec.  "));
  print_GNSS_SIV(0);


  Serial.print(F("Year: "));
  Serial.print(GNSS.getYear());
  Serial.print(F(".  Day of year: "));
  Serial.print(DayOfYear());
  Serial.print(F(".  Hour letter: "));
  Serial.println(HourLetter());

  Serial.print(F("Battery voltage: "));
  Serial.print(battery_voltage());
  Serial.println(F("v"));

  /* this works, but not needed here as we know we haven't opened it yet
  if (!statfile)
    Serial.println(F("statfile does not exist"));
  */
  rotate_statfile(true);  // create+open STATUS.LOG in the YYYY/MM directory
  write_boot_info();

  GNSS.setMeasurementRate(10000);  // back to 10 second measurements  (RAWX rate)

  uint16_t rate = GNSS.getMeasurementRate();  // Get the measurement rate of this module
  Serial.print(F("Measurement interval (ms): "));
  Serial.println(rate);
  /*
  rate = GNSS.getNavigationRate();  // Get the navigation rate of this module
  Serial.print(F("Navigation ratio (cycles): "));
  Serial.println(rate);

  /// Huh?  =0?
  //uint8_t rateFq = GNSS.getNavigationFrequency();  // Get the navigation rate of this module
  //Serial.print(F("Navigation freq (Hz): "));
  //Serial.println(rateFq);
  */

  GNSS.setAutoRXMSFRBXcallbackPtr(&newSFRBX);  // Enable automatic RXM SFRBX messages with callback to newSFRBX() function
  GNSS.setAutoRXMRAWXcallbackPtr(&newRAWX);  // Enable automatic RXM RAWX messages with callback to newRAWX() function
  GNSS.logRXMSFRBX();  // Enable RXM SFRBX data logging
  GNSS.logRXMRAWX();  // Enable RXM RAWX data logging

  //GNSS.checkUblox();  // Check for the arrival of new data and process it.
  //GNSS.checkCallbacks();  // Check if any callbacks are waiting to be processed.

  // blink 10 times to say we're ready
  for (int i = 0; i < 10; i++) {
    digitalWrite(RED_LED_PIN, HIGH);
    delay(200);
    digitalWrite(RED_LED_PIN, LOW);
    delay(200);
  }

  if (digitalRead(STOP_LOG_PIN) == LOW)
    Serial.println(F("Remove jumper to start logging."));

  lastPrint = millis(); // Initialize lastPrint
}



///////////    //////////   /////////  //////// ///////


void loop(void)
{

  GNSS.checkUblox();  // Check for the arrival of new data and process it.
  GNSS.checkCallbacks();  // Check if any callbacks are waiting to be processed.

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  // Start logging if we want to and aren't already
  if (!we_be_logging) {
    if (digitalRead(STOP_LOG_PIN) == HIGH) {
      we_be_logging = true;
      // TODO: turn off a ZED-supported power saving mode?

      statfile.print(F("Logging (re)started at "));
      print_GNSS_timedate(DEST_FILE);
      statfile.println(F(" UTC"));
      statfile.flush();

      for (int i = 0; i < 5; i++) {
        digitalWrite(GRN_LED_PIN, HIGH);
        delay(100);
        digitalWrite(GRN_LED_PIN, LOW);
        GNSS.checkUblox();  // Keep checking, and checking, so we don't fall behind.
        GNSS.checkCallbacks();
        delay(50);
      }

      // CHECK ME:
      // If the file already exists, the new data is appended to the end of the file.
      open_new_filename();  // has built in delay(75), we can use that for de-bouncing
      Serial.println(F("Logging begins."));
    }
    else {
      // we still aren't logging; keep flushing the data buffer anyway
      while (GNSS.fileBufferAvailable() >= sdWriteSize)
      {
        // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into Buffer
        GNSS.extractFileBufferData(Buffer, sdWriteSize);
      }
      return;
    }
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


  // Check to see if we have at least sdWriteSize waiting in the buffer
  while (GNSS.fileBufferAvailable() >= sdWriteSize)
  {
    digitalWrite(GRN_LED_PIN, HIGH); // Flash green LED each time we write to the SD card

    // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into Buffer
    GNSS.extractFileBufferData(Buffer, sdWriteSize);

    // Write exactly sdWriteSize bytes from Buffer to the ubxDataFile on the SD card
    rawfile.write(Buffer, sdWriteSize);

    // In case the SD writing is slow or there is a lot of data to write,
    // keep checking for the arrival of new data along the way.
    GNSS.checkUblox(); // Check for the arrival of new data and process it.
    GNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

    digitalWrite(GRN_LED_PIN, LOW); // Turn LED_BUILTIN off again
  }


  // make sure we've written to disk in the last 60 seconds
  if (millis() - last_write > 60000) {
    digitalWrite(RED_LED_PIN, HIGH);
    rawfile.flush();
    last_write = millis();
    digitalWrite(RED_LED_PIN, LOW);
  }


  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  if (millis() > (lastPrint + 60000)) // Print the message count once every minute
  {
    Serial.print(millis() / 1000.0);
    Serial.print(F(": "));
    // Print how many message groups have been received (see note from Paul Clark at top)
    Serial.print(F("Number of message groups received: SFRBX: "));
    Serial.print(numSFRBX);
    Serial.print(F(" RAWX: "));
    Serial.println(numRAWX);

    // Check how full the file buffer has gotten (not how full it is now)
    uint16_t maxBufferBytes = GNSS.getMaxFileBufferAvail();
    Serial.print(F(" The maximum number of bytes which the file buffer has contained is: "));
    Serial.println(maxBufferBytes);

    // Warn the user if fileBufferSize was more than 80% full
    if (maxBufferBytes > ((fileBufferSize / 5) * 4))
      Serial.println(F("Warning: the file buffer has been over 80% full. Some data may have been lost."));

    /* debug:
    Serial.print(F("fileBufferAvailable: "));
    Serial.println(GNSS.fileBufferAvailable());
    */

    /* this can be a bit slow if we're working at a slow nav update frequency
    Serial.print(F(" "));
    print_latlon(DEST_SERIAL);
    Serial.print(F(" "));
    print_GNSS_timedate(DEST_SERIAL);
    */

    // print how many space vechicles are currently in view
    Serial.print(F(" "));
    print_GNSS_SIV(DEST_SERIAL);


    lastPrint = millis();  // reset lastPrint timer
  }


  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  // rotate the status and raw files once a day
  uint8_t now_hour = GNSS.getHour();
  if ( (now_hour == 0 && last_hour != 0) || (millis() - last_rotate > 86461000) ) {
    /// 1 day, 1 minute, and 1 second in millisec = 24*3600*1000 + 60*1000 + 1000

    GNSS.checkUblox(); // All the while, we keep checking.
    GNSS.checkCallbacks();

    if (last_hour != 25) {  // i.e. not a new-boot, where the file was already opened
        // FIXME: we should also try and catch the case where logging was stopped
        //   at 11 and restarted after 12 but before 1.
      GNSS.logRXMSFRBX(false); // Temporarily disable RXM SFRBX data logging
      GNSS.logRXMRAWX(false);  // Temporarily disable RXM RAWX data logging

      GNSS.setMeasurementRate(500);  // temporary 0.5 second measurements  (RAWX rate)

      Serial.println(F("Another day has finished. Rolling over to the next filename."));
      flush_and_close_logfile();

      open_new_filename();   // new file for RAW data to log to

      rotate_statfile(false);

      GNSS.setMeasurementRate(10000);  // back to 10 second measurements  (RAWX rate)

      GNSS.logRXMSFRBX(); // Re-enable RXM SFRBX data logging
      GNSS.logRXMRAWX();  // Re-enable RXM RAWX data logging
    }

    last_hour = 0;

  } else {
    last_hour = now_hour;
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


  // write to the logger status file once an hour
  uint8_t now_minute = GNSS.getMinute();

  if (now_minute == 0 && last_minute != 0) {

    GNSS.logRXMSFRBX(false); // Temporarily disable RXM SFRBX data logging
    GNSS.logRXMRAWX(false);  // Temporarily disable RXM RAWX data logging

    GNSS.setMeasurementRate(500);  // temporary 0.5 second measurements  (RAWX rate)

    print_hourly_status(DEST_SERIAL);  // introduces too much delay, buffer fills
    print_hourly_status(DEST_FILE);
    statfile.flush();

    GNSS.setMeasurementRate(10000);  // back to 10 second measurements  (RAWX rate)

    GNSS.logRXMSFRBX(); // Re-enable RXM SFRBX data logging
    GNSS.logRXMRAWX();  // Re-enable RXM RAWX data logging

    last_minute = 0;
  }
  else {
    last_minute = now_minute;
  }


  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


  // check if the we-want-to-stop-jumper has been installed
  if (digitalRead(STOP_LOG_PIN) == LOW) {
    // cheap debounce
    delay(75);
    if (digitalRead(STOP_LOG_PIN) == HIGH) {
      return;
    }

    GNSS.checkUblox();  // Keep checking.
    GNSS.checkCallbacks();

    we_be_logging = false;
    flush_and_close_logfile();
    Serial.println(F("Logging stopped."));

    digitalWrite(RED_LED_PIN, HIGH);
    delay(200);
    digitalWrite(RED_LED_PIN, LOW);

    statfile.print(F("Logging stopped at "));
    print_GNSS_timedate(DEST_FILE);
    statfile.println(F(" UTC"));
    statfile.flush();

    // TODO: turn on a ZED-supported power saving mode?

    for (int i = 0; i < 5; i++) {
      digitalWrite(RED_LED_PIN, HIGH);
      delay(100);
      digitalWrite(RED_LED_PIN, LOW);
      GNSS.checkUblox(); // Keep checking.
      GNSS.checkCallbacks();
      delay(50);
    }
  }

}
