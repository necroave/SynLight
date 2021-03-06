#include <NeoPixelBus.h>  //https://github.com/Makuna/NeoPixelBus
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
#include <WiFiUDP.h>

// Depending on the setup, has to be at least the number of LEDs
const uint16_t PixelCount = 1000; 
const int PixelPin = 2;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
//WIFI
const unsigned int localPort = 8787;
WiFiUDP UDP;
boolean udpConnected = false;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
char ReplyBuffer[] = "pong";

int red = 0;
int green = 0;
int blue = 0;
unsigned long t1 = 0;
bool timer = true;
unsigned int ledCounter = 0;
unsigned int totalLedCounter = 0;
bool active = true;

void fill(int _start, int _end, int r, int g, int b)
{
    for (int n = _start; n < PixelCount && n < _end; n++) 
    { 
        strip.SetPixelColor(n,RgbColor(r,g,b));
    }
    strip.Show();
    Serial.println("\n-------- Show --------\n");
}

void setup()
{
    delay(500);
    Serial.begin(115200);
    strip.Begin();
    strip.Show();
    delay(100);
    fill(0, PixelCount, 20, 0, 0);
    delay(100);
    //https://github.com/tzapu/WiFiManager#how-it-works
    WiFiManager wifiManager;
    delay(100);
    wifiManager.autoConnect("SynLight");
    fill(0, PixelCount, 15, 15, 0);
    delay(100);
    udpConnected = (UDP.begin(localPort) == 1);
    if (udpConnected)
    {
        fill(0, PixelCount, 0, 20, 0);
        delay(100);
    }    
}

void loop()
{
    if (udpConnected)
    {
        int packetSize = UDP.parsePacket();        
        if (packetSize)
        {
            UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
            t1 = millis();

            Serial.print("\nPacket of size : ");Serial.println(packetSize);    
            
            if(packetBuffer[0] == 65) //magic number #1, helps eliminate the junk that is broadcasted on the network
            {
                active = true;

                packetSize--;
                Serial.print("\nPacket2 of size : ");Serial.println(packetSize);
                Serial.println("\nValid packet (170) received");
                int packetBuffer2[packetSize];
                for(int n=0; n<packetSize;n++)
                {
                    packetBuffer2[n] = packetBuffer[n+1];
                }
                
                if(packetBuffer2[0]==0)      //PING
                {
                    Serial.println("Received : Ping header");
                    if(packetSize==5)
                    {
                        if(packetBuffer2[1]=='p' && packetBuffer2[2]=='i' && packetBuffer2[3]=='n' && packetBuffer2[4]=='g')
                        {
                            Serial.println("Received : Ping command");
                            Serial.print("Answering -> [");Serial.print(ReplyBuffer);Serial.println("]");
    
                            UDP.beginPacket(UDP.remoteIP(), localPort);
                            UDP.write(ReplyBuffer);
                            UDP.endPacket();
                        }
                        else
                        {
                            Serial.println("Received : Ping commmand with right size but not matching"); 
                        }
                    }
                    else
                    {
                        Serial.println("Received : Ping commmand with wrong size"); 
                    }
                }
                else if(packetBuffer2[0]==1) //STATIC
                {
                    Serial.println("Received : Static color header");
                    if(packetSize==4)
                    {
                        Serial.println("Received : Static color command"); 
                        fill(0, PixelCount, packetBuffer2[1], packetBuffer2[2], packetBuffer2[3]);                                                        
                    }
                    else
                    {
                        Serial.println("Received : Not static color command"); 
                    }
                }
                else if(packetBuffer2[0]==2 || packetBuffer2[0]==3)
                {
                    Serial.println("Received : Payload header");
                    if(packetSize>3)
                    {
                        Serial.println("Received : Payload command"); 
                        Serial.print("ledCounter start\t: ");Serial.println(ledCounter);
                        Serial.print("totalLedCounter start\t: ");Serial.println(totalLedCounter); 
                                          
                        while(ledCounter<=((packetSize-2)/3) && ledCounter<PixelCount)
                        {
                            //Serial.print("ledCounter+totalLedCounter\t: ");Serial.println(ledCounter+totalLedCounter); 
                            red   = packetBuffer2[ledCounter*3 + 1];
                            green = packetBuffer2[ledCounter*3 + 2];
                            blue  = packetBuffer2[ledCounter*3 + 3];                        
                            if((ledCounter)<PixelCount)      
                            {
                                //POWERED FROM A SINGLE USB3.0 CONNECTION, NO EXTERNAL PSU, THUS THE DIVISIONS
                                strip.SetPixelColor(ledCounter+totalLedCounter,RgbColor(red>>2,green>>2,(blue*3)>>4));   
                            }
                            else
                            {
                                break;
                            }
                            ledCounter++;                        
                        }                    
                        totalLedCounter += ledCounter;
                        Serial.print("ledCounter end\t\t: ");Serial.println(ledCounter);
                        Serial.print("totalLedCounter end\t: ");Serial.println(totalLedCounter);
                        ledCounter = 0;                       
                                      
                        if(packetBuffer2[0]==3)
                        {
                            strip.Show();
                            Serial.println("\n-------- Show --------\n");
                            totalLedCounter = 0;
                        }   
                    }
                    else
                    {
                        Serial.println("Received : Not Payload color command"); 
                    }
                }
                else
                {
                    Serial.println("WTF");
                    Serial.println(packetBuffer2[0]);
                    char tmp = packetBuffer2[0];
                    Serial.println(tmp);

                    Serial.println("\npacketBuffer :");
                    for(int n=0; n<=packetSize;n++)
                    {
                        Serial.print(packetBuffer[n]);
                        Serial.print(',');
                    }
                    Serial.println();
    
                    Serial.println("\npacketBuffer2 :");
                    for(int n=0; n<packetSize;n++)
                    {
                        Serial.print(packetBuffer2[n]);
                        Serial.print(',');
                    }
                    Serial.println();
                }
            }
            else
            {
                Serial.println("\nInvalid packet received");
                for(int n=0; n<packetSize;n++)
                {
                    Serial.print(packetBuffer[n]);
                    Serial.print(',');
                }
                Serial.println();
            }
        }
    }
    //STANDBY AFTER 7 SECONDS
    if ((millis() - t1>7000) && active)
    {
        active = false;
        t1 = millis();
        fill(0, PixelCount, 0, 0, 0);
    }
}