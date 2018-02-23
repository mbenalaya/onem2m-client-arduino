#include <ESP8266WiFi.h>
#include <SimpleDHT.h>
#include "Timer.h"

Timer t;
int pinDHT11 = 2;
SimpleDHT11 dht11;

const char* ssid = "Pi3-AP";
const char* password = "raspberry";


const char* host = "192.168.0.2";
const int httpPort = 8080;
const char* origin   = "Cae-nodemcu";

WiFiServer server(80);

void setup() {

  Serial.begin(115200);
  delay(10);

 // prepare GPIO2
  pinMode(5, OUTPUT);
  digitalWrite(5, 0);
  pinMode(4, OUTPUT);
  digitalWrite(4, 0);
  
  // We start by connecting to a WiFi network
  randomSeed(analogRead(0));

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  
  server.begin();
  Serial.println("Server started");
  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  String resulat = send("/~/server/server",2,"{\"ae\":{\"rn\":\"MYDEVICE\",\"api\":\"app-test\",\"rr\":\"true\",\"poa\":[\"http://"+WiFi.localIP().toString()+":80\"]}}");
  
  if(resulat=="HTTP/1.1 201 Created"){
    send("/~/server/server/MYDEVICE",3,"{\"cnt\":{\"rn\":\"TEMPERATURE\"}}");
    send("/~/server/server/MYDEVICE/TEMPERATURE",3,"{\"cnt\":{\"rn\":\"DATA\"}}");

    send("/~/server/server/MYDEVICE",3,"{\"cnt\":{\"rn\":\"HUMIDITY\"}}");
    send("/~/server/server/MYDEVICE/HUMIDITY",3,"{\"cnt\":{\"rn\":\"DATA\"}}");

    send("/~/server/server/MYDEVICE",3,"{\"cnt\":{\"rn\":\"LED\"}}");
    send("/~/server/server/MYDEVICE/LED",3,"{\"cnt\":{\"rn\":\"DATA\"}}");

    push();
    
    String dataled = "{\"cin\":{\"con\":\"<obj><str val='Led' name='type'/><bool val='false' name='value'/></obj>\"}}";
    send("/~/server/server/MYDEVICE/LED/DATA",4,dataled);

  }
  t.every(1000*60, push);
}

void loop(){
  t.update();
    // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  int val;
  int gpio;
  String state;
  String type;
  String cnt;
  if (req.indexOf("false") != -1){
    gpio= 5;
    val = 0;
    state="false";
    type="Led";
    cnt="LED";
  }else if (req.indexOf("true") != -1){
    gpio= 5;
    val = 1;
    state="true";
    type="Led";
    cnt="LED";
  }else {
    Serial.println("invalid request");
    client.stop();
    return;
  }

  // Set GPIO2 according to the request
  digitalWrite(gpio, val);

  String data = String()+"{\"cin\":{\"con\":\"<obj><str val='"+type+"' name='type'/><bool val='"+state+"' name='value'/></obj>\"}}";
  send(String()+"/~/server/server/MYDEVICE/"+cnt+"/DATA",4,data);
  
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
  s += (val)?"high":"low";
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");
  
}


void push(){
  byte temperature = 0;
  byte humidity = 0;

  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    Serial.println("Read DHT11 failed.");
    delay(1000);
    return;
  }
  Serial.print((int)temperature); Serial.print(" *C, "); 
  Serial.print((int)humidity); Serial.println(" %");

  String datat,datah;
  datat = String()+"{\"cin\":{\"con\":\"<obj><str val='Temperature' name='type'/><int val='"+(int)temperature+"' name='value'/><str val='Celsius' name='unit'/></obj>\"}}";
  datah = String()+"{\"cin\":{\"con\":\"<obj><str val='Humidity' name='type'/><int val='"+(int)humidity+"' name='value'/><str val='%' name='unit'/></obj>\"}}";
  send("/~/server/server/MYDEVICE/TEMPERATURE/DATA",4,datat);
  send("/~/server/server/MYDEVICE/HUMIDITY/DATA",4,datah);

}

String send(String url,int ty, String rep) {

  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return "error";
  }
 
  
  //Serial.print("Requesting URL: ");
  //Serial.println(url);
  
  // This will send the request to the server

  String req = String()+"POST " + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "X-M2M-Origin: " + origin + "\r\n" +
               "Content-Type: application/json;ty="+ty+"\r\n" +
               "Content-Length: "+ rep.length()+"\r\n"
               "Connection: close\r\n\n" + 
               rep;

  Serial.println(req+"\n");
  
  client.print(req);
               
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return "error";
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  String res="";
  if(client.available()){
    res = client.readStringUntil('\r');
    Serial.print(res);
  }
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");
  Serial.println();
  return res;
}


