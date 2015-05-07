
//main board = 0;
//relays are at 6, 7, 8, 9;
int relayPins[4] = {6, 9, 10, 11};

void setup() {
  Serial.begin(115200);
  Serial.println();
  //pinMode(input, INPUT);
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
    char inputchars[3];
    sprintf(inputchars, "%d%d%d" , (dataString.charAt(2) - '0'), (dataString.charAt(3) - '0'), (dataString.charAt(4) - '0'));
    String valuestring = inputchars;
    int value = valuestring.toInt();
    //output what we're changing
    String printme = "Dimming Relay: ";
    printme += relay;
    printme += " to Value: ";
    printme += value;
    Serial.println(printme);
    if (value > 255) {
      value = 255;
    }
    else if (value < 0) {
      value = 0;
    }
    //Set the value of that relay.
    analogWrite( relayPins[relay], value );

  }

}

