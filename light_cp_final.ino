/*
 *Functon:Connect to CorePro
 *Applicable environment(Windows7 32/64bit,1GHZ, 2M RAM ):
 1.Hardware: ESP32 Soc,4M Bytes Flash
 2.Software: Arduino 1.8.5,ESP32 SDK3.0 for Arduino
 https://www.arduino.cc/
 https://www.espressif.com/
*/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "mbedtls/md.h"
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
WiFiClient wclient;
PubSubClient client(wclient);
const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 150;

//const char* ssid = "LieBaoWiFi565";///your wifi ssid
//const char* password = "12345678";//your wifi password

const char* ssid = "mimi";///your wifi ssid
const char* password = "12345678aa";//your wifi password

/*
 * Three tuples
 */
/*String productKey = "6613939361801420272";
String deviceName = "light0";
String DeviceSecret = "7cba3f8e3e7b93dade09881e0d222e45fd642bb76b574a9fbd0591ec04b11373";
*/
/*
6613871524408782055
light0
21f9e3a08d9810d6f3557de2eb4f8d74d2a222117f6b6e663bf230a4e23517f7
*/
String productKey = "6613871524408782055";
String deviceName = "light0";
String DeviceSecret = "21f9e3a08d9810d6f3557de2eb4f8d74d2a222117f6b6e663bf230a4e23517f7";
const String timestamp = "1577065603";

//String url = "https://service-2uqamwtq-1256876865.ap-guangzhou.apigateway.myqcloud.com/release/corepro/auth/mqtt_auth?X-NameSpace-Code=corepro&X-MicroService-Name=corepro-device-auth";
String url = "http://service-de8elrpo-1255000335.apigw.fii-foxconn.com/release/corepro/auth/mqtt/?X-NameSpace-Code=corepro&X-MicroService-Name=corepro-device-auth";

//String url = "http://10.64.32.233/auth/device/";
String publishTopic = "/"+productKey+"/"+deviceName+"/property/post";
String subscribeTopic = "/"+productKey+"/"+deviceName+"/property/post/reply";


const int httpsPort = 80;
String signmethod = "HmacSHA256";
String clientId = productKey+"-"+deviceName;  //可能是_或-或不需要clientId
String sign_content ="clientId"+clientId+"deviceName"+deviceName+"productKey"+productKey+"timestamp"+timestamp;
String sign;
String data;

//#define DEFAULT_ID "193.112.227.84"   //MQTT server ip
const char* iotHost = "";
int iotPort;
const char* iotId = "";
const char* iotToken = "";

//确定三色灯引脚
int yellow = 35; //黃色--35
int green = 36;   //綠色--36
int red = 34;     //紅色--34

int yellowState = 0;   //黄色灯状态
int greenState = 0;    //绿色灯状态
int redState = 0;      //红色灯状态

int yellowMsg = 0;   //黄色灯信息發送控制
int greenMsg = 0;    //绿色灯信息發送控制
int redMsg = 0;      //红色灯信息發送控制

void setup() {
    Serial.begin(115200);
    Serial.print("connecting to "); 
    Serial.println(ssid);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(5000);
        Serial.print(".");
        WiFi.begin(ssid, password);
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  
    //设置三色灯ＧＰＩＯ状态
    pinMode(yellow, INPUT);
    pinMode(green, INPUT); 
    pinMode(red, INPUT); 
  
   
  
    sign =HMAC_SHA256(DeviceSecret,sign_content);  
    //data = "productKey="+productKey+"&deviceName="+deviceName+"&timestamp="+timestamp+"&signmethod="+signmethod+"&sign="+sign;
    //打包信息data
    //data ="productKey"+productKey+"&deviceName"+deviceName+"&timestamp"+timestamp+"&signmethod"+signmethod+"sign"+sign;
    data ="clientId="+ clientId+ "&productKey="+productKey+"&deviceName="+deviceName+"&timestamp="+timestamp+"&signmethod="+signmethod+"&sign="+sign;
    Serial.println(data);

    get_idToken(url,data,bufferSize);
    
    //client.setServer(iotHost,iotPort);
    Serial.printf("iotHost:");
    Serial.println(iotHost);
    Serial.printf("iotPort:");
    Serial.println(iotPort);
    client.setServer(iotHost,iotPort);
    client.connect(iotId,iotId,iotToken);
}
 
void loop() {
    
    static long i=18323627;//Mqtt message serial number requirements are different
    if(WiFi.status()== WL_CONNECTED){    
        if (!client.connected()) {
           
          reconnect();
        } 
        else{
          client.loop();
        }
    }
    else{
       Serial.println("Error in WiFi connection"); 
       while (WiFi.status() != WL_CONNECTED) {
          delay(5000);
          Serial.print(".");
          WiFi.begin(ssid, password);
        }          
    }
  
  String s0 = get_light_state();
    String s  = "{\"params\":";
       s += s0;
       s +=",\"msg_ver\":8323627,\"id\":"; //Combine according to the TSL(Thing Specification Language)
       s += i ;
       s += "}";
    if((WiFi.status() == WL_CONNECTED)&&(client.connected()==1)){

        client.publish(publishTopic.c_str(),s.c_str());
        i++;  
    }
    delay(500);
}


