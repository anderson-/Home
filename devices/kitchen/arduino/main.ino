#include <SmoothThermistor.h>
#include <SimpleDHT.h>

SimpleDHT11 dht11;
SmoothThermistor smoothThermistor(A1);

#define DHT11 2
#define LDR A0
#define LED A2
#define RELAY A3

long s1t;
long s2t;
int ldr_th = 160;
bool washing_machine_state = false;
bool relay_state = false;
int safe_temp_th = 200;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  Serial.begin(115200);
  s1t = millis();
  s2t = millis();
}

void loop() {
  int v = analogRead(LDR);
  if (v < ldr_th) {
    if (!washing_machine_state){
      Serial.println("W1");
    }
    washing_machine_state = true;
    digitalWrite(LED, 1);
  } else {
    if (washing_machine_state){
      Serial.println("W0");
    }
    washing_machine_state = false;
    digitalWrite(LED, 0);
  }

  if (smoothThermistor.temperature() > safe_temp_th){
    Serial.println("R0");
    relay_state = false;
    digitalWrite(RELAY, 0);
  }

  int err = SimpleDHTErrSuccess;
  if (millis() - s2t > 500){
    Serial.print("K"); Serial.println(smoothThermistor.temperature());
    s2t = millis();
  }
  if (millis() - s1t > 1500){
    byte temperature = 0;
    byte humidity = 0;
    if ((err = dht11.read(DHT11, &temperature, &humidity, NULL)) == SimpleDHTErrSuccess) {
      Serial.print("x"); Serial.print((int)temperature);
      Serial.print("h"); Serial.println((int)humidity);
    }
  }
  delay(100);
}

#define CMD_SET_TEMP_TH 'T'
#define CMD_GET_TEMP_TH 't'
#define CMD_SET_LDR_TH 'L'
#define CMD_GET_LDR_TH 'l'
#define CMD_GET_LDR 'k'
#define CMD_GET_WASHING_MACHINE 'w'
#define CMD_GET_RELAY 'r'
#define CMD_SET_RELAY 'R'

void serialEvent(){
  if (Serial.available() && Serial.find('cmd')){
    int cmd = Serial.read();
    int val = Serial.parseInt();

    switch (cmd) {
      case CMD_SET_TEMP_TH:
        safe_temp_th = val;
        break;
      case CMD_SET_LDR_TH:
        ldr_th = val;
        break;
      case CMD_GET_TEMP_TH:
        Serial.print('T'); Serial.println(safe_temp_th);
        break;
      case CMD_GET_LDR_TH:
        Serial.print('L'); Serial.println(ldr_th);
        break;
      case CMD_GET_LDR:
        Serial.print('k'); Serial.println(analogRead(LDR));
        break;
      case CMD_GET_WASHING_MACHINE:
        Serial.print('W'); Serial.println(washing_machine_state ? "1" : "0");
        break;
      case CMD_GET_RELAY:
        Serial.print('R'); Serial.println(relay_state ? "1" : "0");
        break;
      case CMD_SET_RELAY:
        relay_state = val == 200;
        digitalWrite(RELAY, relay_state);
        Serial.print('R'); Serial.println(relay_state ? "1" : "0");
        break;

    }
  }
}
