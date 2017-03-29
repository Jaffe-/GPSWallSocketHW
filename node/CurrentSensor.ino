
//current sensor property variables
float NO_FIELD = 512;
float LEVELS_PER_AMP = 38.0;
float readings[SENSOR_RDG_PER_AVG];
float averages[SENSOR_RDG_PER_PACKET];
int rdgIndex;
int avgIndex;
bool readyToSend;

//current sensor Setup
void currentSensor_setup() {
  Serial.println("Set up current sensor.");
  pinMode(CURRENT_SENSOR, INPUT);

  //Initilize fill buffers indices
  rdgIndex = 0;
  avgIndex = 0;

  readyToSend = false;

  //Enable timer to 1 ms
//  OCR0A = 0xAF;
//  TIMSK0 |= _BV(OCIE0A);
  //Enable timer to 1 s
  Timer1.initialize(1000); // set a timer of length 1000 microseconds (or 0.01 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt( sensorUpdate ); // attach the service routine here

  //Calibrate
  calibrate();
}

//functions here
void calibrate() {
  //Assumes relay is turned off at startup
  NO_FIELD = 512; //analogRead(CURRENT_SENSOR);
}

float getReading(float rdg, float mean) {
//  float rdg = (float)analogRead(CURRENT_SENSOR);
//  Serial.print("rdg: ");
 // Serial.println(rdg);
  return pow((rdg - mean)/LEVELS_PER_AMP, 2);
}

void sensorUpdate() 
{
  if(getIsConfigured()) {
    if(readyToSend){} //do not collect readings while sending
    else {
      readings[rdgIndex] = (float)analogRead(CURRENT_SENSOR); //pow(getReading(), 2);
//      Serial.print("rdg: ");
//      Serial.println(readings[rdgIndex]);
  
      
      rdgIndex++;
    
      if(rdgIndex >= SENSOR_RDG_PER_AVG) {
        setAverage();
        rdgIndex = 0;
      }
    }  
  }
}

void setAverage() {
  float total = 0;
  for(int i = 0; i < SENSOR_RDG_PER_AVG; i++) {
    total += readings[i];
  }
  float mean = total/SENSOR_RDG_PER_AVG;

  total = 0;
  for(int i = 0; i < SENSOR_RDG_PER_AVG; i++) {
    total += getReading(readings[i], mean);
  }
  
  averages[avgIndex] = sqrt(total/SENSOR_RDG_PER_AVG);
  //Serial.println(averages[avgIndex]);
  avgIndex += 1;
  if(avgIndex > SENSOR_RDG_PER_PACKET-1) {
    //Serial.println("Ready to Send");
    avgIndex = 0;
    readyToSend = true;
  } 
}

float* getAverageArray() {
  return averages;
}

bool isReadyToSend() {
  return readyToSend;
}

void doneSending() {
  readyToSend = false;
}



