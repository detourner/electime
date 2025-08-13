/*
 *  SwitecX12 Arduino Library
 *  Guy Carpenter, Clearwater Software - 2017
 *
 *  Licensed under the BSD2 license, see license.txt for details.
 *
 *  All text above must be included in any redistribution.
 */

#include "SwitecX12.h"

// This table defines the acceleration curve.
// 1st value is the speed step, 2nd value is delay in microseconds
// 1st value in each row must be > 1st value in subsequent row
// 1st value in last row should be == maxVel, must be <= maxVel
static unsigned short defaultAccelTable[][2] = {
  {   20, 4000},
  {   50, 2000},
  {  100, 1000},
  {  150, 750},
  {  300, 450}
};

const int stepPulseMicrosec = 1;
const int resetStepMicrosec = 300;
#define DEFAULT_ACCEL_TABLE_SIZE (sizeof(defaultAccelTable)/sizeof(*defaultAccelTable))

SwitecX12::SwitecX12()
{
}

void SwitecX12::irqTimerCallback(void * context)
{
    if(context == NULL)
    {
        return;
    }
    // 17us to execute advance();
    SwitecX12 * ptrSwitecX12 = (SwitecX12 *) context;
    ptrSwitecX12->advance();

}


void SwitecX12::begin(unsigned char pinStep, unsigned char pinDir, unsigned char pinReset)
{
        begin(pinStep, pinDir, pinReset, defaultSteps);
}
void SwitecX12::begin(unsigned char pinStep, unsigned char pinDir, unsigned char pinReset, unsigned int steps)
{
  this->steps = steps;
  this->pinStep = pinStep;
  this->pinDir = pinDir;
  pinMode(pinStep, OUTPUT);
  pinMode(pinDir, OUTPUT);

  digitalWrite(pinReset, HIGH);
  pinMode(pinReset, OUTPUT);

  digitalWrite(pinReset, LOW);
  digitalWrite(pinStep, LOW);
  digitalWrite(pinDir, LOW);
  delay(1);  // keep reset low min 1ms
  digitalWrite(pinReset, HIGH);

  dir = 0;
  vel = 0;
  stopped = true;
  currentStep = 0;
  targetStep = 0;

  accelTable = defaultAccelTable;
  maxVel = defaultAccelTable[DEFAULT_ACCEL_TABLE_SIZE-1][0]; // last value in table.

  const esp_timer_create_args_t periodic_timer_args = {
    .callback = &(SwitecX12::irqTimerCallback),
    /* name is optional, but may help identify the timer when debugging */
    .arg = (void*) this,
    .name = "switecX12"
  };
  esp_timer_create(&periodic_timer_args, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, 500);
}



void SwitecX12::step(int dir)
{
  digitalWrite(pinDir, dir > 0 ? HIGH : LOW);
  digitalWrite(pinStep, HIGH);
  delayMicroseconds(stepPulseMicrosec);
  digitalWrite(pinStep, LOW);
  currentStep += dir;
}

void SwitecX12::stepTo(int position)
{
  int count;
  int dir;

  esp_timer_stop(periodic_timer);
  if (position > currentStep) {
    dir = 1;
    count = position - currentStep;
  } else {
    dir = -1;
    count = currentStep - position;
  }
  for (int i=0;i<count;i++) {
    step(dir);
    delayMicroseconds(resetStepMicrosec);
  }
}

void SwitecX12::zero()
{
  currentStep = steps - 1;
  stepTo(0);
  targetStep = 0;
  vel = 0;
  dir = 0;
}

void SwitecX12::advance(void)
{
 if(stopped == true)
 {
     return;
 }
  // detect stopped state
  if (currentStep==targetStep && vel==0) {
    stopped = true;
    dir = 0;
    //time0 = micros();
    esp_timer_stop(periodic_timer);
    return;
  }

  // if stopped, determine direction
  if (vel==0) {
    dir = currentStep<targetStep ? 1 : -1;
    // do not set to 0 or it could go negative in case 2 below
    vel = 1;
  }

  step(dir);

  // determine delta, number of steps in current direction to target.
  // may be negative if we are headed away from target
  int delta = dir>0 ? targetStep-currentStep : currentStep-targetStep;

    if (delta>0)
    {
    // case 1 : moving towards target (maybe under accel or decel)
        if (delta < vel)
        {
            // time to declerate
            vel--;
        }
        else if (vel < maxVel)
        {
            // accelerating
            vel++;
        }
        else
        {
            // at full speed - stay there
        }
    }
    else
    {
        // case 2 : at or moving away from target (slow down!)
        vel--;
    }

  // vel now defines delay
  unsigned char i = 0;
  // this is why vel must not be greater than the last vel in the table.
  while (accelTable[i][0]<vel) {
    i++;
  }
  esp_timer_start_periodic(periodic_timer, accelTable[i][1]);

  //time0 = micros();
}

void SwitecX12::setPosition(unsigned int pos)
{
  // pos is unsigned so don't need to check for <0
  esp_timer_stop(periodic_timer);

  if (pos >= steps) pos = steps-1;
  targetStep = pos;
  if (stopped)
  {
    // reset the timer to avoid possible time overflow giving spurious deltas
    stopped = false;
    vel = 0;
    esp_timer_start_periodic(periodic_timer, 500);
  }  
}

