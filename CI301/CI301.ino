#include <EasyScheduler.h>

#include <WebServer.h>
#include <dht.h>

//#include <stdio.h>
#include <SPI.h>
#include <Ethernet.h>

#include "sha1.h"
#include "mysql.h"

#define PREFIX "/arduino"

//Pin references
int pin_dht[4] = {54, 55, 56, 57}; //{A0,A1,A2,A3}
int temps[4] = {0, 0, 0, 0};
boolean dimmable[] = {1, 1, 1, 1};
int desired_temp[] = {18, 18, 18, 18};
int desired_tolerance[] = {8, 8, 8, 8} ;
boolean relayHeat[] = {0, 0, 0, 0};
//int minimum_dim[] = {0, 0, 0, 0}; //0 - 255

//Sensor nicknames
char* dhtNN[4] = {
  "Ambient Soil",       // DHT1
  "Outer Hood",         // DHT2
  "Plant Top",          // DHT3
  "Plant Center"
};     // DHT4

dht DHT;

long priority_web = 2000000;   // 10,000,000 = 15 minutes
int priority_send = 10;          // 10 = 1.5 minutes = 90 secs
int priority_switch = 100;       // 60 = every 15 seconds

long allocation_time = priority_web;
long allocation_send[1];
long allocation_switch[25];



// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress addr_me(192, 168, 0, 66);
IPAddress addr_mysql(198, 23, 57, 27);

char user[] = "iansmi9_ard";
char password[] = "arduino";
char SQL_POWERON_TEST[] = "INSERT INTO `iansmi9_ard`.`poweron` VALUES (NULL, CURRENT_TIMESTAMP, 'ok')";

/* Setup the MySQL connector */
Connector my_conn; // The Connector reference

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
/* This creates an instance of the webserver.  By specifying a prefix
* of "", all pages will be at the root of the server. */
#define PREFIX ""
EthernetServer server(80);


/*
void privateCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{

}
*/


/*
void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  server.httpSuccess();
  if (type != WebServer::HEAD)
  {
    //P(helloMsg) = "<h1>Hello, World!</h1><a href=\"private.html\">Private page</a>";


    // send a standard http response header
    server.println("HTTP/1.1 200 OK");
    server.println("Content-Type: text/html");
    server.println("Connection: close");  // the connection will be closed after completion of the response
    server.println("Refresh: 60");  // refresh the page automatically every 5 sec
    server.println();

    //and metadata...
    server.println("<!DOCTYPE HTML>");
    server.println("<html>");
    server.println("<head>");
    server.println("<title>Environment Live Times</title>");
    server.println("<link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"http://arduino.cc/en/favicon.png\" />");

    //and CSS style data...
    server.println("<style type=\"text/css\">");
    server.println("  container {");
    server.println("    position: relative;");
    server.println("    width: 400px;");
    server.println("    margin-left:auto;");
    server.println("    margin-right:auto;");
    server.println("  }");
    server.println("  reading {");
    server.println("    float: left;");
    server.println("    width: 100px;");
    server.println("    font-family: \"MS Serif\", \"New York\", serif;");
    server.println("    font-size: 12px;");
    server.println("    text-align: center;");
    server.println("    padding: 20px, 20px. 20px, 20px;");
    server.println("  }");
    server.println("  reading h1 {");
    server.println("    font-family: Georgia, \"Times New Roman\", Times, serif;");
    server.println("    font-size: 14px;");
    server.println("    font-style: oblique;");
    server.println("  }");

    server.println("</style>");
    server.println("</head>");
    //and body content.
    server.println("<body>");
    server.println("<container>");

    // output the value of each sensor

    for (int i = 0; i < 4; i++) {
      DHT.read11(pin_dht[i]);
      server.print("<reading><h1>");
      server.print(dhtNN[i]);
      server.print("</h1><br />Humidity<br /> ");
      server.print(DHT.humidity);
      server.print("%.<br /><br />Temperature<br /> ");
      server.print(DHT.temperature);
      server.println("C</reading>");
    }

    server.println("</container>");

    server.println("</body>");
    server.println("</html>");


    //server.printP(helloMsg);
  }
} */



