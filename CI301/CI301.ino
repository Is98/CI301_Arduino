
#include <stdio.h>
#include <SPI.h>
#include <Ethernet.h>
#include <dht.h>
#include "sha1.h"
#include "mysql.h"

#define dht1 A0 //no ; here. Set equal to channel sensor is on
#define dht2 A1
#define dht3 A2
#define dht4 A3

#define Relay1 2
#define Relay2 3
#define Relay3 4
#define Relay4 5

dht DHT;

int scheduler = 0;
int web_priority = 32700; // 10000 is around 13 seconds. Max 32750 = 40 seconds

//Sensor Nicknames
String dht1NN = "Ambient Soil";
String dht2NN = "Outer Hood";
String dht3NN = "Plant Top";
String dht4NN = "Plant Center";

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress addr_me(192,168,0,66);
IPAddress addr_mysql(198,23,57,27);

char user[] = "iansmi9_ard";
char password[] = "arduino";
char SQL_POWERON_TEST[] = "INSERT INTO `iansmi9_ard`.`poweron` VALUES (NULL, CURRENT_TIMESTAMP, 'ok')";

/* Setup the MySQL connector */
Connector my_conn; // The Connector reference

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(66);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  pinMode(dht1, INPUT);
  pinMode(dht2, INPUT);
  pinMode(dht3, INPUT);
  pinMode(dht4, INPUT);
  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);
  pinMode(Relay4, OUTPUT);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, addr_me);
  server.begin();
  Serial.println("I am at ");
  Serial.println(Ethernet.localIP());

  //Test mysql connection by adding a poweron record
  if (my_conn.mysql_connect(addr_mysql, 3306, user, password))
  {
    delay(500);
    /* run the SQL query */
    my_conn.cmd_query(SQL_POWERON_TEST);
    Serial.println("Query Success!");
  } 
  else  {
    Serial.println("Connection failed.");
  }

}



void loop() {

  switch (scheduler) {

  case 0:
    WebServer();
    break;
  case 1:
    DHT.read11(dht1);
    sendReadings(1, DHT.humidity, DHT.temperature);
    break;
  case 2:
    DHT.read11(dht2);
    sendReadings(2, DHT.humidity, DHT.temperature);
    break;
  case 3:
    DHT.read11(dht3);
    sendReadings(3, DHT.humidity, DHT.temperature);
  case 4:
    DHT.read11(dht4);
    sendReadings(4, DHT.humidity, DHT.temperature);

  }

}





void WebServer() {
  Serial.println("Broadcasting ! ");

  // Progress scheduler
  for (int webtimeslice = 0; webtimeslice < web_priority; webtimeslice++) 
  {
    // listen for incoming clients
    EthernetClient client = server.available();
    if (client) {
      Serial.println("new client");
      // an http request ends with a blank line
      boolean currentLineIsBlank = true;
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          Serial.write(c);
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
            //and CSS style data...
            client.println("<style type=\"text/css\">");
            client.println("container {");
            client.println("position: relative;");
            client.println("width: 400px;");
            client.println("margin-left:auto;");
            client.println("margin-right:auto;");
            client.println("}");
            client.println("reading {");
            client.println("float: left;");
            client.println("width: 100px;");
            client.println("font-family: \"MS Serif\", \"New York\", serif;");
            client.println("font-size: 12px;");
            client.println("text-align: center;");
            client.println("padding: 20px, 20px. 20px, 20px;");
            client.println("}");
            client.println("reading h1 {");
            client.println("font-family: Georgia, \"Times New Roman\", Times, serif;");
            client.println("font-size: 14px;");
            client.println("font-style: oblique;");
            client.println("}");

            client.println("</style>");
            client.println("</head>");
            //and body content.
            client.println("<body>");
            client.println("<container>");

            // output the value of each sensor

            DHT.read11(dht1);
            client.print("<reading><h1>");
            client.print(dht1NN);
            client.print("</h1> <br /> Humidity <br /> ");
            client.print(DHT.humidity);
            client.print("%.  <br /><br />   Temperature <br /> ");
            client.print(DHT.temperature);
            client.println("C </reading>");      

            DHT.read11(dht2);
            client.print("<reading><h1>");
            client.print(dht2NN);
            client.print("</h1> <br /> Humidity <br /> ");
            client.print(DHT.humidity);
            client.print("%.  <br /><br />   Temperature <br /> ");
            client.print(DHT.temperature);
            client.println("C </reading>");    

            DHT.read11(dht3);
            client.print("<reading><h1>");
            client.print(dht3NN);
            client.print("</h1> <br /> Humidity <br /> ");
            client.print(DHT.humidity);
            client.print("%.  <br /><br />   Temperature <br /> ");
            client.print(DHT.temperature);
            client.println("C </reading>");    

            DHT.read11(dht4);
            client.print("<reading><h1>");
            client.print(dht4NN);
            client.print("</h1> <br /> Humidity <br /> ");
            client.print(DHT.humidity);
            client.print("%.  <br /><br />   Temperature <br /> ");
            client.print(DHT.temperature);
            client.println("C </reading>");    
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
      Serial.println("client disonnected");
    }
  delay(1);
  }
  scheduler += 1;
}






void sendReadings(int sensornum, int humidity, int temp) { 
  //tell the serial monitor what we're doing.
  char printf1[50];
  sprintf(printf1, "TRYING TO INSERT SQL TO '%d', '%d', '%d'", sensornum, humidity, temp);
  Serial.println(printf1);

  //build the query, correcting any variable usage/data type issues
  char SQL_SEND_READINGS[100];
  sprintf(SQL_SEND_READINGS, "INSERT INTO iansmi9_ard.log VALUES (NULL, CURRENT_TIMESTAMP, '%d', '%d', '%d')", sensornum, humidity, temp);

  /* run the SQL query */
  my_conn.cmd_query(SQL_SEND_READINGS);

  //
  if (scheduler == 4) {
    scheduler = 0;
  } 
  else {
    scheduler++;
  }
}










