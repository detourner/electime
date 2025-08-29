#include "gauge_freq_meter.h"

#define STEP_FREQ_MIN    49.90  
#define STEP_FREQ_MAX    50.10
#define STEP_SETP_MIN    0
#define STEP_STEP_MAX    3500

GaugeFreqMeter::GaugeFreqMeter()
{

}

void GaugeFreqMeter::begin(    const unsigned char pinStep,
                                const unsigned char pinDir,
                                const unsigned char pinReset )
{
    _gauge.begin(pinStep, pinDir, pinReset);
}


void GaugeFreqMeter::reset(void)
{
    _gauge.zero();
}

void GaugeFreqMeter::setPosition(const float freq)
{
    if(freq != _currentFreq)
    {
        _currentFreq = freq;
        unsigned int pos = (unsigned int)((freq - STEP_FREQ_MIN) * (STEP_STEP_MAX - STEP_SETP_MIN) / (STEP_FREQ_MAX - STEP_FREQ_MIN) + STEP_SETP_MIN);
        if (pos < STEP_SETP_MIN) pos = STEP_SETP_MIN;
        if (pos > STEP_STEP_MAX) pos = STEP_STEP_MAX;
        Serial.print("new pos:");
        Serial.println(pos);
        _gauge.setPosition(pos);
    }
    _currentFreq = freq;
}

void GaugeFreqMeter::setStep(const unsigned int posStep)
{
    _gauge.setPosition(posStep);
}
