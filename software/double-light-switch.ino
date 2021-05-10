#include <modified-WiFiManager.h>

#define software_version "0.2.7"
#define compatible_hardware "ESP-12E"
#define compatible_device "Double light switch"
#define github_project_link "https://github.com/IT-freak-Jake/IT-Freak-Home-smart-light-switch-2-relays"
#define confSize 1024
#define mqttBufferSize 1124

String host_name = "DoubleLightSwitch-" + String(ESP.getChipId());

#define first_light_on_state "ON"
#define first_light_off_state "OFF"

#define second_light_on_state "ON"
#define second_light_off_state "OFF"

#define first_light_on_command "ON"
#define first_light_off_command "OFF"

#define second_light_on_command "ON2"
#define second_light_off_command "OFF2"

#define RELAY 14 
#define RELAY2 4

#define SWITCH 13 
#define SWITCH2 12 

String command_topic, state_topic, state2_topic, attributes_topic, sw1_config_topic, sw2_config_topic, sw1_icon, sw2_icon;
boolean isLightOn=false, prevSwitchState, isLight2On=false, prevSwitch2State;

char sw1_name[64];
char sw2_name[64];

boolean isSw1DynamicIconsEnabled=false;
char sw1_icon_off[32]="mdi:lightbulb";
char sw1_icon_on[32]="mdi:lightbulb-on";

boolean isSw2DynamicIconsEnabled=false;
char sw2_icon_off[32]="mdi:lightbulb";
char sw2_icon_on[32]="mdi:lightbulb-on";

String sw1_name_placeholder="placeholder=\""+host_name+"-sw1\"",
sw2_name_placeholder="placeholder=\""+host_name+"-sw2\"";
WiFiManagerParameter custom_sw1_name("sw1_name", "Switch 1 name", sw1_name, 63, sw1_name_placeholder.c_str());
WiFiManagerParameter custom_sw2_name("sw2_name", "Switch 2 name", sw2_name, 63, sw2_name_placeholder.c_str());

WiFiManagerParameter custom_sw1_enable_dynamic_icons("sw1_enable_dynamic_icons", "Enable dynamic icons for switch 1", "f", 2, "type=\"checkbox\" style=\"width: 20px\" onchange=\"changeDynamicIcons()\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_sw1_icon_off("sw1_icon_off", "Icon for switch 1", sw1_icon_off, 31, "placeholder=\"mdi:lightbulb\"");
WiFiManagerParameter custom_sw1_icon_on("sw1_icon_on", "Icon on ON for switch 1", sw1_icon_on, 31, "placeholder=\"mdi:lightbulb-on\"");
WiFiManagerParameter custom_sw1_icon_div("<div id=\"sw1_icon\" style=\"padding: 0px\">");

WiFiManagerParameter custom_sw2_enable_dynamic_icons("sw2_enable_dynamic_icons", "Enable dynamic icons for switch 2", "f", 2, "type=\"checkbox\" style=\"width: 20px\" onchange=\"changeDynamicIcons()\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_sw2_icon_off("sw2_icon_off", "Icon for switch 2", sw2_icon_off, 31, "placeholder=\"mdi:lightbulb\"");
WiFiManagerParameter custom_sw2_icon_on("sw2_icon_on", "Icon on ON for switch 2", sw2_icon_on, 31, "placeholder=\"mdi:lightbulb-on\"");
WiFiManagerParameter custom_sw2_icon_div("<div id=\"sw2_icon\" style=\"padding: 0px\">");

