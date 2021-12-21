#include "Arduino.h"
#include "air.h"

#include<WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define powerPin  27
#define colorPin  13

WebServer server(80);
Adafruit_NeoPixel pixels(2, colorPin, NEO_GRB + NEO_KHZ800);
bool connectivity = true;
 String style =
                        "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
                        "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
                        "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
                        "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
                        "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
                        ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
        String loginIndex = 
                        "<form name=loginForm>"
                        "<h1>ESP32 Login</h1>"
                        "<input name=userid placeholder='User ID'> "
                        "<input name=pwd placeholder=Password type=Password> "
                        "<input type=submit onclick=check(this.form) class=btn value=Login></form>"
                        "<script>"
                        "function check(form) {"
                        "if(form.userid.value=='admin' && form.pwd.value=='admin')"
                        "{window.open('/serverIndex')}"
                        "else"
                        "{alert('Error Password or Username')}"
                        "}"
                        "</script>" + style;
 
        /* Server Index Page */
        String serverIndex = 
                        "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
                        "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
                        "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
                        "<label id='file-input' for='file'>   Choose file...</label>"
                        "<input type='submit' class=btn value='Update'>"
                        "<br><br>"
                        "<div id='prg'></div>"
                        "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
                        "<script>"
                        "function sub(obj){"
                        "var fileName = obj.value.split('\\\\');"
                        "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
                        "};"
                        "$('form').submit(function(e){"
                        "e.preventDefault();"
                        "var form = $('#upload_form')[0];"
                        "var data = new FormData(form);"
                        "$.ajax({"
                        "url: '/update',"
                        "type: 'POST',"
                        "data: data,"
                        "contentType: false,"
                        "processData:false,"
                        "xhr: function() {"
                        "var xhr = new window.XMLHttpRequest();"
                        "xhr.upload.addEventListener('progress', function(evt) {"
                        "if (evt.lengthComputable) {"
                        "var per = evt.loaded / evt.total;"
                        "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
                        "$('#bar').css('width',Math.round(per*100) + '%');"
                        "}"
                        "}, false);"
                        "return xhr;"
                        "},"
                        "success:function(d, s) {"
                        "console.log('success!') "
                        "},"
                        "error: function (a, b, c) {"
                        "}"
                        "});"
                        "});"
                        "</script>" + style;

air::air(int pin) {
    pinMode(pin, INPUT_PULLUP);
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    pixels.begin();
    _pin = pin;
}

void air::launch(const char* host, const char* ssid, const char* password, bool debug = false) {

    _host = host;
    _ssid = ssid; 
    _password = password;
    _debug = debug;                                         
   int state = digitalRead(_pin);

   

    if(state == LOW) {
        
       
        if (connectivity == true) {
       
            WiFi.begin(_ssid, _password);

            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                pixels.clear();
                pixels.setPixelColor(1,  pixels.Color(0, 255,255));
                pixels.show(); 
                Serial.print(".");
            }
            Serial.println("Connected");

            if (!MDNS.begin(_host)) {
                Serial.println("Error setting up MDNS responder!");
                while (1) {
                    delay(1000);
                }
            }
            Serial.println("mDNS responder started");
            pixels.clear();
            pixels.setPixelColor(1,  pixels.Color(255, 0, 255));
            pixels.show(); 
            if (_debug == true) {
                Serial.print("Connected to ");
                Serial.println(ssid);
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                delay(5000);
            }
            connectivity = false;
        }
        server.on("/", HTTP_GET, []() {
            server.sendHeader("connection", "close");
            server.send(200, "text/html", loginIndex);
        });

        server.on("/serverIndex", HTTP_GET, []() {
            server.sendHeader("Connection", "close");
            server.send(200, "text/html", serverIndex);
            });
            /*handling uploading firmware file */
        server.on("/update", HTTP_POST, []() {
            server.sendHeader("Connection", "close");
            server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            ESP.restart();
            }, []() {
            HTTPUpload& upload = server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("Update: %s\n", upload.filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
                    
                    Update.printError(Serial);
                
                    }
                } 
            else if (upload.status == UPLOAD_FILE_WRITE) {
                /* flashing firmware to ESP*/
                pixels.clear();
                pixels.setPixelColor(1,  pixels.Color(124, 252, 0));
                pixels.show(); 
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                        Update.printError(Serial);

                    }
                } 
            else if (upload.status == UPLOAD_FILE_END) {
                    if (Update.end(true)) { //true to set the size to the current progress
                    pixels.clear();
                    digitalWrite(powerPin, LOW);

                    Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                } 
            else {
                    Update.printError(Serial);
                    }
                }
            });
            server.begin();

    
    for(int i=0; i>=0; i++) {
           server.handleClient();
            }

    }

  else{
   if ( WiFi.status() == WL_CONNECTED) {
        delay(50);
        server.stop();
        WiFi.disconnect();
        connectivity = true;
        delay(50);
   }
  }
}




