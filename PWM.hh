/*
The MIT License (MIT)

Copyright (c) 2014 Thomas Mercier Jr.

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

#ifndef PWM_HH
#define PWM_HH

#include "Uncopyable.hh"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

// LOCKFREE define specifies the use of a (single producer, single consumer) lockfree container for
// the transfer of transition events from the thread which detects these events, to the thread which
// will call the user-provided callback function. This implementation is EXTREMELY wasteful of CPU
// time!!! However, it provides about 0.5 ms lower latency than the default implementation. If lower
// latency is required, please one of the BeagleBone Black PRUs, or a kernel module.
#ifdef LOCKFREE
   #include <boost/lockfree/spsc_queue.hpp>
#else
   #include <queue>
   #include <mutex>
   #include <condition_variable>
#endif


class PWM : private Uncopyable
{
public:

   //-----------------------------------------------------------------------------------------------
   /// @enum Direction
   /// @brief Type used to configure a PWM as an input or output
   //-----------------------------------------------------------------------------------------------
   enum class Direction : char {
      IN,
      OUT
   };

   //-----------------------------------------------------------------------------------------------
   /// @enum Value
   /// @brief Type used to indicate the logic level of a PWM
   //-----------------------------------------------------------------------------------------------
   enum class Value : char {
      HIGH,
      LOW
   };

   //-----------------------------------------------------------------------------------------------
   /// @enum Edge
   /// @brief Type used to indicate which logic level transitions on an input PWM should result in
   ///        a call to the user-provided callback function.
   //-----------------------------------------------------------------------------------------------
   enum class Edge : char {
      NONE,
      RISING,
      FALLING,
      BOTH
   };


   //-----------------------------------------------------------------------------------------------
   // FUNCTION NAME: PWM (constructor)
   ///
   /// @brief Construt an input or output PWM object.
   ///
   /// @param[in]   id         The PWM ID. Often referred to a "pin number".
   /// @param[in]   direction  The type (INPUT or OUTPUT) of PWM to construct.
   ///
   //-----------------------------------------------------------------------------------------------
   explicit PWM(
      unsigned short id,
      Direction direction);


   //-----------------------------------------------------------------------------------------------
   // FUNCTION NAME: PWM (constructor)
   ///
   /// @brief Construt an input PWM object which will call a callback function every time a
   ///        transition of type edge occurs.
   ///
   ///
   /// @param[in]   id    The PWM ID. Often referred to a "pin number".
   /// @param[in]   edge  The type (INPUT or OUTPUT) of PWM to construct.
   /// @param[in]   isr   The function to call when transitions of type edge occur.
   ///
   /// @note If function isr throws an exception, IT WILL NOT BE HANDLED OR IGNORED BY THIS CLASS.
   ///
   //-----------------------------------------------------------------------------------------------
   explicit PWM(
      unsigned short id,
      Edge edge,
      std::function<void(Value)> isr);


   //-----------------------------------------------------------------------------------------------
   // FUNCTION NAME: PWM (destructor)
   ///
   /// @note If a program which uses this class is ungracefully terminated (this destructor is not
   ///       called), the PWM will be left in an exported state which will prevent subsequent
   ///       construction of a PWM object with the same id.
   //-----------------------------------------------------------------------------------------------
   ~PWM();


   //-----------------------------------------------------------------------------------------------
   // FUNCTION NAME: setValue
   ///
   /// @brief Set the logical value (HIGH or LOW) of the PWM. All PWMs are active-high.
   ///
   /// @param[in]   value    The logical value to set.
   ///
   /// @return None
   ///
   //-----------------------------------------------------------------------------------------------
   void setValue(const Value value) const;


   //-----------------------------------------------------------------------------------------------
   // FUNCTION NAME: getValue
   ///
   /// @brief Get the logical value (HIGH or LOW) of the PWM. All PWMs are active-high.
   ///
   /// @return The logical value of the PWM.
   ///
   //-----------------------------------------------------------------------------------------------
   Value getValue() const;


private:
   void initCommon() const;
   void pollLoop();
   void isrLoop();

private:
   static const std::string  _sysfsPath;

   const unsigned short _id;
   const std::string    _id_str;
   const Direction      _direction;

   const Edge _edge;
   const std::function<void(Value)> _isr;

   std::thread _pollThread;
   int _pollFD;

   std::thread _isrThread;

   std::atomic<bool> _destructing;
   int               _pipeFD[2];

#ifdef LOCKFREE
   boost::lockfree::spsc_queue<Value, boost::lockfree::capacity<64>> _spsc_queue;
#else
   std::queue<Value>        _eventQueue; // stores values generated by interrupts
   std::mutex               _eventMutex;
   std::condition_variable  _eventCV;
#endif

};

#endif