WiFiManagerParameter custom_additional_js(R"(<script>
document.getElementById("sw1_enable_dynamic_icons").checked = document.getElementById("sw1_enable_dynamic_icons").value=="t";
document.getElementById("sw2_enable_dynamic_icons").checked = document.getElementById("sw2_enable_dynamic_icons").value=="t";
var labels = document.getElementsByTagName('LABEL');
for (var i = 0; i < labels.length; i++) {
  if (labels[i].htmlFor != '') {
     var elem = document.getElementById(labels[i].htmlFor);
     if (elem)
        elem.label = labels[i];         
  }
}
changeDynamicIcons();
function changeDynamicIcons()
{
  const isCheckedSw1=document.getElementById("sw1_enable_dynamic_icons").checked;
  const isCheckedSw2=document.getElementById("sw2_enable_dynamic_icons").checked;
  if(isCheckedSw1)
  {
    document.getElementById("sw1_icon_off").label.innerHTML = "Icon on OFF for switch 1";
    document.getElementById("sw1_icon").style.display = "initial";
  }
  else if(!isCheckedSw1)
  {
    document.getElementById("sw1_icon_off").label.innerHTML = "Icon for switch 1";
    document.getElementById("sw1_icon").style.display = "none";
  }
  if(isCheckedSw2)
  {
    document.getElementById("sw2_icon_off").label.innerHTML = "Icon on OFF for switch 2";
    document.getElementById("sw2_icon").style.display = "initial";
  }
  else if(!isCheckedSw2)
  {
    document.getElementById("sw2_icon_off").label.innerHTML = "Icon for switch 2";
    document.getElementById("sw2_icon").style.display = "none";
  }
}
</script>)");

#include "IT_Freak_Home_essentials.h"

void WiFiManager::handleControl() {
  DEBUG_WM(DEBUG_VERBOSE,F("<- HTTP Control"));
  if (captivePortal()) return;
  handleRequest();

  if (server->hasArg("action"))
  {
    String action = server->arg("action");
    if (action=="on")
    {
      isLightOn=true;
      if(mqttClient.connected())
      {
        msg = first_light_on_state;
        mqttClient.publish(state_topic, msg);
        if(isSw1DynamicIconsEnabled && isDiscoveryOn)resendConfig();
      }
      digitalWrite(RELAY, HIGH);
    }
    else if (action=="off")
    {
      isLightOn=false;
      if(mqttClient.connected())
      {
        msg = first_light_off_state;
        mqttClient.publish(state_topic, msg);
        if(isSw1DynamicIconsEnabled && isDiscoveryOn)resendConfig();
      }
      digitalWrite(RELAY, LOW);
    }
    else if (action=="on2")
    {
      isLight2On=true;
      if(mqttClient.connected())
      {
        msg = second_light_on_state;
        mqttClient.publish(state2_topic, msg);
        if(isSw2DynamicIconsEnabled && isDiscoveryOn)resendConfig();
      }
      digitalWrite(RELAY2, HIGH);
    }
    else if (action=="off2")
    {
      isLight2On=false;
      if(mqttClient.connected())
      {
        msg = second_light_off_state;
        mqttClient.publish(state2_topic, msg);
        if(isSw2DynamicIconsEnabled && isDiscoveryOn)resendConfig();
      }
      digitalWrite(RELAY2, LOW);
    }
  }
  
  String page = getHTTPHead("Control"); 
  page += "<h1>";
  page += (device_user_name[0]!='\0'?device_user_name:host_name.c_str());
  page += "</h1>";
  page += "First switch is ";
  page += (isLightOn?"ON":"OFF");
  page += "<br/>";
  page += "Second switch is ";
  page += (isLight2On?"ON":"OFF");
  page += "<br/><br/>";
  page += "<button ";
  page += isLightOn?"style=\"height: 70px;line-height: 1rem;width: 47%;\" class=\"D\" onclick=\"location.href='?action=off';\"":"style=\"background-color: #44bb44;height: 70px;line-height: 1rem;width: 47%;\" onclick=\"location.href='?action=on';\"";
  page += "><h3>Turn ";
  page += (isLightOn?"OFF 1":"ON 1");
  page += "</h3></button>";
  page += "<button ";
  page += isLight2On?"style=\"height: 70px;line-height: 1rem;width: 47%; margin-left: 6%;\" class=\"D\" onclick=\"location.href='?action=off2';\"":"style=\"background-color: #44bb44;height: 70px;line-height: 1rem;width: 47%; margin-left: 6%;\" onclick=\"location.href='?action=on2';\"";
  page += "><h3>Turn ";
  page += (isLight2On?"OFF 2":"ON 2");
  page += "</h3></button><br/><br/>";
  page += "<hr><br/>";
  page += "<button style=\"width:47%;\" onclick=\"location.href='/';\" >Back</button>";
  page += "<button style=\"width:47%; margin-left: 6%\" onclick=\"location.href='?';\" >Refresh</button>";
  reportStatus(page);
  reportMqttStatus(page);
  page += FPSTR(HTTP_END);

  server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
  server->send(200, FPSTR(HTTP_HEAD_CT), page);
  if(_preloadwifiscan) WiFi_scanNetworks(_scancachetime,true); 
}

