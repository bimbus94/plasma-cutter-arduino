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
    int in_endStopType, // 0-mechanical endstop, 1 -optical endstop
    int in_moveType)
{
    //Przypisanie wartosci z config-u ->wiecej informacji w config.h
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

    moveType=in_moveType;

    //Przypisanie PINow i wartosci PINom w zaleznosci od config.h
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
    //Przypisanie wartosci PINom odpowiedzialnym za mikrokroki
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

    //LOW na step i kierunek
    digitalWrite(dirPin, LOW);
    digitalWrite(stepPin, LOW);

    currentStepcount=0; //aktualna liczba krokow
    targetStepcount=0; //docelowa liczba krokow
    targetStepcountInMM=0; //~~


    // Rozne stepy w zaleznosci od silnika dla osi Y
    // steps_per_mm = ilosc krokow w jednym obrocie / (Pi*srednica rury)*liczba mikro-krokow*przelozenie mechaniczne
    steps_per_mm = (int)((kStepsPerRevolution/(defaultDiameter*M_PI))*kMicroStepping*gearRatio+0.5); 

    //liczba krokow na 1 radian
    steps_per_rad=(int)((kStepsPerRevolution/(2*M_PI))*kMicroStepping*gearRatio+0.5);

    //liczba krokow na 1 stopien
    steps_per_deg=(int)((kStepsPerRevolution/(360))*kMicroStepping*gearRatio+0.5);
    
    //ustawienie domyslnej liczby krokow jako kroki na mm
    steps_per=steps_per_mm;

    enableStepper(false); // wylaczenie silnikow
}

//Funkcja do przypisania nowej srednicy dla silnika, oblicznie krokow na mm, ustawienie nowej srednicy
void StepperModel::resetSteppersForObjectDiameter(double diameter)
{
    //Obliczenie krokow na mm
    steps_per_mm = (int)((kStepsPerRevolution/(diameter*M_PI))*kMicroStepping*gearRatio+0.5);
    steps_per=steps_per_mm;
    //Ustawienie nowej srednicy 
    defaultDiameter=diameter;
    
    //Wyzerowanie, wylaczenie i reset silnikow
    if(endStopPin>=0)
    {

        autoHoming();

        enableStepper(false);
    }
    else
        resetStepper();
}


//FUNKCJA DO ZMIANY TYPU W JAKI SA ODMIERZANE KROKI, CZY TO RADIANY, MM, CZY STOPNIE steps_per_mm, steps_per_deg, steps_per_rad
void StepperModel::resetSteppersForMoveType(int type)
{
    moveType=type; // okreslenie typu w jaki bedzie odmierzany ruch mm/rad/deg
    
    //Obliczenie liczby krokow na 1mm
    if (type==1)
    {
        steps_per=steps_per_mm;
    }
    //Obliczenie liczby krokow na 1 rad
    else if (type==2)
    {
        steps_per=steps_per_rad;
    }
    //Obliczenie liczby krokow na 1 stopien
    else if (type==3)
    {
        steps_per=steps_per_deg;
    }

    //Wyzerowanie, wylacznie i reset silnikow
    if(endStopPin>=0)
    {   autoHoming();

        enableStepper(false);
    }
    else
        resetStepper();
}

//Funkcja zwraca liczbe krokow potrzebna na przebycie zadanego dystansu podanego w (mm,rad lub stopniach)
long StepperModel::getStepsForMM(double mm)
{
    long steps = (long)(steps_per*mm);

    return steps;
}

//Funkcja zwraca liczbe krokow potrzebna na przebycie zadanego dystansu podanego w mm, potrzebna by obliczyc dystans w MM, gdy oś jest zadeklarowana w radianach lub stopniach
long StepperModel::getStepsForMMinMM(double mm)
{
    long steps = (long)(steps_per_mm*mm);

    return steps;
}

