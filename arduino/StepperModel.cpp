/*
 * Copyright 2011 by Eberhard Rensch <http://pleasantsoftware.com/developer/3d>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/*Changed by Jacek Korbel 2018 from EggBot to PlasmaCutter for Tubes*/
 
#include "StepperModel.h"
#include "Arduino.h"


/*
 * inEnablePin < 0 => No Endstop
 */
StepperModel::StepperModel(
        int inDirPin, int inStepPin, int inEnablePin, int inEndStopPin, 
        int inMs1Pin, int inMs2Pin, int inMs3Pin, 
        int inSleepPin, int inResetPin,
        bool vms1, bool vms2, bool vms3, 
        long minSC, long maxSC,
        double in_kStepsPerRevolution, int in_kMicroStepping, int in_gearRatio,
		double in_defaultDiameter,
		int in_endStopType) // 0-mechanical endstop, 1 -optical endstop
{
  kStepsPerRevolution=in_kStepsPerRevolution;
  kMicroStepping=in_kMicroStepping;

  gearRatio=in_gearRatio;

  dirPin = inDirPin;
  stepPin = inStepPin;
  enablePin = inEnablePin;
  endStopPin = inEndStopPin;
  sleepPin = inSleepPin;
  resetPin = inResetPin;
  ms1Pin = inMs1Pin;
  ms2Pin = inMs2Pin;
  ms3Pin = inMs3Pin;
    
  minStepCount=minSC;
  maxStepCount=maxSC;

  defaultDiameter=in_defaultDiameter;

  endStopType=in_endStopType;

  
  pinMode(dirPin, OUTPUT);  
  pinMode(stepPin, OUTPUT);  
  pinMode(enablePin, OUTPUT);  
  if((sleepPin >=0))
  {
    pinMode(sleepPin, OUTPUT);
    digitalWrite(sleepPin, HIGH);
  }
  if((resetPin >=0))
  {
    pinMode(resetPin, OUTPUT);   
    digitalWrite(resetPin, HIGH); 
  }
  if(endStopPin>=0)
    pinMode(endStopPin, INPUT); 
  if((ms1Pin >=0))
  {
	  pinMode(ms1Pin, OUTPUT);  	
	  digitalWrite(ms1Pin, vms1);    
  }
  if((ms2Pin >=0))
  {
	  pinMode(ms2Pin, OUTPUT);  	
	  digitalWrite(ms2Pin, vms1);    
  }
  if((ms3Pin >=0))
  {
	  pinMode(ms3Pin, OUTPUT);  	
	  digitalWrite(ms3Pin, vms1);    
  }

  digitalWrite(dirPin, LOW);
  digitalWrite(stepPin, LOW);
 
  currentStepcount=0;
  targetStepcount=0;

// rozne stepy w zaleznosci od silnika dla osi Y
// steps_per_mm - trzeba dodac zmiane srednicy w kodzie = ilosc krokow w jednym obrocie / (Pi*srednica rury)*liczba mikro-krokow*przelozenie mechaniczne
  steps_per_mm = (int)((kStepsPerRevolution/(defaultDiameter*M_PI))*kMicroStepping*gearRatio+0.5); // default value for a example 100 diameter

//liczba krokow na 1 radian 
  steps_per_rad=(int)((kStepsPerRevolution/(2*M_PI))*kMicroStepping*gearRatio+0.5);

//liczba krokow na 1 stopien
  steps_per_deg=(int)((kStepsPerRevolution/(360))*kMicroStepping*gearRatio+0.5);

  steps_per=steps_per_mm;

  enableStepper(false);
}

void StepperModel::resetSteppersForObjectDiameter(double diameter)
{
  // Calculate the motor steps required to move per mm.
  steps_per_mm = (int)((kStepsPerRevolution/(diameter*M_PI))*kMicroStepping*gearRatio+0.5);
  steps_per=steps_per_mm;
  
  if(endStopPin>=0)
  {
#ifdef AUTO_HOMING
    autoHoming();
#endif
    enableStepper(false);
  }
  else
    resetStepper();    
}


//FUNKCJA DO ZMIANY TYPU W JAKI SA ODMIERZANE KROKI, CZY TO RADIANY, MM, CZY STOPNIE steps_per_mm, steps_per_deg, steps_per_rad
void StepperModel::resetSteppersForMoveType(int type)
{
  // Calculate the motor steps required to move per mm.
  if (type==1){steps_per=steps_per_mm;}//(int)((kStepsPerRevolution/(diameter*M_PI))*kMicroStepping*gearRation+0.5)};
  // Calculate the motor steps required to move per 1 rad
  else if (type==2){steps_per=steps_per_rad;}
   // Calculate the motor steps required to move per 1 degree
  else if (type==3){steps_per=steps_per_deg;}
  
  if(endStopPin>=0)
  {
//#ifdef AUTO_HOMING
    autoHoming();
//#endif
    enableStepper(false);
  }
  else
    resetStepper();    
}


long StepperModel::getStepsForMM(double mm)
{
  long steps = (long)(steps_per*mm);
  
//  Serial.print("steps for ");
//  Serial.print(mm);
//  Serial.print(" mm: ");
//  Serial.println(steps);
  
  return steps;
}

/* Currently unused */
/*
void StepperModel::setTargetStepcount(long tsc)
{
   targetPosition = (double)tsc/steps_per_mm;
   targetStepcount = tsc;
   delta = targetStepcount-currentStepcount;
   direction = true;
   if (delta != 0) {
     enableStepper(true);
   }
   if (delta < 0) {
	delta = -delta;
	direction = false;
   }
}*/

void StepperModel::setTargetPosition(double pos)
{
   targetPosition = pos;
   targetStepcount = getStepsForMM(targetPosition);
   //Serial.println(targetStepcount);
   delta = targetStepcount-currentStepcount;
   direction = true;
   if (delta != 0) {
     enableStepper(true);
   }
   if (delta < 0) {
	delta = -delta;
	direction = false;
   }
}

double StepperModel::getCurrentPosition()
{
    return (double)currentStepcount/steps_per;
}

void StepperModel::enableStepper(bool enabled)
{
  digitalWrite(enablePin, !enabled);
}

void StepperModel::resetStepper()
{
  enableStepper(false);
  currentStepcount=0;
  targetStepcount=0;
  delta=0;  
}

void StepperModel::doStep(long intervals)
{
  counter += delta;
  if (counter >= 0) {
    digitalWrite(dirPin, direction?HIGH:LOW);
    counter -= intervals;
    if (direction) {
      if(maxStepCount==0 || currentStepcount<=maxStepCount)
      {
        digitalWrite(stepPin, HIGH);
        currentStepcount++;
      }
    } else {
      if(minStepCount==0 || currentStepcount>=minStepCount)
      {
        digitalWrite(stepPin, HIGH);
        currentStepcount--;
      }
    }
    digitalWrite(stepPin, LOW);
  }
}  

//#ifdef AUTO_HOMING
void StepperModel::autoHoming()
{
  enableStepper(true);
  digitalWrite(dirPin, LOW);
 if (endStopType==0) //MECHANICAL ENDSTOP
 {
  	while(digitalRead(endStopPin))
  		{
        digitalWrite(stepPin, HIGH);
        digitalWrite(stepPin, LOW);
        delay(2);
  		}
}

else //OPTICAL ENDSTOP
{
	  while(!(digitalRead(endStopPin)))
  		{
        digitalWrite(stepPin, HIGH);
        digitalWrite(stepPin, LOW);
        delay(1);
  		}

}

  currentStepcount= minStepCount-16;
}
//#endif