void addAdditionalParameters()
{
  wm.addParameter(&custom_sw1_name);
  wm.addParameter(&custom_sw2_name);
  
  wm.addParameter(&custom_br);
  wm.addParameter(&custom_sw1_enable_dynamic_icons);
  wm.addParameter(&custom_br);
  wm.addParameter(&custom_br);
  wm.addParameter(&custom_sw1_icon_off);
  wm.addParameter(&custom_sw1_icon_div);
  wm.addParameter(&custom_sw1_icon_on);
  wm.addParameter(&custom_end_div);

  wm.addParameter(&custom_br);
  wm.addParameter(&custom_sw2_enable_dynamic_icons);
  wm.addParameter(&custom_br);
  wm.addParameter(&custom_br);
  wm.addParameter(&custom_sw2_icon_off);
  wm.addParameter(&custom_sw2_icon_div);
  wm.addParameter(&custom_sw2_icon_on);
  wm.addParameter(&custom_end_div);
  
  wm.addParameter(&custom_additional_js);
  wm.addParameter(&custom_hr);
}

void setAdditionalParameters()
{
  custom_sw1_name.setValue(sw1_name, 64);
  custom_sw1_enable_dynamic_icons.setValue(isSw1DynamicIconsEnabled?"t":"f",2);
  custom_sw1_icon_off.setValue(sw1_icon_off, 32);
  custom_sw1_icon_on.setValue(sw1_icon_on, 32);
  String stringBuffer;
  if(sw1_name[0]=='\0'){
    stringBuffer=host_name+"-sw1";
    stringBuffer.toCharArray(sw1_name, 64);
  }
  if(sw1_icon_off[0]=='\0'){
    stringBuffer="mdi:lightbulb";
    stringBuffer.toCharArray(sw1_icon_off, 32);
  }
  if(sw1_icon_on[0]=='\0'){
    stringBuffer="mdi:lightbulb-on";
    stringBuffer.toCharArray(sw1_icon_on, 32);
  }
  sw1_icon=sw1_icon_off;

  custom_sw2_name.setValue(sw2_name, 64);
  custom_sw2_enable_dynamic_icons.setValue(isSw2DynamicIconsEnabled?"t":"f",2);
  custom_sw2_icon_off.setValue(sw2_icon_off, 32);
  custom_sw2_icon_on.setValue(sw2_icon_on, 32);
  if(sw2_name[0]=='\0'){
    stringBuffer=host_name+"-sw2";
    stringBuffer.toCharArray(sw2_name, 64);
  }
  if(sw2_icon_off[0]=='\0'){
    stringBuffer="mdi:lightbulb";
    stringBuffer.toCharArray(sw2_icon_off, 32);
  }
  if(sw2_icon_on[0]=='\0'){
    stringBuffer="mdi:lightbulb-on";
    stringBuffer.toCharArray(sw2_icon_on, 32);
  }
  sw2_icon=sw2_icon_off;
}

void reloadAdditionalParameters()
{
  String stringBuffer;
  if(conf.containsKey("sw1_name"))
  {
    stringBuffer = conf["sw1_name"].as<String>();
    stringBuffer.toCharArray(sw1_name, 64);
  }
  if(conf.containsKey("isSw1DynamicIconsEnabled"))isSw1DynamicIconsEnabled = conf["isSw1DynamicIconsEnabled"];
  if(conf.containsKey("sw1_icon_off"))
  {
    stringBuffer = conf["sw1_icon_off"].as<String>();
    stringBuffer.toCharArray(sw1_icon_off, 32);
  }
  if(conf.containsKey("sw1_icon_on"))
  {
    stringBuffer = conf["sw1_icon_on"].as<String>();
    stringBuffer.toCharArray(sw1_icon_on, 32);
  }

  if(conf.containsKey("sw2_name"))
  {
    stringBuffer = conf["sw2_name"].as<String>();
    stringBuffer.toCharArray(sw2_name, 64);
  }
  if(conf.containsKey("isSw2DynamicIconsEnabled"))isSw2DynamicIconsEnabled = conf["isSw2DynamicIconsEnabled"];
  if(conf.containsKey("sw2_icon_off"))
  {
    stringBuffer = conf["sw2_icon_off"].as<String>();
    stringBuffer.toCharArray(sw2_icon_off, 32);
  }
  if(conf.containsKey("sw2_icon_on"))
  {
    stringBuffer = conf["sw2_icon_on"].as<String>();
    stringBuffer.toCharArray(sw2_icon_on, 32);
  }
}

