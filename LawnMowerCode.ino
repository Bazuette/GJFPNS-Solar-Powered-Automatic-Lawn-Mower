#include <BTS7960.h>
#include <NewPing.h>


// Load Wi-Fi library
#include <WiFi.h>
// Load Dc Motor Library
#include <L298N.h>

// Replace with your network credentials
const char* ssid     = "GJFPNS Automatic Lawn Mower";
const char* password = "GJFPNS123";

// Set web server port number to 80
WiFiServer server(80);
NewPing frontSensor(4, 4, 500);
NewPing rightSensor(2,2,500);
NewPing leftSensor(15,15,500);
// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String leftState = "off";
String rightState = "off";
String forwardState = "off";
String backwardState = "off";
String cutterState = "off";
String autoState = "off";
bool automatic = false;

//motor instances
BTS7960 motor1(23, 22, 21, 19);
BTS7960 cutter(26, 33, 25, 32);

BTS7960 motor2(12, 13, 5, 18);
//Task
TaskHandle_t autoTask;
// Parameters
int turnSpeed = 255;
int robotSpeed = 255;
int obstacleDistance = 20;

void setup() {
  xTaskCreatePinnedToCore(
      autoTaskCode, /* Function to implement the task */
      "autoTask", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &autoTask,  /* Task handle. */
      0); /* Core where the task should run */
  delay(500);
  Serial.begin(9600);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
  
}

