#include <SmoothThermistor.h>
#include <SerialCommands.h>
// #include <avr/wdt.h>
#include <SimpleDHT.h>

#define PIN_DHT    2
#define PIN_LDR   A0
#define PIN_NTC   A1
#define PIN_LED   A2
#define PIN_RELAY A3

#define STR_LDR   "ldr"
#define STR_NTC   "ntc"
#define STR_DHT   "dht"
#define STR_LED   "led"
#define STR_RELAY "relay"

#define LDR_IDX   0
#define NTC_IDX   1
#define DHT_IDX   2

unsigned int up_ints[] = {50, 50, 1500, 5000}; //LDR, NTC, DHT, REPORT
unsigned long up_timer[4];
int up_vals[] = {0, 0, 0, 0, 0, 0};

unsigned long relay_t;
unsigned long relay_timer = 600000;
int relay_th_max = 91*100;
int relay_th_min = 1*100;

SimpleDHT11 dht11;
SmoothThermistor smoothThermistor(PIN_NTC);

char serial_command_buffer_[32];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

void print_report(){
	Serial.print("time=");
	Serial.print(millis());
	Serial.print(";");
  Serial.print(STR_LDR);
	Serial.print("=");
	Serial.print(up_vals[0]);
	Serial.print(";");
  Serial.print(STR_NTC);
	Serial.print("=");
	Serial.print(up_vals[1]/100);
	Serial.print(".");
  if (up_vals[1] >= 0){
    Serial.print(up_vals[1]%100);
  } else {
    Serial.print("0");
  }
	Serial.print(";");
  Serial.print(STR_DHT);
	Serial.print("t=");
	Serial.print(up_vals[2]/100);
	Serial.print(".");
  if (up_vals[2] >= 0){
    Serial.print(up_vals[2]%100);
  } else {
    Serial.print("0");
  }
	Serial.print(";");
  Serial.print(STR_DHT);
	Serial.print("h=");
	Serial.print(up_vals[3]/100);
	Serial.print(".");
  if (up_vals[3] >= 0){
    Serial.print(up_vals[3]%100);
  } else {
    Serial.print("0");
  }
	Serial.print(";");
  Serial.print(STR_LED);
	Serial.print("=");
	Serial.print(up_vals[4]);
	Serial.print(";");
  Serial.print(STR_RELAY);
	Serial.print("=");
	Serial.println(up_vals[5]);
}

void cmd_unrecognized(SerialCommands* sender, const char* cmd) {
	sender->GetSerial()->print("Unrecognized command [");
	sender->GetSerial()->print(cmd);
	sender->GetSerial()->println("]");
	sender->GetSerial()->flush();
}

void cmd_analog_read(SerialCommands* sender) {
	char* port_str = sender->Next();
	if (port_str == NULL) {
		sender->GetSerial()->println("ERROR NO_PORT");
		return;
	}

	int port = atoi(port_str);
	int value = analogRead(port);

	sender->GetSerial()->print("analogRead(");
	sender->GetSerial()->print(port);
	sender->GetSerial()->print(")=");
	sender->GetSerial()->print(value);
	sender->GetSerial()->println();
}

void cmd_digital_read(SerialCommands* sender) {
	char* port_str = sender->Next();
	if (port_str == NULL) {
		sender->GetSerial()->println("ERROR NO_PORT");
		return;
	}

	int port = atoi(port_str);
	int value = digitalRead(port);

	sender->GetSerial()->print("digitalRead(");
	sender->GetSerial()->print(port);
	sender->GetSerial()->print(")=");
	sender->GetSerial()->print(value);
	sender->GetSerial()->println();
}

void cmd_digital_write(SerialCommands* sender) {
	char* port_str = sender->Next();
	if (port_str == NULL) {
		sender->GetSerial()->println("ERROR NO_PORT");
		return;
	}
  char* value_str = sender->Next();
	if (value_str == NULL) {
		sender->GetSerial()->println("ERROR NO_VALUE");
		return;
	}

  int port = atoi(port_str);
	int value = atoi(value_str);
  if (value != 0 && value != 1){
    value = !digitalRead(port);
  }
	digitalWrite(port, value);
  if (port == PIN_RELAY){
    if (value) {
      relay_t = millis();
    } else {
      relay_t = 0;
    }
  }

	sender->GetSerial()->print("digitalWrite(");
	sender->GetSerial()->print(port);
	sender->GetSerial()->print(",");
	sender->GetSerial()->print(value);
	sender->GetSerial()->println(")");
}

void cmd_pin_mode(SerialCommands* sender) {
	char* port_str = sender->Next();
	if (port_str == NULL) {
		sender->GetSerial()->println("ERROR NO_PORT");
		return;
	}
  char* mode_str = sender->Next();
	if (mode_str == NULL) {
		sender->GetSerial()->println("ERROR NO_MODE");
		return;
	}

  int port = atoi(port_str);

  if (strcmp(mode_str, "OUTPUT") == 0){
	   pinMode(port, OUTPUT);
	}	else if (strcmp(mode_str, "INPUT") == 0) {
	   pinMode(port, INPUT);
	}	else if (strcmp(mode_str, "INPUT_PULLUP") == 0) {
	   pinMode(port, INPUT_PULLUP);
	}	else {
    sender->GetSerial()->println("ERROR INV_MODE");
    return;
  }

	sender->GetSerial()->print("pinMode(");
	sender->GetSerial()->print(port);
	sender->GetSerial()->print(",");
	sender->GetSerial()->print(mode_str);
	sender->GetSerial()->println(")");
}

void cmd_report(SerialCommands* sender) {
  print_report();
}