void saveAdditionalParameters()
{
  conf["sw1_name"] = custom_sw1_name.getValue();
  Serial.println(conf["sw1_name"].as<String>());
  if((strncmp(custom_sw1_enable_dynamic_icons.getValue(), "f", 1) == 0)==true || (strncmp(custom_sw1_enable_dynamic_icons.getValue(), "t", 1) == 0)==true){
    conf["isSw1DynamicIconsEnabled"]=true;}
  else{
    conf["isSw1DynamicIconsEnabled"]=false;}
  conf["sw1_icon_off"] = custom_sw1_icon_off.getValue();
  Serial.println(conf["sw1_icon_off"].as<String>());
  conf["sw1_icon_on"] = custom_sw1_icon_on.getValue();
  Serial.println(conf["sw1_icon_on"].as<String>());

  conf["sw2_name"] = custom_sw2_name.getValue();
  Serial.println(conf["sw2_name"].as<String>());
  if((strncmp(custom_sw2_enable_dynamic_icons.getValue(), "f", 1) == 0)==true || (strncmp(custom_sw2_enable_dynamic_icons.getValue(), "t", 1) == 0)==true){
    conf["isSw2DynamicIconsEnabled"]=true;}
  else{
    conf["isSw2DynamicIconsEnabled"]=false;}
  conf["sw2_icon_off"] = custom_sw2_icon_off.getValue();
  Serial.println(conf["sw2_icon_off"].as<String>());
  conf["sw2_icon_on"] = custom_sw2_icon_on.getValue();
  Serial.println(conf["sw2_icon_on"].as<String>());
}

