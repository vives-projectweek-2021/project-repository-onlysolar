/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "platform/mbed_thread.h"
#include "INA260.hpp"

PwmOut horizontal(D7);   // servo on top
PwmOut vertical(D9);     // servo on base

AnalogIn TR(A0); //rechtsboven
AnalogIn TL(A1); //linksboven
AnalogIn DL(A2); //linksonder
AnalogIn DR(A3); //rechtsonder

I2C i2c1(PB_9,PB_8); //Toekenning SDA en SCL INA260 1
I2C i2c2(PB_11, PB_10); //Toekenning SDA en SCL INA260 2

INA260 sens1(i2c1);
INA260 sens2(i2c2);

Serial pc(USBTX, USBRX);

//INSTELWAARDES VERSCHILLENDE MODES:
//DIRECT VOLGEN,VEEL OVERSHOOT: 0.01  /  0.000004  /  0.04  /  0
//TRAAG: 0.001  /  0.000007  /  0.2  /  2
//SNEL: 0.08  /  0.00004  /  0.03  /  1
float foutmarge = 0.08;         //foutmarge LDR's 0.01 is 1%
float stapgrootte = 0.000007;   //stapgrootte per stap
float stapdelay = 0.2;          //tijd in seconden na een stap 
float waittime = 2;             //wachttijd nadat deze goed staat

bool horizontalReached = false; //als horizontal goed staat
bool verticalReached = false;   //als vertical goed staat

float hor_pos = 0.0005; //0.0005 = 0°, 0.0019 = 180°
float ver_pos = 0.0005;

float degrees_hor;
float degrees_ver;

Thread thread1; //thread voor PWM servo horizontal
Thread thread2; //thread voor PWM servo vertical

float Tgemiddelde;
float Dgemiddelde;
float Tmin;
float Tmax;
float Dmin;
float Dmax;

float Lgemiddelde;
float Rgemiddelde;
float Lmin;
float Lmax;
float Rmin;
float Rmax;

double V1, C1, P1;  //Spanning, stroom en vermogen sensor 1
double V2, C2, P2;  //Spanning, stroom en vermogen sensor 2

void calculateMargins(){  // tolerantie LDR's berekenen
    Tgemiddelde = (TL + TR)/2;     //horizontal-top-gemiddelde
    Dgemiddelde = (DL + DR)/2;     //horizontal-down-gemiddelde
    Tmin = (1 - foutmarge) * Tgemiddelde;
    Tmax = (1 + foutmarge) * Tgemiddelde;
    Dmin = (1 - foutmarge) * Dgemiddelde;
    Dmax = (1 + foutmarge) * Dgemiddelde;

    Lgemiddelde = (TL + DL)/2;     //vertical-left-gemiddelde
    Rgemiddelde = (TR + DR)/2;     //vertical-right-gemiddelde
    Lmin = (1 - foutmarge) * Lgemiddelde;
    Lmax = (1 + foutmarge) * Lgemiddelde;
    Rmin = (1 - foutmarge) * Rgemiddelde;
    Rmax = (1 + foutmarge) * Rgemiddelde;  
}

//checken of verticaal goed staat, anders servo bijsturen
void checkVertical(){
    if(((TL > Tmin && TL < Tmax)&&(TR > Tmin && TR < Tmax))&&((DL > Dmin && DL < Dmax)&&(DR > Dmin && DR < Dmax))){
        verticalReached = true;
    }
    else{
        if(TL > TR && DL > DR) {
            ver_pos = (ver_pos - stapgrootte);
            wait(stapdelay);    
        }
        if(TL < TR && DL < DR) {
            ver_pos = (ver_pos + stapgrootte);
            wait(stapdelay);
        }
    }
}

//checken of horizontaal goed staat, anders servo bijsturen
void checkHorizontal(){
    if(((TL > Lmin && TL < Lmax)&&(DL > Lmin && DL < Lmax))&&((TR > Rmin && TR < Rmax)&&(DR > Rmin && DR < Rmax))){
        horizontalReached = true;
    }
    else{
        if(TL > DL && TR > DR) {
            hor_pos = (hor_pos + stapgrootte);
            wait(stapdelay);
        }
        if(TL < DL && TR < DR) {
            hor_pos = (hor_pos - stapgrootte);
            wait(stapdelay);
        }
    }
}

//begrenzing periode servo's
void limitServos(){
    if(ver_pos > 0.0019){
        ver_pos = 0.0019;
    }
    if(ver_pos < 0.0005){
        ver_pos = 0.0005;
    }
    if(hor_pos > 0.0019){
        hor_pos = 0.0019;
    }
    if(hor_pos < 0.0005){
        hor_pos = 0.0005;
    }
}

//thread voor servo 1
void servo_hor_thread(float* hor_pos)
{
    horizontal.period(0.020);
    while (true) {
        horizontal.pulsewidth(*hor_pos);
    }
}

//thread voor servo 2
void servo_ver_thread(float* ver_pos)
{
    vertical.period(0.020);
    while (true) {
        vertical.pulsewidth(*ver_pos);
    }
}

//uitlezen stroom- en spanningssensoren (INA260)
void battery_reading()
{
    //sensor 1
    sens1.getVoltage(&V1);
    sens1.getCurrent(&C1);
    sens1.getPower(&P1);
    //sensor 2
    sens2.getVoltage(&V2);
    sens2.getCurrent(&C2);
    sens2.getPower(&P2);
}

int main() {

    thread1.start(callback(servo_hor_thread, &hor_pos));
    thread2.start(callback(servo_ver_thread, &ver_pos));
    
    sens1.setConfig(0x0600 | 0x01C0 | 0x0038 | 0x0007);
    sens2.setConfig(0x0600 | 0x01C0 | 0x0038 | 0x0007);
    
    while(1){
        calculateMargins();
        checkVertical();
        checkHorizontal();
        limitServos();
        //graden berekenen servo's
        degrees_hor = (hor_pos - 0.0005)/(0.0014/180);
        degrees_ver = (ver_pos - 0.0005)/(0.0014/180);
        //als horizontal en vertical goed staan, dan een bepaalde langere tijd wachten + waardes doorsturen  
        if(verticalReached && horizontalReached){
            battery_reading();
            pc.printf("%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n", degrees_hor, degrees_ver, V1, C1, P1, V2, C2, P2);
            wait(waittime);
            verticalReached = false;
            horizontalReached = false;
        }   
    }
}