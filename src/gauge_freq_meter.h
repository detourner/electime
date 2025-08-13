#ifndef GAUGE_FREQ_METER_H
#define GAUGE_FREQ_METER_H

#include "SwitecX12.h"

class GaugeFreqMeter
{

    public:

        GaugeFreqMeter();

        void begin( const unsigned char pinStep,
                    const unsigned char pinDir,
                    const unsigned char pinReset );

        void reset(void);

        void setPosition(const float freq);

        void setStep(const unsigned int posStep);

        bool stopped() { return _gauge.Stopped(); }

    private:
        SwitecX12   _gauge;
        float  _currentFreq = 0.0f; // Current frequency
};

#endif