void sendReadings() {

  int sensorvalue[8];
  //fill the sensorvalue array
  for (int i = 0; i < 4; i++) {
    DHT.read11(pin_dht[i]);
    sensorvalue[i * 2] = DHT.humidity;
    sensorvalue[(i * 2) + 1] = DHT.temperature;

  }

  //build the query, correcting any variable usage/data type issues
  char SQL_SEND_READINGS[128];
  sprintf(SQL_SEND_READINGS, "INSERT INTO iansmi9_ard.log VALUES (NULL, CURRENT_TIMESTAMP, '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d');",
          sensorvalue[0], sensorvalue[1], sensorvalue[2], sensorvalue[3],
          sensorvalue[4], sensorvalue[5], sensorvalue[6], sensorvalue[7]);

  /* run the SQL query */
  my_conn.cmd_query(SQL_SEND_READINGS);
  Serial.println(SQL_SEND_READINGS);
}



void relaySwitch(int i) {

  //Serial.println("In RelaySwitch() ");

  int DHTnum = i;
  DHT.read11(pin_dht[DHTnum]);
  int temp = DHT.temperature;


  if ((temp > 0 && temp < 60) && (temp != temps[DHTnum])) {

    if (dimmable[DHTnum]) {
      int minimum_dim = 0;
      //Tell MySQL
      char SQL_SEND_READINGS[100];
      sprintf(SQL_SEND_READINGS, "INSERT INTO iansmi9_ard.log_relays VALUES (NULL, CURRENT_TIMESTAMP, '%d', '2');", DHTnum);
      my_conn.cmd_query(SQL_SEND_READINGS);
      //Serial.println();
      //Find the PWM value
      double pwmNumerator = (double) (temp - (desired_temp[DHTnum] - (desired_tolerance[DHTnum] / 2)  ) );
      double pwmDecimal = pwmNumerator / desired_tolerance[DHTnum];
      if (pwmDecimal < 0) {
        pwmDecimal = 0.00;
      };
      if (pwmDecimal > 1) {
        pwmDecimal = 1.00;
      };
      //Serial.println(pwmDecimal);
      //Serial.println(minimum_dim);
      int pwmMaximum = (255 - minimum_dim);
      //Serial.println(pwmMaximum);
      double dim_component = pwmMaximum * pwmDecimal ;
      //Serial.println(dim_component);
      int pwmValue = (int) minimum_dim + dim_component;
      //Serial.println(pwmValue);

      //Write this to arduino 2
      char toSend[12];
      sprintf(toSend, "#%d %03d$", DHTnum, pwmValue);
      Serial.write(toSend);
      //Tell Serial monitor
      char toTell[36];
      sprintf(toTell, "   Sensor %d has temperature %3d! PWM: %d!", DHTnum, temp, pwmValue);
      Serial.println(toTell);
      temps[i] = temp;
    }
    else if ((!dimmable) && (temp > (desired_temp[DHTnum] + ((int) (desired_tolerance[DHTnum] / 2)) )) ) {
      //Tell MySQL
      //build the query, correcting any variable usage/data type issues
      char SQL_SEND_READINGS[88];
      sprintf(SQL_SEND_READINGS,
              "INSERT INTO iansmi9_ard.log_relays VALUES (NULL, CURRENT_TIMESTAMP, '%d', '1');", DHTnum);
      /* run the SQL query */
      my_conn.cmd_query(SQL_SEND_READINGS);
      Serial.println(SQL_SEND_READINGS);
      //Write this to arduino 2
      char toSend[7];
      sprintf(toSend, "#%d 255$", DHTnum);
      Serial.write(toSend);
      //Tell Serial Monitor
      Serial.print("Too high on sensor: ");
      Serial.print(DHTnum);
      Serial.println(".");
      Serial.print(temp);
      Serial.print(" - Turning Cooling ON - ");
      Serial.println(DHTnum);
      temps[i] = temp;
    }
    else if ((!dimmable) && (temp < (desired_temp[DHTnum] - ((int) (desired_tolerance[DHTnum] / 2)) )) ) {
      //Tell MySQL
      char SQL_SEND_READINGS[88];
      sprintf(SQL_SEND_READINGS,
              "INSERT INTO iansmi9_ard.log_relays VALUES (NULL, CURRENT_TIMESTAMP, '%d', '0');", DHTnum);
      /* run the SQL query */
      my_conn.cmd_query(SQL_SEND_READINGS);
      Serial.println(SQL_SEND_READINGS);
      //Write this to arduino 2
      char toSend[7];
      sprintf(toSend, "#%d 000$", DHTnum);
      Serial.write(toSend);
      //Tell Serial Monitor
      Serial.print("Too low on sensor: ");
      Serial.print(i);
      Serial.println(".");
      Serial.print(temp);
      Serial.print(" - Turning Cooling OFF - ");
      Serial.println(DHTnum);
      temps[i] = temp;
    }
  }
  else if (! ( temp > 0 && temp < 60)) {
    Serial.print(" -- OUTLYING TEMPERATURE - ");
    Serial.println(temp);
  }

}