//Funcja do obliczanie krokow potrzebnych do przebycia zadanej odleglosci, delty, a wiec roznicy w liczbie krokow miedzy aktualnym a oczekiwanym polozeniem, oraz ustaleniem docelowej pozycji w [mm], potrzebnej do okreslenia interwalow (predkosci silnikow), gdy jeden w nich operuje w [rad] lub [deg]
void StepperModel::setTargetPosition(double pos)
{
    targetPosition = pos; //pozycja docelowa w mm/rad/deg

    if (moveType==1)//targetPositionInMM- docelowa pozycja w [mm]
    {
        targetPositionInMM = pos;   //mm bez zmian, gdy operujemy na [mm]
    }
    else if(moveType==2)
    {
        targetPositionInMM = pos*(double)defaultDiameter/2.0;   // odleglosc w radianach przeliczana na [mm]: pos[rad] * srednica[mm] /2
    }
    else if(moveType==3)
    {
        targetPositionInMM = pos/360.0*M_PI*(double)defaultDiameter;   // odleglosc w stopniach przeliczana na [mm]: pos[deg]/(2*180[deg])*Pi*srednica[mm]
    }

    targetStepcount = getStepsForMM(targetPosition); //docelowa liczba krokow
    delta = targetStepcount-currentStepcount; //przyrost liczby krokow, potrzebny do osiagniecia docelowej pozycji

    direction = true; //okreslenie kierunku ruchu w zaleznosci od znaku przyrostu - delta
    if (delta != 0)
    {
        enableStepper(true);
    }
    if (delta < 0)
    {
        delta = -delta;
        direction = false;
    }
}

//Funckja zwracajaca pozycje w rad/mm/stopniach w zaleznosci od typu pracy silnika
double StepperModel::getCurrentPosition()
{
    return (double)currentStepcount/steps_per;
}

//Funkcja zwracajaca pozycje w mm do obliczenia dystansu i interwalow
double StepperModel::getCurrentPositionInMM()
{
    return (double)currentStepcount/steps_per_mm;
}

//Funkcja do wlaczania silnikow
void StepperModel::enableStepper(bool enabled)
{
    digitalWrite(enablePin, !enabled);
}

//Funkcja do resetu silnikow
void StepperModel::resetStepper()
{
    enableStepper(false);
    currentStepcount=0;
    targetStepcount=0;
    delta=0;
}

//Funkcja do poruszania - robienia stepow - silnikami z zadanymi interwalami
void StepperModel::doStep(long intervals)
{
    counter += delta;
    if (counter >= 0)
    {
        digitalWrite(dirPin, direction?HIGH:LOW);
        counter -= intervals;
        if (direction)
        {
            if(maxStepCount==0 || currentStepcount<=maxStepCount)
            {
                digitalWrite(stepPin, HIGH);
                currentStepcount++;
            }
        }
        else
        {
            if(minStepCount==0 || currentStepcount>=minStepCount)
            {
                digitalWrite(stepPin, HIGH);
                currentStepcount--;
            }
        }
        digitalWrite(stepPin, LOW);
    }
}


//Funkcja do autohomingu - zerowania polozenia silnikow z wykorzystaniem endstopow
void StepperModel::autoHoming()
{
    enableStepper(true);
    digitalWrite(dirPin, LOW);
    if (endStopType==0) //endstop mechaniczy - nacisniecie (zwarcie) endstopu powoduje zatrzymanie silnika i ustalenie 0
    {
        while(digitalRead(endStopPin))
        {
            digitalWrite(stepPin, HIGH);
            digitalWrite(stepPin, LOW);
            delay(2);
        }
    }

    else //optyczny endstop - przerwanie wiazki w fotokomorce powoduje zatrzymanie silnika i ustalenie 0
    {
        while(!(digitalRead(endStopPin)))
        {
            digitalWrite(stepPin, HIGH);
            digitalWrite(stepPin, LOW);
            delay(1);
        }

    }

    currentStepcount= minStepCount-16; // Przypisanie liczby stepow w 0, po wyzerowaniu
}