/*
 *Get mqtt Id and Token
*/
void get_idToken(String http_url,String headers,const size_t json_bufferSize){
    HTTPClient http;   
    http.begin(http_url);                                                            //Specify destination for HTTP request
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");             //Specify content-type header
    int httpResponseCode = http.POST(headers);                                       //Send the actual POST request
    if(httpResponseCode>0){
        DynamicJsonBuffer jsonBuffer(json_bufferSize);
        String response = http.getString();                                          //Get the response to the request
        Serial.println(response);
        if (response.indexOf("payload") != -1){
            JsonObject& root = jsonBuffer.parseObject(response);
            JsonObject& payload0 = root["payload"][0];
            iotHost = payload0["iotHost"];
            iotPort = payload0["iotPort"];
            iotId = payload0["iotId"];
            iotToken = payload0["iotToken"];// "wQzIeI1YKLcR2zlMT"
            Serial.printf("iotHost:");
            Serial.println(iotHost);
            Serial.printf("iotPort:");
            Serial.println(iotPort);
            Serial.printf("iotId:");
            Serial.println(iotId);
            Serial.printf("iotToken:");
            Serial.println(iotToken);
      //还有两个返回，host和port作为mqtt地址
        }
        else{
          iotId = "";
          iotToken = "";
        }
    }
    else{
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
        iotId = "";
        iotToken = "";
    }
    http.end();  //Free resources
}

/*
 *HMAC SHA256
*/
String  HMAC_SHA256(String ikey,String ipayload) {
  char count = ikey.length();
  char key[count]; 
  char bytes[count/2]; 
  const char *payload = ipayload.c_str();   
  strcpy(key, ikey.c_str());     
  strToByte(key,bytes);
  byte hmacResult[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  const size_t payloadLength = strlen(payload);
  const size_t keyLength = sizeof(bytes);//count/2
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) bytes, keyLength);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);
  Serial.print("Hash: ");
  String msha = "";
  for (int i = 0; i < sizeof(hmacResult); i++) {
    char str[3];
    sprintf(str, "%02x", (int)hmacResult[i]);
    msha += str;
  }
  Serial.println(msha);
  return msha;
}

/*
 *string to bytes
 *"A" or "a"
*/
int strToByte(char inputStr[],char outputByte[]) {
    int i,n = 0;
    int c=strlen(inputStr);
    for(i=0;i<c;i++){
      if(inputStr[i] >= 'A'&&inputStr[i] <= 'F'){
            inputStr[i] = inputStr[i]+0x20;
        }
    }
    for(i = 0; inputStr[i]; i += 2) {
        if(inputStr[i] >= 'a' && inputStr[i] <= 'f')
            outputByte[n] = inputStr[i] - 'a' + 10;
        else outputByte[n] = inputStr[i] - '0';
        if(inputStr[i + 1] >= 'a' && inputStr[i + 1] <= 'f')
            outputByte[n] = (outputByte[n] << 4) | (inputStr[i + 1] - 'a' + 10);
        else outputByte[n] = (outputByte[n] << 4) | (inputStr[i + 1] - '0');
        outputByte[n]=(0xff&outputByte[n]);
        ++n;
    }
}


//读取三色灯状态
String get_light_state(){
   //讀取GPIO狀態
   yellowState = digitalRead(yellow);
   greenState = digitalRead(green);
   redState = digitalRead(red);
   
   Serial.println(yellowState);
   Serial.println(greenState);
   Serial.println(redState);
   delay(100);
   //恢復消息控制發送
   if(yellowState == 1){        
       yellowMsg = 0;        
   }
   if(greenState == 1){        
       greenMsg = 0;        
   }
   if(redState == 1){        
       redMsg = 0;        
   }
 
       //循環發佈消息
   if (client.connected()){      
     // client.loop();  
      //三色灯变化消息     
      if(yellowState == 0 && yellowMsg == 0){
      Serial.println("yellow is lighted");
            //client.publish("test0","Yellow");
            yellowMsg++;         
      }
      if(greenState == 0 && greenMsg == 0){     
      Serial.println("green is lighted");
            //client.publish("test0","Green");
            greenMsg++;          
      }
      if(redState == 0 && redMsg == 0){    
      Serial.println("red is lighted");
            //client.publish("test0","Red");
            redMsg++;          
      }
   }        
   String msg = ""; 
          msg +=  "{\"redState\":\"";
          msg +=  (redState)?"0":"1";
          msg += "\",\"greenState\":\"";
          msg += (greenState)?"0":"1";
          msg += "\",\"yellowState\":\"";
          msg += (yellowState)?"0":"1";
          msg += "\"}";
   Serial.println(msg);
   return msg;          
                   
}


//重连mqtt
void reconnect() {
  while (!client.connected()){
    Serial.println("Connecting to mqtt...");
    Serial.printf("iotId:");
    Serial.println(iotId);
    Serial.printf("iotToken:");
    Serial.println(iotToken);
    //productKey.c_str()或iotId或clientId
    if(client.connect(iotId,iotId,iotToken)) { 
      Serial.println("Connected MQTT Server Ok");
      client.subscribe(subscribeTopic.c_str());           
     }
    else{
        Serial.println("Connecting to MQTT server is error!");    
    }
    
  }
}
  