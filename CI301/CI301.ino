
#include <stdio.h>
#include <SPI.h>
#include <Ethernet.h>
#include <dht.h>
#include "sha1.h"
#include "mysql.h"

//Pin references 
int pin_relay[] = {
  2,3,4,5};
int pin_dht[] = {
  54,55,56,57}; //no ; here. Set equal to channel sensor is on {A0,A1,A2,A3}

//Sensor nicknames
String dhtNN[] = { 
  "Ambient Soil", "Outer Hood", "Plant Top", "Plant Center"};

dht DHT;

int web_priority = 25000; // 10000 is around 13 seconds. Max 32750 = 40 seconds

int relay1_on_temp = 23;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
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

  for (int j=0; j < 4; j++) {
    pinMode(pin_dht[j], INPUT);
    pinMode(pin_relay[j], OUTPUT);
  }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, addr_me);
  server.begin();
  Serial.println("I am at ");
  Serial.println(Ethernet.localIP());

  //Test mysql connection by adding a poweron record
  if (my_conn.mysql_connect(addr_mysql, 3306, user, password))
  {
    delay(5);
    /* run the SQL query */
    my_conn.cmd_query(SQL_POWERON_TEST);
    Serial.println("Query Success!");
  } 
  else  {
    Serial.println("Connection failed.");
  }
}



void loop() {

    WebServer();
    sendReadings();
    relaySwitch();
}





