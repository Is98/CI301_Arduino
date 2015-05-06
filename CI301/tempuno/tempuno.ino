#include <dht.h>
#define input 0
//main board = 0;
//relays are at 6, 7, 8, 9;
int relayPins[4] = {6, 7, 8, 9};
dht DHT;
int desiredtemp = 25;
int tolerance = 2;
int dimValues[4] = {200, 100, 150, 0};

void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(input, INPUT);
  for (int i = 0; i < 4 ; i++ ) {
    pinMode(relayPins[i], OUTPUT);
  }
  Serial.println("STARTED");
}

void loop() {

  //If there is an input

  char data = Serial.read();
  String dataString;
  if (data == '#') {
    //Serial.println("Start");
    data = Serial.read();
    //print until stop character found
    for (int i = 0; (data != '$'); data = Serial.read()) {
      if (data > -1) {
        dataString += data;
        //Serial.print(data);
      }
    }
    //Serial.println(dataString);
    //Serial.println("STOP");

    // get the value from input
    int relay = dataString.charAt(0) - '0';
    char inputchars[sizeof(dataString)] {dataString.charAt(2), dataString.charAt(3), dataString.charAt(4)};
    String valuestring = inputchars;
    int value = valuestring.toInt();
    //output what we're changing
    String printme = "Dimming Relay: ";
    printme += relay;
    printme += " to Value: ";
    printme += value;
    Serial.println(printme);
    //Set the value of that relay.
    analogWrite( relay, value );

  }

}




//int pwm = ( (temp - (desiredtemp - tolerance) / (2 * tolerance) ) * 255);
