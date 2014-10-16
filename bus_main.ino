/***********************************************************************
* This is a modified version of Mikal Hart's GPS Parser from Adafruit.com combined
* with a GSM code that sends the NMEA sentences to Xively database at our
* project's feeds.
* Transportation Sensors
*-------------------------------- Author: Ethan Thompson-------------------------------------------- *
***********************************************************************/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include <String.h>

//check the feeds by going to https://api.xively.com/v2/feeds/614237319.csv?duration=6hours&interval=0

// Use pins 2 and 3 to talk to the GPS. 2 is the TX pin, 3 is the RX pin
SoftwareSerial gpsSerial = SoftwareSerial(2, 3);

//Use pins 7 and 8 for GSM module
SoftwareSerial gsmSerial = SoftwareSerial(7, 8);

// Use pin 4 to control power to the GPS
#define powerpin 4

// Set the GPSRATE to the baud rate of the GPS module. Most are 4800
// but some are 38400 or other. Check the datasheet!
#define GPSRATE 9600

//GSM baud rate is 19200
#define GSMRATE 19200

// The buffer size that will hold a GPS sentence. They tend to be 80 characters long
// so 90 is plenty.
#define BUFFSIZ 90 // plenty big

// global variables
char buffer[BUFFSIZ];

// string buffer for the sentence
char *parseptr;

// a character pointer for parsing
char buffidx;

// an indexer into the buffer
String gpsData2Send;

//string that gets pushed out
// The time, date, location data, etc.
uint8_t hour, minute, second, year, month, date;
uint32_t latitude, longitude;
uint8_t groundspeed, trackangle;
char latdir, longdir;
uint8_t measurementNum = 0;

//defining number of measurements taken so far
char status;

void setup()
{
	if (powerpin) {
		pinMode(powerpin, OUTPUT);
	}
	// Use the pin 13 LED as an indicator
	pinMode(13, OUTPUT);
	// connect to the serial terminal at 9600 baud
	Serial.begin(9600);
	// connect to the GPS and GSM at the desired rates
	gsmSerial.begin(GSMRATE);
	//rate at 19200
	gpsSerial.begin(GPSRATE);
	//rate at 9600
	//by default, Arduino starts listening on last serial port initialized
	digitalWrite(powerpin, LOW);
	// pull low to turn on!
	// prints title with ending line break
	Serial.println("\nTransportation Sensors Tracking Device");
	delay(2000);
	Serial.println("\nEstablishing Connection with Xively...\n");
	gsmSerial.listen();
	gsmSerial.write("AT+CGATT?");
	delay(1000);
	ShowGSMSerialData();
	gsmSerial.println("AT+CSTT=\"epc.tmobile.com\""); //start task and setting the APN,
	delay(1000);
	ShowGSMSerialData();
	gsmSerial.println("AT+CIICR"); //bring up wireless connection
	delay(3000);
	ShowGSMSerialData();
	gsmSerial.println("AT+CIFSR"); //get local IP adress
	delay(2000);
	ShowGSMSerialData();
	gsmSerial.println("AT+CIPSPRT=0");
	delay(3000);
	ShowGSMSerialData();

	// start up the connection
	gsmSerial.println("AT+CIPSTART=\"tcp\",\"api.xively.com\",\"8081\"");

	delay(2000);
	ShowGSMSerialData();
}

