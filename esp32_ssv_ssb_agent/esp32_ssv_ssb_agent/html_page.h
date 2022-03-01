#ifndef HTML_PAGE_H
#define HTML_PAGE_H

const char html_main_page[] PROGMEM = R"rawliteral(
       
<!DOCTYPE html>
   <html>

      <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
      </head>
      
      <style>
        html {
            font-family: Arial, Helvetica, sans-serif;
            text-align: center;
        }      
        .button {
            padding: 1em 2.3em;
            font-size: 1em;
            text-align: center;
            outline: none;
            color: #fff;
            background-color: #3FA3D7;
            border: none;
            border-radius: 5px;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-tap-highlight-color: rgba(0,0,0,0);
           }
           .button:active {
             background-color: #143D77;
             box-shadow: 2 2px #CDCDCD;
             transform: translateY(2px);
           }
      
           h1{
            height: 2.3em;
            font-size:2em;
            background-color: #3FA3D7; 
            color: #fff;
           }
            /* Chrome, Safari, Edge, Opera */
          input::-webkit-outer-spin-button,
          input::-webkit-inner-spin-button {
              -webkit-appearance: none;
              margin: 0;
          }
          /* Firefox */
          input[type=number] {
            -moz-appearance: textfield;
          }    
      </style>

      <body>     
        <h1> <img src="ssv-logo_blue" width="150" style="float: left;"/>SSB-Agent</h1>    
        <blockquote>
        <div style = "font-size: 1em;">
        <p>Set WiFi configuration:</p>
        <p> WiFi SSID (Max 32 Chars)  <input type="text" id="ssid" value="" /> </p>
        <p> WiFi Pass (Max 32 Chars) <input type="text" id="password" value="" /> </p>

        <input type="radio" id="auto" name="radio_button" value="auto" onclick="ShowHideDiv()" >
        <label for="auto">MQTT auto configuration (mDNS)</label>
        <br>
        <input type="radio" id="manual" name="radio_button" value="manual" onclick="ShowHideDiv()" >
        <label for="manual">MQTT manual configuration</label>
        <br>
        <div id="manual_config" style="display: none">
            <p> MQTT Broker IP:<input type="text" id="ip_string"  value="" /> </p>   
        </div>   
        <br>
        <p><button id="button" class="button">Save configuration!</button> </p>
        </blockquote>
                                   
        <script>
             var gateway = `ws://${window.location.hostname}/ws`;
             var websocket;
             
             window.addEventListener('load', onLoad);

             function onLoad(event) {
                initWebSocket();
                initButton();
              }
  
             function initButton() {
                document.getElementById('button').addEventListener('click', sendValues);            
             }
                         
             function initWebSocket() {
                console.log('Trying to open a WebSocket connection...');
                websocket = new WebSocket(gateway);
                websocket.onopen    = onOpen;
                websocket.onclose   = onClose;
                websocket.onmessage = onMessage; // <-- add this line
             }
             
             function onOpen(event) {
                console.log('Connection opened');
             }
            
             function onClose(event) {
                console.log('Connection closed');
                setTimeout(initWebSocket, 2000);
             }
              
             function onMessage(event) {
                 var data_arr = event.data.split("/t");
                 if(event.data.search("success!") != -1 ){
                    window.alert("Setup stored! SSB Agent leaving AP-Mode...");
                }else{
                 document.getElementById("ssid").value = data_arr[0];
                 document.getElementById("password").value = data_arr[1];
                 document.getElementById("ip_string").value = data_arr[2];  
                }
             }
                          
              function sendValues(){
                var str; 
                str = document.getElementById("ssid").value + ",";
                str +=document.getElementById("password").value + ",";

                if(document.getElementById("auto").checked){
                   str += document.getElementById("auto").value;
                }else{
                    str += document.getElementById("manual").value + ",";
                    str += document.getElementById("ip_string").value;
                }
                
                websocket.send( str );
              }

              function ShowHideDiv() {
                document.getElementById("manual_config").style.display = document.getElementById("manual").checked ? "block" : "none";
              }
              
        </script>
      </body>
    </html>
           
)rawliteral";
#endif //HTML_PAGE_H
