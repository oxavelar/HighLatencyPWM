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

#include "PWM.hh"

#include <fstream>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/stat.h>

#include <iostream>
using std::cerr;
using std::endl;



const std::string PWM::_sysfsPath("/sys/class/pwm/");


PWM::PWM(unsigned short id) :
   _id(id),
   _id_str(std::to_string(id)),
    _dutycycle(50),
   _period_ns(2000000),
   _state(PWM::State::DISABLED)
{
   initCommon();
}


PWM::PWM(unsigned short id, PWM::Dutycycle dutycycle, PWM::Period period_ns) :
   _id(id),
   _id_str(std::to_string(id)),
   _dutycycle(dutycycle),
   _period_ns(period_ns),
   _state(PWM::State::DISABLED)
{
   initCommon();
}


void PWM::initCommon(void) const
{
   /* validate id # */
   {
      if( !boost::filesystem::exists(_sysfsPath) )
      {
         throw std::runtime_error(_sysfsPath + " does not exist.");
      }

      using boost::filesystem::directory_iterator;
      const directory_iterator end_itr; /* default construction yields past-the-end */

      bool found = false;
      for( directory_iterator itr(_sysfsPath); itr != end_itr; ++itr)
      {
         if( is_directory(itr->status()) &&
             itr->path().string().find("pwmchip") != std::string::npos )
         {
            std::ifstream infile(itr->path().string() + "/base");
            if( !infile )
            {
               throw std::runtime_error("Unable to read  " + itr->path().string() + "/base");
            }

            /*
               There is no way to be fast and const-correct here :(
               http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
            */
            std::string base;
            infile >> base;
            infile.close();

            infile.open(itr->path().string() + "/npwm");
            if( !infile )
            {
               throw std::runtime_error("Unable to read  " + itr->path().string() + "/npwm");
            }

            std::string npwm;
            infile >> npwm;
            infile.close();

            /* PWM id is valid */
            if( std::stoul(base) <= _id && _id < std::stoul(base) + std::stoul(npwm) )
            {
               found = true;
               break;
            }
         }
      }
      if( !found )
      {
         throw std::runtime_error("PWM " + _id_str + " is invalid");
      }
   }



   /* validate not already exported */
   {
      /* In decreasing order of speed: stat() -> access() -> fopen() -> ifstream */
      struct stat stat_buf;
      const std::string path(_sysfsPath + "pwm" + _id_str);
      if( stat(path.c_str() , &stat_buf) == 0 )
      {
         throw std::runtime_error(
            "PWM " + _id_str + " already exported." +
            "(Some other PWM object already owns this PWM)");
      }
   }



   /* attempt to export */
   {
      std::ofstream sysfs_export(_sysfsPath + "export", std::ofstream::app);
      if( !sysfs_export.is_open() )
      {
         throw std::runtime_error("Unable to export PWM " + _id_str);
      }
      sysfs_export << _id_str;
      sysfs_export.close();
   }
   
   /* Common initializations on default behaivour */
   //setState(_state);
   //setPeriod(_period);
   //setDutyCycle(_dutycycle);

}

PWM::~PWM()
{
   /* attempt to unexport */
   try
   {
      std::ofstream sysfs_unexport(_sysfsPath + "unexport", std::ofstream::app);
      if( sysfs_unexport.is_open() )
      {
         sysfs_unexport << _id_str;
         sysfs_unexport.close();
      }
      else /* Do not throw exception in destructor! Effective C++ Item 8. */
      {
         cerr << "Unable to unexport PWM " + _id_str + "!" << endl;
         cerr << "This will prevent initialization of another PWM object for this PWM." << endl;
      }
   }
   catch(...)
   {
      cerr << "Exception caught in destructor for PWM " << _id_str << endl;
   }
}


void PWM::setState(const PWM::State &state)
{
   /* attempt to set disable by default */
   {
      std::ofstream sysfs_enable(
         _sysfsPath + "gpio" + _id_str + "/enable", std::ofstream::app);
      if( !sysfs_enable.is_open() )
      {
         throw std::runtime_error("Unable to disable PWM " + _id_str);
      }
      if( state == PWM::State::DISABLED )      sysfs_enable << "0";
      else if( state == PWM::State::ENABLED )  sysfs_enable << "1";
      sysfs_enable.close();
   }
   _state = state;
}


PWM::State PWM::getState(void)
{
   std::ifstream sysfs_value(_sysfsPath + "pwm" + _id_str + "/enable");
   if( !sysfs_value.is_open() )
   {
      throw std::runtime_error("Unable to obtain enabled value for PWM " + _id_str);
   }

   const char value = sysfs_value.get();
   if( !sysfs_value.good() )
   {
      throw std::runtime_error("Unable to obtain enabled value for PWM " + _id_str);
   }
   
   State status = PWM::State::DISABLED;
   if ( value == '1' )  status = PWM::State::ENABLED;

   return(status);
}



void PWM::setDutyCycle(const PWM::Dutycycle &value)
{
   if( (value > 100) or (value < 0) )
   {
      throw std::runtime_error("Pick a value between 0 and 100 for the PWM dutycycle");
   }

   std::ofstream sysfs_value(_sysfsPath + "pwm" + _id_str + "/duty_cycle", std::ofstream::app);
   if( !sysfs_value.is_open() )
   {
      throw std::runtime_error("Unable to set dutycycle value for PWM " + _id_str);
   }

   sysfs_value << std::to_string(value);
   sysfs_value.close();
   _dutycycle = value;
}


PWM::Dutycycle PWM::getDutyCycle(void)
{
   std::ifstream sysfs_value(_sysfsPath + "pwm" + _id_str + "/duty_cycle");
   if( !sysfs_value.is_open() )
   {
      throw std::runtime_error("Unable to get dutycycle value for PWM " + _id_str);
   }

   const char value = sysfs_value.get();
   if( !sysfs_value.good() )
   {
      throw std::runtime_error("Unable to get dutycycle value for PWM " + _id_str);
   }

   Dutycycle val = value - '0';
   _dutycycle = val;
   return(val);
}