void WebServer() {
  Serial.println("Broadcasting ! ");

  // Progress scheduler
  for (int webtimeslice = 0; webtimeslice < web_priority; webtimeslice++) 
  {
    // listen for incoming clients
    EthernetClient client = server.available();
    if (client) {
      Serial.println();
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
            client.println("<link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"http://arduino.cc/en/favicon.png\" />");

            //and google chart script...
            client.println("<script type=\"text/javascript\" src=\"https://www.google.com/jsapi\"></script>");
            client.println("<script type=\"text/javascript\">");
            client.println("  google.load(\"visualization\", \"1\", {packages:[\"corechart\"]});");
            client.println("  google.setOnLoadCallback(drawChart);");
            client.println("  function drawChart() {");
            client.println("    var data = google.visualization.arrayToDataTable([");
            client.println("      ['Year', 'Sensor_1_Temp', 'Sensor_2_Temp', 'Sensor_3_Temp', 'Sensor_4_Temp'],");



            // SELECT query for lookup value (1 row returned)
            // Here we get a value from the database and use it.
        /*    int temperature = 0;
            char GETTEMP[];
            sprintf(GETTEMP, "")
            my_conn.cmd_query(QUERY_POP);
            // We ignore the columns but we have to read them to get that data out of the queue
            my_conn.get_columns();
            // Now we read the rows.
            row_values *row = NULL;
            do {
              row = my_conn.get_next_row();
              // We use the first value returned in the row - population of NYC!
              if (row != NULL) {
                head_count = atol(row->values[0]);
              }
            } 
            while (row != NULL);
            // We're done with the buffers so Ok to clear them (and save precious memory).
            my_conn.free_columns_buffer();
            my_conn.free_row_buffer();
            // Now, let's do something with the data.
            Serial.print("NYC pop = ");
            Serial.println(head_count);

*/

            /*
             client.println("      ['2004',      23,         26,       41,       40],");
             client.println("      ['2005',      23,         27,       40,       41],");
             client.println("      ['2006',      22,         27,       39,       42],");
             client.println("      ['2007',      23,         26,       38,       40]");
             */


            client.println("    ]);");
            client.println("");
            client.println("    var options = {");
            client.println("      title: 'Sensor Readings'");
            client.println("    };");
            client.println("");
            client.println("    var chart = new google.visualization.LineChart(document.getElementById('chart_div'));");
            client.println("");
            client.println("    chart.draw(data, options);");
            client.println("  }");
            client.println("</script>");
            client.println("");

            //and CSS style data...
            client.println("<style type=\"text/css\">");
            client.println("container {");
            client.println("  position: relative;");
            client.println("  width: 400px;");
            client.println("  margin-left:auto;");
            client.println("  margin-right:auto;");
            client.println("}");
            client.println("reading {");
            client.println("  float: left;");
            client.println("  width: 100px;");
            client.println("  font-family: \"MS Serif\", \"New York\", serif;");
            client.println("  font-size: 12px;");
            client.println("  text-align: center;");
            client.println("  padding: 20px, 20px. 20px, 20px;");
            client.println("}");
            client.println("reading h1 {");
            client.println("  font-family: Georgia, \"Times New Roman\", Times, serif;");
            client.println("  font-size: 14px;");
            client.println("  font-style: oblique;");
            client.println("}");
            client.println("</style>");

            client.println("</head>");

            //and body content.
            client.println("<body>");
            client.println("<container>");

            // output the value of each sensor

            DHT.read11(pin_dht[0]);
            client.print("<reading><h1>");
            client.print(dhtNN[0]);
            client.print("</h1> <br /> Humidity <br /> ");
            client.print(DHT.humidity);
            client.print("%.  <br /><br />   Temperature <br /> ");
            client.print(DHT.temperature);
            client.println("C </reading>");      

            DHT.read11(pin_dht[1]);
            client.print("<reading><h1>");
            client.print(dhtNN[1]);
            client.print("</h1> <br /> Humidity <br /> ");
            client.print(DHT.humidity);
            client.print("%.  <br /><br />   Temperature <br /> ");
            client.print(DHT.temperature);
            client.println("C </reading>");    

            DHT.read11(pin_dht[2]);
            client.print("<reading><h1>");
            client.print(dhtNN[2]);
            client.print("</h1> <br /> Humidity <br /> ");
            client.print(DHT.humidity);
            client.print("%.  <br /><br />   Temperature <br /> ");
            client.print(DHT.temperature);
            client.println("C </reading>");    

            DHT.read11(pin_dht[3]);
            client.print("<reading><h1>");
            client.print(dhtNN[3]);
            client.print("</h1> <br /> Humidity <br /> ");
            client.print(DHT.humidity);
            client.print("%.  <br /><br />   Temperature <br /> ");
            client.print(DHT.temperature);
            client.println("C </reading>");  

            client.println("<br /><br />");
            client.println("<div id=\"chart_div\" style=\"width: 900px; height: 500px; position: absolute; top: 200px;\"></div>");


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
}






void sendReadings() { 
  //tell the serial monitor what we're doing.
  Serial.println(" SQL INSERT ");
  
  int sensorvalue[8];
  //fill the sensorvalue array
  for (int i=0; i < 4; i++) {
    DHT.read11(pin_dht[i]);
    sensorvalue[i*2] = DHT.humidity;
    sensorvalue[(i*2)+1] = DHT.temperature;
  }
  //build the query, correcting any variable usage/data type issues
  char SQL_SEND_READINGS[100];
  sprintf(SQL_SEND_READINGS, "INSERT INTO iansmi9_ard.log VALUES (NULL, CURRENT_TIMESTAMP, '%d', '%d', '%d',  '%d', '%d', '%d',  '%d', '%d')", 
        sensorvalue[0], sensorvalue[1], sensorvalue[2], sensorvalue[3], sensorvalue[4], sensorvalue[5], sensorvalue[6], sensorvalue[7]);

  /* run the SQL query */
  my_conn.cmd_query(SQL_SEND_READINGS);
}



void relaySwitch() {
  /* DHT.read11(relay1_dht);
   Serial.println(relay1_dht); */

  DHT.read11(pin_dht[2]);
  int temp = DHT.temperature;

  if (temp > 0 && temp < 60) {
    if (temp > relay1_on_temp) {
      if (digitalRead(pin_relay[0]) == HIGH) {
        digitalWrite(pin_relay[0], LOW);
        Serial.print(temp);
        Serial.print(" - Turning Relay 1 ON");
        //Tell MySQL
        char SQL_SEND_READINGS[] = "INSERT INTO iansmi9_ard.log_relays VALUES (NULL, CURRENT_TIMESTAMP, '1', '1');";
        my_conn.cmd_query(SQL_SEND_READINGS);
      }
    } 
    else if (temp < 21) { //relay1_off_temp
      if (digitalRead(pin_relay[0]) == LOW) {
        digitalWrite(pin_relay[0], HIGH);
        Serial.print(temp);
        Serial.print(" - Turning Relay 1 OFF");
        //Tell MySQL
        char SQL_SEND_READINGS[] = "INSERT INTO iansmi9_ard.log_relays VALUES (NULL, CURRENT_TIMESTAMP, '1', '0');";
        my_conn.cmd_query(SQL_SEND_READINGS);
      }
    } 
    /*else {
     Serial.print(" - I'm toasty warm");
     }*/
    Serial.println();
  } 
  else {
    Serial.print(" -- OUTLYING TEMPERATURE - "); 
    Serial.println(temp);
  }
}