void pinsSetup()
{
  Serial.println("Setting up pins");
  pinMode(RELAY, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(SWITCH2, INPUT_PULLUP);
  digitalWrite(RELAY, LOW);
  digitalWrite(RELAY2, LOW);
  prevSwitchState=digitalRead(SWITCH);
  prevSwitch2State=digitalRead(SWITCH2);
  Serial.println("Done!");
}

void mqttTopicsSetup()
{
  Serial.println("Setting up MQTT topics");
  command_topic=String(mqtt_prefix)+"/"+nodeID;
  state_topic=String(mqtt_prefix)+"/"+nodeID+"/state";
  state2_topic=String(mqtt_prefix)+"/"+nodeID+"/state2";
  attributes_topic=String(mqtt_prefix)+"/"+nodeID+"/attributes";
  sw1_config_topic=String(discoveryPrefix)+"/switch/"+host_name+"/switch1/config";
  sw2_config_topic=String(discoveryPrefix)+"/switch/"+host_name+"/switch2/config";
  Serial.println("Done!");
}

void mqttStatusResend()
{
  Serial.println("Resending all MQTT messages");
  if(isDiscoveryOn)
  {
    resendConfig();
    mqttClient.loop();
  }
  else
  {
    Serial.println("Auto discovery disabled");
  }
  msg = isLightOn ? first_light_on_state:first_light_off_state;
  mqttClient.publish(state_topic, msg);
  mqttClient.loop();
  msg = isLight2On ? second_light_on_state:second_light_off_state;
  mqttClient.publish(state2_topic, msg);
  mqttClient.loop();
  resendAttributes();
  mqttClient.loop();
}

void switches_handle()
{
  if(digitalRead(SWITCH) != prevSwitchState)
  {
    isLightOn = !isLightOn;
    digitalWrite(RELAY, isLightOn);
    msg = isLightOn ? first_light_on_state:first_light_off_state;
    Serial.print("First light is ");  
    Serial.println(msg);  
    if(mqttClient.connected())mqttClient.publish(state_topic, msg);
    if(isSw1DynamicIconsEnabled && isDiscoveryOn && mqttClient.connected())resendConfig();
    delay(20);
    prevSwitchState=digitalRead(SWITCH);
  }  
  if(digitalRead(SWITCH2) != prevSwitch2State)
  {
    isLight2On = !isLight2On;
    digitalWrite(RELAY2, isLight2On);
    msg = isLight2On ? second_light_on_state:second_light_off_state;
    Serial.print("Second light is ");  
    Serial.println(msg);  
    if(mqttClient.connected())mqttClient.publish(state2_topic, msg);
    if(isSw2DynamicIconsEnabled && isDiscoveryOn && mqttClient.connected())resendConfig();
    delay(20);
    prevSwitch2State=digitalRead(SWITCH2);
  }  
}

void resendConfig()
{
  Serial.println("Sending auto discovery config");
  if(isSw1DynamicIconsEnabled)sw1_icon=isLightOn?sw1_icon_on:sw1_icon_off;
  if(isSw2DynamicIconsEnabled)sw2_icon=isLightOn?sw2_icon_on:sw2_icon_off;
  StaticJsonDocument<570> doc;
  StaticJsonDocument<128> doc2;
  String obj;
  obj="";
  doc2["identifiers"] = host_name;
  doc2["name"] = (device_user_name[0]!='\0'?device_user_name:host_name.c_str());
  doc2["manufacturer"] = mark;
  doc2["model"] = compatible_device;
  doc2["sw_version"] = software_version;

  doc["device"] = doc2;
  doc["name"] = sw1_name;
  doc["unique_id"] = host_name+"-sw1";
  doc["command_topic"] = command_topic;
  doc["state_topic"] = state_topic;
  doc["json_attributes_topic"] = attributes_topic;
  doc["retain"] = false;
  doc["payload_on"] = first_light_on_command;
  doc["payload_off"] = first_light_off_command;
  doc["state_on"] = first_light_on_state;
  doc["state_off"] = first_light_off_state;
  doc["icon"] = sw1_icon;
  serializeJson(doc, obj);
  Serial.println(sw1_config_topic);
  Serial.print("Sending auto discovery conf for sw1: "); Serial.println(obj);
  mqttClient.publish(sw1_config_topic, obj);

  mqttClient.loop();
  delay(5);
  doc.clear();
  obj="";
  doc["device"] = doc2;
  doc["name"] = sw2_name;
  doc["unique_id"] = host_name+"-sw2";
  doc["command_topic"] = command_topic;
  doc["state_topic"] = state2_topic;
  doc["json_attributes_topic"] = attributes_topic;
  doc["retain"] = false;
  doc["payload_on"] = second_light_on_command;
  doc["payload_off"] = second_light_off_command;
  doc["state_on"] = second_light_on_state;
  doc["state_off"] = second_light_off_state;
  doc["icon"] = sw2_icon;
  serializeJson(doc, obj);
  Serial.println(sw2_config_topic);
  Serial.print("Sending auto discovery conf for sw2: "); Serial.println(obj);
  mqttClient.publish(sw2_config_topic, obj);
}

void setup() 
{
  setupProcess();
}

void loop() 
{
  wifi_handle();
  mqtt_handle();
  ota_handle();
  switches_handle();
}

void mqtt_message_handle(String topic, String payload)
{
  if(payload==first_light_on_command)
  {
    isLightOn=true;
    msg = first_light_on_state;
    mqttClient.publish(state_topic, msg);
    digitalWrite(RELAY, HIGH);
    if(isSw1DynamicIconsEnabled && isDiscoveryOn)resendConfig();
  }
  else if(payload==first_light_off_command)
  {
    isLightOn=false;
    msg = first_light_off_state;
    mqttClient.publish(state_topic, msg);
    digitalWrite(RELAY, LOW);
    if(isSw1DynamicIconsEnabled && isDiscoveryOn)resendConfig();
  }
  else if(payload==second_light_on_command)
  {
    isLight2On=true;
    msg = second_light_on_state;
    mqttClient.publish(state2_topic, msg);
    digitalWrite(RELAY2, HIGH);
    if(isSw2DynamicIconsEnabled && isDiscoveryOn)resendConfig();
  }
  else if(payload==second_light_off_command)
  {
    isLight2On=false;
    msg = second_light_off_state;
    mqttClient.publish(state2_topic, msg);
    digitalWrite(RELAY2, LOW);
    if(isSw2DynamicIconsEnabled && isDiscoveryOn)resendConfig();
  }
}
