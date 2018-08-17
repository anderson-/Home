#include <SmoothThermistor.h>
#include <SoftwareSerial.h>
#include <SerialCommands.h>
#include <IRLremote.h>
#include <Servo.h>

#define PIN_PWM_R     10
#define PIN_PWM_G     11
#define PIN_PWM_B      9
#define PIN_COOLER    12
#define PIN_SERVO      8
#define PIN_CLOCK_RX   5
#define PIN_CLOCK_TX   6
#define PIN_NTC       A0
#define PIN_ANLG_VCC  A1
#define PIN_LDR       A2
#define PIN_IR         2

Servo myservo;
CNec IRLremote;
SmoothThermistor smoothThermistor(PIN_NTC);
SoftwareSerial ss(PIN_CLOCK_TX, PIN_CLOCK_RX);

char serial_command_buffer_[32];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

char timestr[18];
unsigned long up_ints[] = {50, 50, 120000, 5000}; //LDR, NTC, CLOCK, REPORT
unsigned long up_timer[4];
int up_vals[] = {0, 0, 0, 0, 0, 0, 0}; //LDR,NTC,Light,R,G,B,Cooler
Nec_command_t last = 0;


void print_report(){
	Serial.print("time=");
	Serial.print(millis());
	Serial.print(";");

  int h, m, s, d, M, y;
  sscanf(timestr, "%d:%d:%d %d/%d/%d", &h, &m, &s, &d, &M, &y);
	Serial.print("h=");
  Serial.print(h);
	Serial.print(";");
	Serial.print("m=");
  Serial.print(m);
	Serial.print(";");
	Serial.print("s=");
  Serial.print(s);
	Serial.print(";");
	Serial.print("d=");
  Serial.print(d);
	Serial.print(";");
	Serial.print("M=");
  Serial.print(M);
	Serial.print(";");
	Serial.print("y=");
  Serial.print(y);
	Serial.print(";");

	Serial.print("ldr=");
	Serial.print(up_vals[0]);
	Serial.print(";");

	Serial.print("ntc=");
	Serial.print(up_vals[1]/100);
	Serial.print(".");
  if (up_vals[1] >= 0){
    Serial.print(up_vals[1]%100);
  } else {
    Serial.print("0");
  }
	Serial.print(";");

	Serial.print("L=");
	Serial.print(up_vals[2]);
	Serial.print(";");

	Serial.print("R=");
	Serial.print(up_vals[3]);
	Serial.print(";");
	Serial.print("G=");
	Serial.print(up_vals[4]);
	Serial.print(";");
	Serial.print("B=");
	Serial.print(up_vals[5]);
	Serial.print(";");

	Serial.print("C=");
	Serial.print(up_vals[6]);
	Serial.print(";");

	Serial.println();
}

void cmd_unrecognized(SerialCommands* sender, const char* cmd) {
	sender->GetSerial()->print("Unrecognized command [");
	sender->GetSerial()->print(cmd);
	sender->GetSerial()->println("]");
	sender->GetSerial()->flush();
}

void cmd_bb(SerialCommands* sender) {
  char* header  = sender->Next();
	if (header == NULL) {
		sender->GetSerial()->println("ERROR NO_HEADER");
		return;
	}
	char* var  = sender->Next();
  char* val  = sender->Next();

  switch (header[0]) {
    case '?':
      ss.println("?          ");
      sender->GetSerial()->print("bb(");
    	sender->GetSerial()->print(header);
    	sender->GetSerial()->println(")");
      break;
    case 'T':
      if (var == NULL) {
        sender->GetSerial()->println("ERROR NO_TIME");
        return;
      }
      ss.print(header[0]);
      ss.print(var);

      sender->GetSerial()->print("bb(");
    	sender->GetSerial()->print(header);
    	sender->GetSerial()->print(",");
    	sender->GetSerial()->print(var);
    	sender->GetSerial()->println(")");
      break;
    case 'B':
      if (var == NULL) {
        sender->GetSerial()->println("ERROR NO_VAR");
        return;
      }
      if (val == NULL) {
        sender->GetSerial()->println("ERROR NO_VALUE");
        return;
      }
      switch (var[0]) {
        case 'M':
        case 'h':
        case 'm':
        case 's':
        case 'S':
          ss.print(header[0]);
          ss.print(var[0]);
          ss.print(val);
          ss.println("       ");

          sender->GetSerial()->print("bb(");
        	sender->GetSerial()->print(header);
        	sender->GetSerial()->print(",");
        	sender->GetSerial()->print(var);
        	sender->GetSerial()->print(",");
        	sender->GetSerial()->print(val);
        	sender->GetSerial()->println(")");
          break;
        default:
          sender->GetSerial()->println("ERROR INVALID_SVAR");
          return;
      }
      break;
    default:
      sender->GetSerial()->println("ERROR INVALID_VAR");
      return;
  }
}