void cmd_update(SerialCommands* sender) {
	char* var = sender->Next();
	if (var == NULL) {
		sender->GetSerial()->println("ERROR NO_VAR");
		return;
	}
  char* int_str = sender->Next();
	if (int_str == NULL) {
		sender->GetSerial()->println("ERROR NO_INTERVAL");
		return;
	}

  int interval = atoi(int_str);

  if (strcmp(var, STR_LDR) == 0){
    up_ints[0] = interval;
	}	else if (strcmp(var, STR_NTC) == 0) {
    up_ints[1] = interval;
	}	else if (strcmp(var, STR_DHT) == 0) {
    up_ints[2] = interval;
	}	else if (strcmp(var, "report") == 0) {
    up_ints[3] = interval;
	}	else {
    sender->GetSerial()->println("ERROR INV_VAR");
    return;
  }

	sender->GetSerial()->print(var);
	sender->GetSerial()->print(":");
	sender->GetSerial()->println(int_str);
}

void cmd_th(SerialCommands* sender) {
  char* opt_str = sender->Next();
	if (opt_str == NULL) {
		sender->GetSerial()->println("ERROR NO_OPT");
		return;
	}
	char* var = sender->Next();
	if (var == NULL) {
		sender->GetSerial()->println("ERROR NO_VAR");
		return;
	}
  int value;
  if (strcmp(opt_str, "set") == 0){
    char* value_str = sender->Next();
  	if (value_str == NULL) {
  		sender->GetSerial()->println("ERROR NO_VALUE");
  		return;
  	}
    value = atoi(value_str);
    if (strcmp(var, "temp_min") == 0){
      relay_th_min = value;
  	}	else if (strcmp(var, "temp_max") == 0) {
      relay_th_max = value;
  	}	else if (strcmp(var, "time") == 0) {
      relay_timer = value;
  	}	else {
      sender->GetSerial()->println("ERROR INV_VAR");
      return;
    }
  	sender->GetSerial()->print(var);
  	sender->GetSerial()->print("->");
  	sender->GetSerial()->println(value);
	}	else if (strcmp(opt_str, "get") == 0) {
    if (strcmp(var, "temp_min") == 0){
      value = relay_th_min;
  	}	else if (strcmp(var, "temp_max") == 0) {
      value = relay_th_max;
  	}	else if (strcmp(var, "time") == 0) {
      value = relay_timer;
  	}	else {
      sender->GetSerial()->println("ERROR INV_VAR");
      return;
    }
  	sender->GetSerial()->print(var);
  	sender->GetSerial()->print(":");
  	sender->GetSerial()->println(value);
	}	else {
    sender->GetSerial()->println("ERROR INV_OPT");
    return;
  }
}

void cmd_reset(SerialCommands* sender) {
  delay(5000);
	sender->GetSerial()->println("reset failed");
}

SerialCommand cmd_analog_read_("analogRead", cmd_analog_read);
SerialCommand cmd_digital_read_("digitalRead", cmd_digital_read);
SerialCommand cmd_digital_write_("digitalWrite", cmd_digital_write);
SerialCommand cmd_pin_mode_("pinMode", cmd_pin_mode);
SerialCommand cmd_reset_("reset", cmd_reset);
SerialCommand cmd_th_("th", cmd_th);
SerialCommand cmd_update_("update", cmd_update);
SerialCommand cmd_report_("report", cmd_report);

void setup(){
  // wdt_enable(WDTO_2S);
	Serial.begin(115200);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_LED, 0);
  digitalWrite(PIN_RELAY, 0);

	serial_commands_.SetDefaultHandler(cmd_unrecognized);
	serial_commands_.AddCommand(&cmd_analog_read_);
	serial_commands_.AddCommand(&cmd_digital_read_);
	serial_commands_.AddCommand(&cmd_digital_write_);
	serial_commands_.AddCommand(&cmd_pin_mode_);
	serial_commands_.AddCommand(&cmd_reset_);
  serial_commands_.AddCommand(&cmd_th_);
	serial_commands_.AddCommand(&cmd_update_);
	serial_commands_.AddCommand(&cmd_report_);
	Serial.println("Ready!");

  for (int i = 0; i < 4; i++){
    up_timer[i] = millis();
  }
}

void loop(){
	serial_commands_.ReadSerial();
  // wdt_reset();
  if (millis() - up_timer[0] > up_ints[0]){
    up_vals[0] = analogRead(PIN_LDR);
    up_vals[4] = digitalRead(PIN_LED);
    up_vals[5] = digitalRead(PIN_RELAY);
    up_timer[0] = millis();
  }
  if (millis() - up_timer[1] > up_ints[1]){
    up_vals[1] = (int)(smoothThermistor.temperature()*100);
    if (up_vals[1] <= relay_th_min || up_vals[1] >= relay_th_max){
      digitalWrite(PIN_RELAY, 0);
      relay_t = 0;
    }
    up_timer[1] = millis();
  }
  if (millis() - up_timer[2] > up_ints[2]){
    float temperature = 0;
    float humidity = 0;
    if (dht11.read2(PIN_DHT, &temperature, &humidity, NULL) == SimpleDHTErrSuccess) {
      up_vals[2] = (int)(temperature*100);
      up_vals[3] = (int)(humidity*100);
    }
    up_timer[2] = millis();
  }
  if (millis() - up_timer[3] > up_ints[3]){
    print_report();
    up_timer[3] = millis();
  }
  if (relay_t != 0 && millis() - relay_t > relay_timer) {
    digitalWrite(PIN_RELAY, 0);
    relay_t = 0;
  }
}