void loop()
{
	uint32_t tmp;
	gpsSerial.listen();
	readline();
	// check if $GPRMC (global positioning fixed data)
	if ((strncmp(buffer, "$GPRMC",6) == 0))
	{
		//if found a GPRMC data, parse it out
		delay(50);
		// hhmmss time data
		parseptr = buffer+7;
		//parsptr is the pointer to the place in
		tmp = parsedecimal(parseptr);
		//the string we're looking at at that point
		hour = tmp / 10000;
		minute = (tmp / 100) % 100;
		second = tmp % 100;
		parseptr = strchr(parseptr, ',') + 1;
		status = parseptr[0];
		parseptr += 2;
		// grab latitude & long data
		// latitude
		latitude = parsedecimal(parseptr);
		if (latitude != 0) {
			latitude *= 10000;
			parseptr = strchr(parseptr, '.')+1;
			latitude += parsedecimal(parseptr);
		}
		parseptr = strchr(parseptr, ',') + 1;
		// read latitude N/S data
		if (parseptr[0] != ',') {
			latdir = parseptr[0];
		}
		// longitude
		parseptr = strchr(parseptr, ',')+1;
		longitude = parsedecimal(parseptr);
		if (longitude != 0) {
			longitude *= 10000;
			parseptr = strchr(parseptr, '.')+1;
			longitude += parsedecimal(parseptr);
		}
		parseptr = strchr(parseptr, ',')+1;
		// read longitude E/W data
		if (parseptr[0] != ',') {
			longdir = parseptr[0];
		}
		measurementNum ++;
		// groundspeed
		parseptr = strchr(parseptr, ',')+1;
		groundspeed = parsedecimal(parseptr);
		//get ground - speed
		// track angle
		parseptr = strchr(parseptr, ',')+1;
		trackangle = parsedecimal(parseptr);
		//get track angle
		// date
		parseptr = strchr(parseptr, ',')+1;
		tmp = parsedecimal(parseptr);
		date = tmp / 10000;
		month = (tmp / 100) % 100;
		year = tmp % 100;
		//get timevalues
		gpsData2Send += latitude;
		gpsData2Send += ',';
		gpsData2Send += longitude;
		gpsData2Send += '\\n';
		gpsSerial.listen();
		//Serial.println("\n" + gpsData2Send);
		Serial.println(gpsData2Send);
		//Serial.println(measurementNum);
		if(measurementNum == 1)
		{
			//gsmSerial.listen();
			//Serial.println("\nSending data to Xively now.");
			//send string thru gsm
			Send2Xively();
			//Serial.println("Data Transfer Complete\n");
			gpsData2Send = "";
			measurementNum = 0;
			//when done, reset string to send and increment number of
			measurements taken so far
				delay(500);
			//gpsSerial.listen();
		}
		delay(5000); //This is where we change the time interval between GPS measurements
	}
	delay(50);
}

/*******************************************************************/
void Send2Xively()
{
	gsmSerial.println("AT+CIPSEND"); //begin send data to remote server
	delay(400);
	//ShowGSMSerialData();
	//String humidity = "1031";//these 4 line code are imitate the real sensor data, because the demo did't add other sensor, so using 4 string variable to replace.
	gsmSerial.print("{\"method\": \"put\",\"resource\": \"/feeds/614237319/\",\"params\"");
	delay(600);
	gsmSerial.print(":{},\"headers\":{\"X-ApiKey\":\"imuzcw90PJa15g0XxgzuagnsbsFCoHJpW9pdQHVcy561AhMT\"},\"body\": {\"v2\" :\"1.0.0\",\"datastreams\": [ {\"id\": \"capstone_GPS_data\",\"current_value\": \"" + gpsData2Send + "\"}]}}");
	delay(800);
	gsmSerial.println((char)26);//sending
	delay(5000);//waitting for reply, important! the time is base on the condition of internet
	gsmSerial.println();
	//gsmSerial.println("AT+CIPCLOSE");//close the connection
	//delay(1000);
	}
	/******************************************************************/
	void readline(void) {
		char c;
		buffidx = 0; // start at begninning
		while (1) {
			c=gpsSerial.read();
			if (c == -1)
				continue;
			//function to read and print the lines of NMEA sentences being found
			//Serial.print(c);
			if (c == '\n')
				continue;
			if ((buffidx == BUFFSIZ-1) || (c == '\r')) {
				buffer[buffidx] = 0;
				return;
			}
			buffer[buffidx++]= c;
		}
		delay(1000);
	}
	/*******************************************************************/
	uint32_t parsedecimal(char *str) {
		uint32_t d = 0;
		while (str[0] != 0) {
			if ((str[0] > '9') || (str[0] < '0')) return d;
			d *= 10;
			d += str[0] - '0';
			str++;
		}
		return d;
	}
	/*********************************************************************/
}