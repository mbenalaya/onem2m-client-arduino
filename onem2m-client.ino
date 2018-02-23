#include <ESP8266WiFi.h>
 
// WIFI network
const char* ssid = "Pi3-AP";
const char* password = "raspberry";
 
// Target CSE
const char* host = "172.24.1.1";
const int httpPort = 8282;
 
// AE-ID
const char* origin   = "Clight_ae1";
 
// AE listening port
WiFiServer server(80);
 
void setup() {
 
  Serial.begin(115200);
  delay(10);
 
  // Configure pin 5 for LED control
  pinMode(5, OUTPUT);
  digitalWrite(5, 0);
 
  Serial.println();
  Serial.println();
 
  // Connect to WIFI network
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.persistent(false);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 
  // Start HTTP server
  server.begin();
  Serial.println("Server started");
 
  // Create AE resource
  String resulat = send("/~/gateway-1/gateway-1",2,"{\"m2m:ae\":{\"rn\":\"light_ae1\",\"acpi\":[\"/gateway-1/gateway-1/MN-CSEAcp\"],\"api\":\"light.company.com\",\"rr\":\"true\",\"poa\":[\"http://"+WiFi.localIP().toString()+":80\"]}}");
 
  if(resulat=="HTTP/1.1 201 Created"){
    // Create Container resource
    send("/~/gateway-1/gateway-1/light_ae1",3,"{\"m2m:cnt\":{\"rn\":\"light\"}}");
 
    // Create ContentInstance resource
    send("/~/gateway-1/gateway-1/light_ae1/light",4,"{\"m2m:cin\":{\"con\":\"OFF\"}}");
 
    // Create Subscription resource
    send("/~/gateway-1/gateway-1/light_ae1/light",23,"{\"m2m:sub\":{\"rn\":\"lightstate_sub\",\"nu\":[\"Clight_ae1\"],\"nct\":1}}");
  }
}
 
// Method in charge of receiving event from the CSE
void loop(){
  // Check if a client is connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
 
  // Read the request
  String req = client.readString();
  Serial.println(req);
  client.flush();
 
  // Switch the LED state according to the received value
  if (req.indexOf("ON") != -1){
    digitalWrite(5, 1);
  }else if (req.indexOf("OFF") != -1){
    digitalWrite(5, 0);
  }else{
    Serial.println("invalid request");
    client.stop();
    return;
  }
 
  client.flush();
 
  // Send HTTP response to the client
  String s = "HTTP/1.1 200 OK\r\n";
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");
 
}
 
 
// Method in charge of sending request to the CSE
String send(String url,int ty, String rep) {
 
  // Connect to the CSE address
  Serial.print("connecting to ");
  Serial.println(host);
 
  WiFiClient client;
 
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return "error";
  }
 
 
  // prepare the HTTP request
  String req = String()+"POST " + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "X-M2M-Origin: " + origin + "\r\n" +
               "Content-Type: application/json;ty="+ty+"\r\n" +
               "Content-Length: "+ rep.length()+"\r\n"
               "Connection: close\r\n\n" + 
               rep;
 
  Serial.println(req+"\n");
 
  // Send the HTTP request
  client.print(req);
 
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return "error";
    }
  }
 
  // Read the HTTP response
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
