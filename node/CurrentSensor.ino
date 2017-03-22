
//current sensor property variables
int NO_FIELD = 512;
float LEVELS_PER_AMP = 38.0;
int readings[SENSOR_RDG_PER_AVG];
float averages[SENSOR_RDG_PER_PACKET];
int rdgIndex;
int avgIndex;
int cur_currentAvg;
bool readyToSend;

//current sensor Setup
void currentSensor_setup() {
  Serial.println("Set up current sensor.");

  //Initilize fill buffers indices
  rdgIndex = 0;
  avgIndex = 0;

  readyToSend = false;

  //Enable timer to 1 ms
//  OCR0A = 0xAF;
//  TIMSK0 |= _BV(OCIE0A);
  //Enable timer to 1 s
  Timer1.initialize(1000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt( sensorUpdate ); // attach the service routine here

  //Calibrate
  calibrate();
}

//functions here
void calibrate() {
  //Assumes relay is turned off at startup
  int NO_FIELD = analogRead(CURRENT_SENSOR);
}

int getReading() {
  return analogRead(CURRENT_SENSOR) - NO_FIELD;
}

void sensorUpdate() 
{
  if(getIsConfigured()) {
    if(readyToSend){} //do not collect readings while sending
    else {
      readings[rdgIndex] = sq(getReading());
      rdgIndex++;
    
      if(rdgIndex > SENSOR_RDG_PER_AVG-1) {
        setAverage();
        rdgIndex = 0;
      }
    }  
  }
}

void setAverage() {
  int total = 0;
  for(int i = 0; i < 99; i++) {
    total += readings[i];
  }
  averages[avgIndex] = sqrt((float)(total)/SENSOR_RDG_PER_AVG)/LEVELS_PER_AMP;
  avgIndex += 1;
  if(avgIndex > SENSOR_RDG_PER_PACKET-1) {
    //Serial.println("Ready to Send");
    avgIndex = 0;
    readyToSend = true;
  } 
}

bool isReadyToSend() {
  return readyToSend;
}

void doneSending() {
  readyToSend = false;
}