void setup() {

  // Open serial communications and wait for port to open:
  Serial.begin(115200);


  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.println();
  Serial.println("Hello, I'm about to start! One minute :)");
  Serial.println();

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Serial.println("Retrying!!");
    setup();
  }

  Ethernet.begin(mac, addr_me);
  //webserver.setDefaultCommand(&defaultCmd);
  //webserver.addCommand("index.html", &defaultCmd);
  //webserver.addCommand("private.html", &privateCmd);
  //webserver.begin();

  server.begin();

  // print your local IP address:
  Serial.print("My IP address is ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();


  //Test mysql connection by adding a poweron record
  if (my_conn.mysql_connect(addr_mysql, 3306, user, password))
  {
    //delay(15);
    // run the SQL query
    my_conn.cmd_query(SQL_POWERON_TEST);
    Serial.println("Power up query success!");
  }
  else  {
    Serial.println("Connection failed. Retrying...");
    setup();
  }

  Serial.println();

  for (int i = 0; i < priority_send; i++) {
    allocation_send[i] = (allocation_time * (i + 1) ) / priority_send;
  }

  Serial.print("Sending every ");
  Serial.print( allocation_send[0] / 10000 );
  Serial.println(" seconds");

  for (int i = 0; i < priority_switch; i++) {
    allocation_switch[i] = (allocation_time * (i + 1) ) / priority_switch;
  }

  Serial.print("Switching every ");
  Serial.print( allocation_switch[0] / 10000 );
  Serial.println(" seconds");
  Serial.println();

  for (int j = 0; j < 4; j++) {
    pinMode(pin_dht[j], INPUT);
  }



}



void loop() {
  for (int i = 0; i < 0; i++);
  {
    //Serial.println("Inside LOOP");
    int allocation_sendcount = 0;
    int allocation_switchcount = 0;

    for (long timeslice = 0; timeslice < priority_web; timeslice++)
    {
      WebServer();

      if (allocation_send[allocation_sendcount] == (timeslice + 1)) {
        sendReadings();
        allocation_sendcount++;
      }

      if (allocation_switch[allocation_switchcount] == (timeslice + 1)) {
        relaySwitch(0);
        relaySwitch(1);
        relaySwitch(2);
        relaySwitch(3);
        allocation_switchcount++;
      }
      /*
        char buff[64];
        int len = 64;

        // process incoming connections one at a time forever
        webserver.processConnection(buff, &len);
      */
    }

  }
}






void WebServer() {

  // listen for incoming clients
  EthernetClient client = server.available();

  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 60");  // refresh the page automatically every 5 sec
          client.println();

          //and metadata...
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>Environment Live Times</title>");
          client.println("<link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"http://arduino.cc/en/favicon.png\" />");

          //and CSS style data...
          client.println("<style type=\"text/css\">");
          client.println("  container {");
          client.println("    position: relative;");
          client.println("    width: 400px;");
          client.println("    margin-left:auto;");
          client.println("    margin-right:auto;");
          client.println("  }");
          client.println("  reading {");
          client.println("    float: left;");
          client.println("    width: 100px;");
          client.println("    font-family: \"MS Serif\", \"New York\", serif;");
          client.println("    font-size: 12px;");
          client.println("    text-align: center;");
          client.println("    padding: 20px, 20px. 20px, 20px;");
          client.println("  }");
          client.println("  reading h1 {");
          client.println("    font-family: Georgia, \"Times New Roman\", Times, serif;");
          client.println("    font-size: 14px;");
          client.println("    font-style: oblique;");
          client.println("  }");

          client.println("</style>");
          client.println("</head>");
          //and body content.
          client.println("<body>");
          client.println("<container>");

          // output the value of each sensor

          for (int i = 0; i < 4; i++) {
            DHT.read11(pin_dht[i]);
            client.print("<reading><h1>");
            client.print(dhtNN[i]);
            client.print("</h1><br />Humidity<br /> ");
            client.print(DHT.humidity);
            client.print("%.<br /><br />Temperature<br /> ");
            client.print(DHT.temperature);
            client.println("C</reading>");
          }

          client.println("</container>");

          client.println("</body>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();

  }
}








//void RelaySendValue(int RelayPin, )
//Serial.write("#2 345$");
