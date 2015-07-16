/*
The MIT License (MIT)

Copyright (c) 2014 Thomas Mercier Jr & Omar X. Avelar.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef GPIO_HH
#define GPIO_HH

#include "Uncopyable.hh"

#include <atomic>
#include <functional>
#include <string>
#include <thread>


class PWM : private Uncopyable
{
public:
    
    enum class State : char { DISABLED, ENABLED };

    using Dutycycle = long;
    using Period = long;
    
    explicit PWM(unsigned short id);
    explicit PWM(unsigned short id, Dutycycle value, Period period_ns);
    virtual ~PWM(void);

    void setState(const State &state);
    void setDutyCycle(const Dutycycle &dutycycle);
    void setPeriod(const Period &period_ns);

    State getState(void);
    Dutycycle getDutyCycle(void);
    Period getPeriod(void);


private:
    static const std::string  _sysfsPath;

    const unsigned short _id;
    const std::string _id_str;
    Dutycycle _dutycycle;
    Period _period_ns;
    State _state = State::DISABLED;

    void initCommon(void) const;
};

#endif
