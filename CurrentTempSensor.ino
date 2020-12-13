// Jose Nunes 13/10/2020
// updates CURRENTHOUSE each minute
// updates LOCALTEMP2 each 10 minutes
// NodeMCU ESP32, DHT22 temperature sensor
// Energy sensor board based on project: https://learn.openenergymonitor.org/electricity-monitoring/ct-sensors/how-to-build-an-arduino-energy-monitor-measuring-current-only?redirected=true

#include <ESP8266WiFi.h>   
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>



// wireless connection
char ssid[] = "xxxxxxxx";         // your SSID
char pass[] = "xxxxxxxxx";     // your SSID Password
//mysql connection
char user[] = "yyyyyyyyy";              // MySQL user login username
char password[] = "yyyyyyyyy";        // MySQL user login password




IPAddress server_addr_1(192,168,1,32);  // IP of the MySQL *server* here
IPAddress server_addr_2(192,168,1,33);
IPAddress server_addr_3(192,168,1,34);
IPAddress server_addr_4(192,168,1,40);




WiFiClient  client;
MySQL_Connection conn(&client);
MySQL_Cursor* cursor;


#include <DHT.h>
#include <DHT_U.h>
#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;                   // Create an instance
#include "DHT.h"
#define DHTPIN 2    // modify to the pin we connected
#define DHTTYPE DHT22   // AM2301 DHT21

DHT dht(DHTPIN, DHTTYPE);

double ampMinute[12] = {};
double averageMinute;
int i=0;
unsigned long time_start;
unsigned long time_exec;
int minutes=0;

void setup()
{
  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect. Needed for Leonardo only
  // Begin WiFi section
  Serial.printf("\nConnecting to %s", ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  // print out info about the connection:
  Serial.println("\nConnected to network");
  Serial.print("My IP address is: ");
  Serial.println(WiFi.localIP());
  //SQL
  Serial.print("Connecting to SQL...  ");
  if (conn.connect(server_addr_1, 3306, user, password))
    Serial.println("OK. ip local 32");
  else if (conn.connect(server_addr_2, 3306, user, password))
    Serial.println("OK. ip local 33");
  else if (conn.connect(server_addr_3, 3306, user, password))
    Serial.println("OK. ip local 34");
  else if (conn.connect(server_addr_4, 3306, user, password))
    Serial.println("OK. ip local 40");
   else 
    Serial.println("FAILED to connect to mysql");
  // create MySQL cursor object
  cursor = new MySQL_Cursor(&conn);
  //dht and energy setup
  dht.begin();
  emon1.current(0, 111.1);             // Current: input pin, calibration.
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void loop()   //run each secound
{
  time_start = millis();
  
  //ENERGY
  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  Serial.print("Power: ");
  Serial.print(Irms*230.0);         // Apparent power
  Serial.print("   Current: ");
  Serial.println(Irms);          // Irms
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) 
  {
    Serial.println("Failed to read from DHT");
  } 
  
  
  
  ampMinute[i]=Irms;
  i++;
  minutes++;
  
  if (i>=12)
    {
      //Serial.print("print");
      //insert database medium   to do
      double sum=0;
      for(int j = 0; j<12 ; j++)
      {
        //Serial.println(ampMinute[j]);
        sum+=ampMinute[j];
        ampMinute[j]=0;
      }
      averageMinute=sum/12;
      i=0;
      //Serial.println(averageMinute);
      //Serial.println(averageMinute*230);

      //change query here for temperature values
      String INSERT_SQL_CURRENT = "USE josenunes; INSERT INTO CURRENTHOUSE (date, CURRENT) VALUES (NOW(),"+String(averageMinute)+");";
      
      char charBuff[INSERT_SQL_CURRENT.length()+1];
      INSERT_SQL_CURRENT.toCharArray(charBuff,INSERT_SQL_CURRENT.length()+1);
      if (conn.connected())
        cursor->execute(charBuff);
      else 
        resetFunc();  //call reset
    }

  if (minutes>=10)
  {
      minutes=0;
      //upload temp and humidity to DB
      String INSERT_SQL_TEMP = "USE josenunes; INSERT INTO LOCALTEMP2 (date, temp, humidity) VALUES (NOW(),"+String(t)+","+String(h)+");";
      char charBuffT[INSERT_SQL_TEMP.length()+1];
      //char INSERT_SQL[]=
      INSERT_SQL_TEMP.toCharArray(charBuffT,INSERT_SQL_TEMP.length()+1);
    
      if (conn.connected())
        cursor->execute(charBuffT);
      else 
        resetFunc();  //call reset
      
  }
  
  //Use it for debugging
  Serial.println("time to execute the looop");
  Serial.println(millis()-time_start);
  delay(5000-(millis()-time_start));
}
