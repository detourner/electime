#ifndef SwitecX12_h
#define SwitecX12_h

#include <Arduino.h>
#include <esp32-hal-timer.h>


class SwitecX12 {
  public:

        SwitecX12();

        void begin(unsigned char pinStep, unsigned char pinDir, unsigned char pinReset);
        void begin(unsigned char pinStep, unsigned char pinDir,  unsigned char pinReset, unsigned int steps);
        //void stepUp();

        void zero();
        void setPosition(unsigned int pos);
        bool Stopped(void) { return stopped; }
        unsigned int Steps(void) { return steps; }

    private :

        void stepTo(int position);
        void step(int dir);
        void advance();
        
        static void irqTimerCallback(void * context);
        
        const unsigned int defaultSteps = 315 * 12;

        unsigned char pinStep;
        unsigned char pinDir;
        unsigned int steps;            // total steps available
        unsigned short (*accelTable)[2]; // accel table can be modified.

        volatile unsigned int currentStep;      // step we are currently at
        volatile unsigned int targetStep;       // target we are moving to


        volatile unsigned int maxVel;           // fastest vel allowed
        volatile unsigned int vel;              // steps travelled under acceleration
        volatile signed char dir;             // direction -1,0,1
        volatile boolean stopped;               // true if stopped


        esp_timer_handle_t periodic_timer;

};

#endif