void loop(){
  motor1.Enable();
  motor2.Enable();
  cutter.Enable();
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /leftState/on") >= 0) {
              Serial.println("left ON");
              leftState = "on";
              resetMovement("leftState");
            } else if (header.indexOf("GET /leftState/off") >= 0) {
              Serial.println("left OFF");
              leftState = "off";
              resetMovement("leftState");
            } else if (header.indexOf("GET /rightState/on") >= 0) {
              Serial.println("right ON");
              rightState = "on";
              resetMovement("rightState");
            } else if (header.indexOf("GET /rightState/off") >= 0) {
              Serial.println("right OFF");
              rightState = "off";
              resetMovement("rightState");
            } else if (header.indexOf("GET /forwardState/on") >= 0) {
              Serial.println("forward ON");
              forwardState = "on";
              resetMovement("forwardState");
            } else if (header.indexOf("GET /forwardState/off") >= 0) {
              Serial.println("forward OFF");
              forwardState = "off";
              resetMovement("forwardState");
            } else if (header.indexOf("GET /backwardState/on") >= 0) {
              Serial.println("backward ON");
              backwardState = "on";
              resetMovement("backwardState");
            } else if (header.indexOf("GET /backwardState/off") >= 0) {
              Serial.println("backward OFF");
              backwardState = "off";
              resetMovement("backwardState");
            } else if (header.indexOf("GET /cutterState/off") >= 0) {
              Serial.println("cutter OFF");
              cutterState = "off";
              cutter.TurnRight(0);
            } else if (header.indexOf("GET /cutterState/on") >= 0) {
              Serial.println("cutter on");
              cutterState = "on";
              cutter.TurnRight(100);
            } else if (header.indexOf("GET /automatic/off") >= 0){
              automatic = false;
              stopDCMotors();
            } else if (header.indexOf("GET /automatic/on") >= 0){
              automatic = true;
              stopDCMotors();
            } else if (header.indexOf("GET /automatic/autostate/on") >= 0 ){
              autoState = "on";
            } else if (header.indexOf("GET /automatic/autostate/off") >= 0 ){
              autoState = "off";
              stopDCMotors();
            }
          
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 35px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Lawn Mower Controls</h1>");
            
            if (!automatic){
              // Display current state, and ON/OFF buttons for GPIO 26  
              client.println("<p>Turn Left </p>");
              // If the output26State is off, it displays the ON button       
              if (leftState=="on") {
                client.println("<p><a href=\"/leftState/off\"><button class=\"button\">ON</button></a></p>");
              } else {
                client.println("<p><a href=\"/leftState/on\"><button class=\"button button2\">OFF</button></a></p>");
              } 

              client.println("<p>Turn Right </p>");
              // If the output26State is off, it displays the ON button       
              if (rightState=="on") {
                client.println("<p><a href=\"/rightState/off\"><button class=\"button\">ON</button></a></p>");
              } else {
                client.println("<p><a href=\"/rightState/on\"><button class=\"button button2\">OFF</button></a></p>");
              } 

              client.println("<p>Go Forward </p>");
              if (forwardState=="on") {
                client.println("<p><a href=\"/forwardState/off\"><button class=\"button\">ON</button></a></p>");
              } else {
                client.println("<p><a href=\"/forwardState/on\"><button class=\"button button2\">OFF</button></a></p>");
              } 
              client.println("<p>Go Backward </p>");
              if (backwardState=="on") {
                client.println("<p><a href=\"/backwardState/off\"><button class=\"button\">ON</button></a></p>");
              } else {
                client.println("<p><a href=\"/backwardState/on\"><button class=\"button button2\">OFF</button></a></p>");
              } 
            } else {
              client.println("<p>Auto Status</p>");
              if (autoState=="on") {
                client.println("<p><a href=\"/automatic/autostate/off\"><button class=\"button\">ON</button></a></p>");
              } else {
                client.println("<p><a href=\"/automatic/autostate/on\"><button class=\"button button2\">OFF</button></a></p>");
              } 
            }
            client.println("<p>Cutter </p>");
            if (cutterState=="on") {
              client.println("<p><a href=\"/cutterState/off\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/cutterState/on\"><button class=\"button button2\">OFF</button></a></p>");
            } 
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection

  }
  
  
}
  void resetMovement(String state) {
  if (state == "leftState"){
      rightState = "off";
      forwardState = "off";
      backwardState = "off";
    } else if (state == "rightState"){
      leftState = "off";
      forwardState = "off";
      backwardState = "off";
    } else if (state == "forwardState"){
      backwardState = "off";
      leftState = "off";
      rightState = "off";
    } else if (state == "backwardState"){
      forwardState = "off";
      leftState = "off";
      rightState = "off";
    }

  
}
  void turnRight(){
    motor1.TurnLeft(turnSpeed);
    motor2.TurnRight(turnSpeed);
    Serial.print("VROOM RIGHT");
  }
  void turnLeft(){
    motor2.TurnLeft(turnSpeed);
    motor1.TurnRight(turnSpeed);
    Serial.print("VROOM LEFT");
  }
  void backward(){
    motor1.TurnLeft(robotSpeed);
    motor2.TurnLeft(robotSpeed);
    Serial.print("VROOM BACKWARD");
  }
  void forward(){
    motor1.TurnRight(robotSpeed);
    motor2.TurnRight(robotSpeed);
    Serial.print("VROOM FORWARD");
  }
  void stopDCMotors(){
    motor1.Stop();
    motor2.Stop();
  }
  void autoTaskCode( void * parameter) {
    for(;;) {
      if (!automatic) {
        if(leftState == "off" && rightState == "off" && forwardState == "off" && backwardState == "off") {
        stopDCMotors();
        } 
          if (leftState == "on") {
            turnLeft();
          }
          if (rightState == "on") {
            turnRight();
          }
          if (forwardState == "on") {
            forward();
          }
          if (backwardState == "on") {
            backward();
          }
      } else if (automatic && (autoState == "on")){
        delay(50);
        Serial.println(frontSensor.ping_cm());
        Serial.println(leftSensor.ping_cm());
        Serial.println(rightSensor.ping_cm());
          if (frontSensor.ping_cm() > obstacleDistance){
            if (rightSensor.ping_cm() > obstacleDistance){
                if (leftSensor.ping_cm() > obstacleDistance){
                  forward();
                } else {
                  Serial.println("Left Sensor Obstacle");
                  stopDCMotors();
                  delay(400);
                  backward();
                  delay(700);
                  stopDCMotors();
                  delay(200);
                  turnRight();
                  delay(400);
                  stopDCMotors();
                  delay(300);
                }
            } else {
              Serial.println("Right Sensor Obstacle");
              stopDCMotors();
              delay(400);
              backward();
              delay(700);
              stopDCMotors();
              delay(200);
              turnLeft();
              delay(400);
              stopDCMotors();
              delay(300);
            }
          } else {
            Serial.println("Front Sensor Obstacle");
            stopDCMotors();
            delay(400);
            backward();
            delay(700);
            stopDCMotors();
            delay(200);
            turnRight();
            delay(4000);
            stopDCMotors();
            delay(300);
          }
        
        
        }
        if (automatic && (autoState == "off")){
          stopDCMotors();
        }
    }
  }