void cmd_ctrl(SerialCommands* sender) {
	char* var  = sender->Next();
	if (var == NULL) {
		sender->GetSerial()->println("ERROR NO_VAR");
		return;
	}
	char* val  = sender->Next();
	if (var == NULL) {
		sender->GetSerial()->println("ERROR NO_VALUE");
		return;
	}

  int v;

  switch (var[0]) {
    case 'L':
      v = constrain(atoi(val), 0, 1);
      analogWrite(PIN_PWM_R, 0);
      analogWrite(PIN_PWM_G, 0);
      analogWrite(PIN_PWM_B, 0);
      myservo.attach(PIN_SERVO);
      if (v){
        myservo.write(15);
        delay(200);
        myservo.write(20);
      } else {
        myservo.write(55);
        delay(200);
        myservo.write(50);
      }
      up_vals[2] = v;
      delay(100);
      myservo.detach();
      //reset timer 1 (fix bug in arduino libs)
      TCCR1B =_BV(CS11) | _BV(CS10);
      TCCR1A =_BV(WGM10);
      TIMSK1 = 0;
      //restore RGB
      pinMode(PIN_PWM_R, OUTPUT);
      pinMode(PIN_PWM_G, OUTPUT);
      pinMode(PIN_PWM_B, OUTPUT);
      analogWrite(PIN_PWM_R, up_vals[3]);
      analogWrite(PIN_PWM_G, up_vals[4]);
      analogWrite(PIN_PWM_B, up_vals[5]);
      break;
    case 'R':
      v = constrain(atoi(val), 0, 255);
      analogWrite(PIN_PWM_R, v);
      up_vals[3] = v;
      break;
    case 'G':
      v = constrain(atoi(val), 0, 255);
      analogWrite(PIN_PWM_G, v);
      up_vals[4] = v;
      break;
    case 'B':
      v = constrain(atoi(val), 0, 255);
      analogWrite(PIN_PWM_B, v);
      up_vals[5] = v;
      break;
    default:
  		sender->GetSerial()->println("ERROR INVALID_VAR");
      return;
  }

	sender->GetSerial()->print("ctrl(");
	sender->GetSerial()->print(var);
	sender->GetSerial()->print(",");
	sender->GetSerial()->print(val);
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

  if (strcmp(var, "ldr") == 0){
    up_ints[0] = interval;
	}	else if (strcmp(var, "ntc") == 0) {
    up_ints[1] = interval;
	}	else if (strcmp(var, "clock") == 0) {
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

void cmd_reset(SerialCommands* sender) {
  delay(5000);
	sender->GetSerial()->println("reset failed");
}

SerialCommand cmd_ctrl_("ctrl", cmd_ctrl);
SerialCommand cmd_bb_("bb", cmd_bb);
SerialCommand cmd_reset_("reset", cmd_reset);
SerialCommand cmd_update_("update", cmd_update);
SerialCommand cmd_report_("report", cmd_report);

void setup(){
  Serial.begin(115200);
  pinMode(PIN_CLOCK_TX, INPUT);
  pinMode(PIN_CLOCK_RX, OUTPUT);
  ss.begin(9600);
  if (!IRLremote.begin(PIN_IR))
    Serial.println(F("You did not choose a valid pin."));
  pinMode(PIN_PWM_R, OUTPUT);
  pinMode(PIN_PWM_G, OUTPUT);
  pinMode(PIN_PWM_B, OUTPUT);
  pinMode(PIN_COOLER, OUTPUT);
  pinMode(PIN_ANLG_VCC, OUTPUT);
  digitalWrite(PIN_ANLG_VCC, 1);

	serial_commands_.SetDefaultHandler(cmd_unrecognized);
	serial_commands_.AddCommand(&cmd_ctrl_);
	serial_commands_.AddCommand(&cmd_bb_);
	serial_commands_.AddCommand(&cmd_reset_);
	serial_commands_.AddCommand(&cmd_update_);
	serial_commands_.AddCommand(&cmd_report_);
	Serial.println("Ready!");

  for (int i = 0; i < 4; i++){
    up_timer[i] = millis();
  }
}

void loop(){
  serial_commands_.ReadSerial();

  while (ss.available() >= 18) { // bulbdial clock echo
    if (ss.read() == '>'){
      for (int i = 0; i < 17; i++){
        timestr[i] = ss.read();
      }
      timestr[17] = 0;
    }
  }

  if (millis() - up_timer[0] > up_ints[0]){
    up_vals[0] = analogRead(PIN_LDR);
    up_timer[0] = millis();
  }
  if (millis() - up_timer[1] > up_ints[1]){
    up_vals[1] = (int)(smoothThermistor.temperature()*100);
    up_timer[1] = millis();
  }
  if (millis() - up_timer[2] > up_ints[2]){
    ss.println("?          ");
    up_timer[2] = millis();
  }
  if (millis() - up_timer[3] > up_ints[3]){
    print_report();
    up_timer[3] = millis();
  }

  if (IRLremote.available()){
    auto data = IRLremote.read();
    Serial.print("R(");
    Serial.print(data.address, HEX);
    Serial.print(",");
    Serial.print(data.command, HEX);
    Serial.println(")");
  }
}
