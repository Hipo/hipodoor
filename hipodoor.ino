#include <Ethernet.h>
#include <Adafruit_VC0706.h>
#include <SPI.h>
#include <SoftwareSerial.h>         
#define chipSelect 4
#if ARDUINO >= 100
SoftwareSerial cameraconnection = SoftwareSerial(2, 3);
#else
NewSoftSerial cameraconnection = NewSoftSerial(2, 3);
#endif
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);
//ETHERNET
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "http://hipodoor-photos.s3.eu-central-1.amazonaws.com";
EthernetClient client;
IPAddress ip(192, 168, 0, 177);
void setup() {
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);
  }
  delay(1000);
  Serial.println("connecting...");
  
  #if !defined(SOFTWARE_SPI)
    pinMode(4, OUTPUT);
  #endif
  Serial.begin(9600);
  Serial.println("VC0706 Camera snapshot test");
  
  // Try to locate the camera
  if (cam.begin()) {
    Serial.println("Camera Found:");
  } else {
    Serial.println("No camera found?");
    return;
  }
  cam.setImageSize(VC0706_160x120);
  uint8_t imgsize = cam.getImageSize();
  Serial.print("Image size: ");
  if (imgsize == VC0706_160x120) Serial.println("160x120");

  Serial.println("Snap in 3 secs...");
  delay(3000);
  
  if (! cam.takePicture()) 
    Serial.println("Failed to snap!");
  else 
    Serial.println("Picture taken!");
  
  char filename[13];
  strcpy(filename, "snapshot.jpg");
  uint16_t jpglen = cam.frameLength();
  Serial.print("Sending... ");
  Serial.print(jpglen, DEC);
  Serial.print(" byte image.");

    // Prepare request
    String start_request = "";
    String end_request = "";
    start_request = start_request + "\n" + "--AaB03x" + "\n" + "Content-Disposition: form-data; name=\"key\"; filename=\"snapshot.jpg\"" + "\n" + "Content-Type: image/jpeg" + "\n" + "Content-Transfer-Encoding: binary" + "\n" + "\n";  
    end_request = end_request + "\n" + "--AaB03x--" + "\n";
      
    uint16_t extra_length;
    extra_length = start_request.length() + end_request.length();
    Serial.println("Extra length:");
    Serial.println(extra_length);
      
    uint16_t len = jpglen + extra_length;  

  // Print Full Request
   Serial.println("/***************** FULL REQUEST ****************/");
   Serial.println(F("POST / HTTP/1.1"));
   Serial.println(F("Host: http://hipodoor-photos.s3.eu-central-1.amazonaws.com"));
   Serial.println(F("Content-Type: multipart/form-data; boundary=AaB03x"));
   Serial.print(F("Content-Length: "));
   Serial.println(len);
   Serial.print(start_request);
   Serial.print("binary data");
   Serial.print(end_request);
   Serial.println("/**********************************************/");
   

  
  if (client.connect(server, 80)) {  
      Serial.println(F("Connected !"));
      
      client.println(F("POST / HTTP/1.1"));
      client.println(F("Host: http://hipodoor-photos.s3.eu-central-1.amazonaws.com"));
      client.println(F("Content-Type: multipart/form-data; boundary=AaB03x"));
      client.print(F("Content-Length: "));
      client.println(len);
      client.print(start_request);

            
      // Read all the data up to # bytes!
      byte wCount = 0; // For counting # of writes
      while (jpglen > 0) {
        
        uint8_t *buffer;
        uint8_t bytesToRead = min(32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
        
        buffer = cam.readPicture(bytesToRead);
        client.write(buffer, bytesToRead);

        for (int i = 0; i < sizeof(bytesToRead); i++) {
           Serial.print(buffer[i]);
        }
    
        if(++wCount >= 32) { // Every 2K, give a little feedback so it doesn't appear locked up
          Serial.print('.');
          wCount = 0;
        }
        
        jpglen -= bytesToRead; 
      }
      
      client.print(end_request);
      client.println();
      
    Serial.println("Transmission over");
  } 
  
  else {
      Serial.println(F("Connection failed"));    
    }
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

    if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    for(;;)
      ;
  }
}

