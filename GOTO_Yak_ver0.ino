//Libaries definition
//
#include <Time.h>
#include <TimeLib.h>
#include <SPI.h>
#include <math.h>
#include "AlignStars.h"
//#include <WiFiNINA.h>
//Constants definition
//
const int Mount_type = 1;   //1=Equatorial Mount; 2=AltAz Mount
const int Gear_ratio = 702;
const int Motor_steps = 200;
const int Microsteps = 1;
const int min2Meridian_flip = 3;
const double Sidereal_day = 86164.1;  //Sidereal day in seconds
const int H_timezone = 2;    //Israel TZ GMT+2
const int min_to_meridian = 5;
const double pi = 3.14159;
//
//Pin out definition
// RA motor/Az motor
#define X_STEP_PIN         54
#define X_DIR_PIN          55
#define X_ENABLE_PIN       38
#define X_MIN_PIN           3       //for future limit stop
// DEC motor/Alt motor
#define Y_STEP_PIN         60
#define Y_DIR_PIN          61
#define Y_ENABLE_PIN       56
#define Y_MIN_PIN          14       //for future limit stop

#define LED_PIN            13       //on board led


//Report of curent RA and DEC
double curr_RA_H, curr_RA_M, curr_RA_S, curr_DEC_D, curr_DEC_M, curr_DEC_S;
double Observation_Long;      //hour decimal
double Observation_Lat;       //Deg decimal
double delta_RA_algn;
double delta_DEC_algn;
double RA_D_const;
double DEC_D_const;
int cur_year;
int cur_month;
int cur_day;
int cur_hour;     //LMT Hour
int cur_minute;   //LMT minute
int cur_second;   //LMT seconds
int day_saving = 0;   //0- no day saving, 1-summer day saving
int SLEW_RA_microsteps;   // Where the mottors needs to go in order to point to the object
int SLEW_DEC_microsteps;
long RA_steps;            //RA steps position sent to the GOTO or track function
long DEC_steps;           //DEC steps position -"-
long cur_RA_steps;         //measurment of RA steps in GOTO or track function
long cur_DEC_steps;        //measurment of DEC steps in -"-
unsigned long MicroSteps_360;   //amount of maximum microstep in one RA/DEC revolution
int state = 1;     //1- Init  2-Alignment  3-GOTO   7- Fail
int Alignindex = 1;
int IndexStar;
String msg;
String LatLong_msg;
String DatenTime;
String StarName;
String answer;
bool Alignstar = false;
bool IS_MER_FLIP;
bool IS_TRACK;

//Variables definition



//***************************************************************************************************************
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  MicroSteps_360 = (unsigned long)Gear_ratio * Motor_steps * Microsteps;
  double RA_D_const = MicroSteps_360 / 360.0;   //how many microsteps in 1 deg
  double DEC_D_const = RA_D_const;
  int  Clock_Motor = int(1000 / (MicroSteps_360 / Sidereal_day)); //delay time for step motor while tracking in ms
}

void loop() {

//Objects RA and DEC
double OBJECT_RA_H;
double OBJECT_RA_M;
double OBJECT_RA_dec;
double OBJECT_DEC_D;
double OBJECT_DEC_M;
double OBJECT_DEC_dec;
bool IS_OBJ_VISIBLE; 
double HA_H_dec;

// Main state machine

switch (state){
  
  case  1:    //init state
    
        //Enter Lat/Long of observation site
        msg = "Enter Lat[dd:mm:ss] & Long[deg:mm:ss] location (space between Lat&Long):";
        Serial.println(msg);
        while (Serial.available()==0){
        }
        LatLong_msg = Serial.readString();
        //finding the index of the string
        int i1 = LatLong_msg.indexOf(":");
        int i2 = LatLong_msg.indexOf(":",i1+1);
        int i3 = LatLong_msg.indexOf(" ",i2+1);
        int i4 = LatLong_msg.indexOf(":",i3+1);
        int i5 = LatLong_msg.indexOf(":",i4+1);
        //breaking down the string
        int Lat_deg = LatLong_msg.substring(0,i1).toInt();
        int Lat_mm = LatLong_msg.substring(i1+1,i2).toInt();
        int Lat_ss = LatLong_msg.substring(i2+1,i3).toInt();
        Observation_Lat = Lat_deg+Lat_mm/60.0+Lat_ss/3600.0;
        int Long_deg = LatLong_msg.substring(i3+1,i4).toInt();
        int Long_mm = LatLong_msg.substring(i4+1,i5).toInt();
        int Long_ss = LatLong_msg.substring(i5+1,LatLong_msg.length()).toInt();
        //calculating the Observation long in decimal hours
        Observation_Long = (Long_deg+Long_mm/60.0+Long_ss/3600.0)/15.0;     //dividing by 15 to convert from deg to hour
        
        //******Debug*******//
        Serial.println();
        Serial.print("Observation Long: ");
        Serial.print(Observation_Long,5);
        Serial.println();
        Serial.print("Observation_Lat: ");
        Serial.print(String(Lat_deg)+":"+String(Lat_mm)+":"+String(Lat_ss));
        Serial.println();
        Serial.println(i3);      //******** I tested till this point
        
        //Enter Date and Local time
        msg = "Enter date [DD/MM/YYYY] & Time [HH:MM:SS]";
        Serial.println(msg);
        while (Serial.available()==0){
        }
        DatenTime = Serial.readString();
        //Finding the indices of ":"
        i1 = DatenTime.indexOf("/");
        i2 = DatenTime.indexOf("/",i1+1);
        i3 = DatenTime.indexOf(" ",i2+1);
        i4 = DatenTime.indexOf(":",i3+1);
        i5 = DatenTime.indexOf(":",i4+1);
        //breaking down the DatenTime string
        cur_day = DatenTime.substring(0,i1).toInt();
        cur_month = DatenTime.substring(i1+1,i2).toInt();
        cur_year = DatenTime.substring(i2+1,i3).toInt();
        cur_hour = DatenTime.substring(i3+1,i4).toInt();
        cur_minute = DatenTime.substring(i4+1,i5).toInt();
        cur_second = DatenTime.substring(i5+1,DatenTime.length()).toInt();
        setTime(cur_hour,cur_minute,cur_second,cur_day,cur_month,cur_year);

        //*******Debug*******//
        delay(1000);
        Serial.println(String(i1)+"\t"+String(i2)+"\t"+String(i3)+"\t"+String(i4)+"\t"+String(i5));
        Serial.println();
        Serial.print("The date today is: ");
        Serial.print(String(day())+"-"+String(month())+"-"+String(year()));
        Serial.println();
        Serial.print("The time now is: ");
        Serial.print(String(hour())+"-"+String(minute())+"-"+String(second()));
        Serial.println();
        
        //Enter do you use daylight saving Y/(N);
        msg = "Do you use daylight saving Y/[N] ?";
        Serial.println(msg);
        while (Serial.available()==0){
        } 
        answer = Serial.readString();
        Serial.println(answer);
        if (answer.substring(0,1).equalsIgnoreCase("Y")){
          day_saving = 1;
          Serial.println(day_saving);
        }
        
        
        //******Debug******//
        Serial.println();
        Serial.print(answer);
        Serial.print("Day saving: ");
        Serial.print(day_saving);
        Serial.println();
        
        if (DatenTime!=" " && LatLong_msg!=" "){
          state = 2;
        }
        Serial.println(state);
 
  case  2:    //Alignment state

        msg = "Star Alignment with [1]/2/3 stars, Enter number between 1-3";
        Serial.println(msg);
        while (Serial.available()==0){
        }
      Alignindex = Serial.readString().toInt();
      Serial.print(Alignindex);
      for (int ii=1; ii<=Alignindex; ii++){
          msg = "Enter index of a Star";
          Serial.println(msg);
          while (Serial.available()==0){
          }
          IndexStar = Serial.readString().toInt();
          i1 = Stars[IndexStar].indexOf(";");
          i2 = Stars[IndexStar].indexOf(";",i1+1);
          i3 = Stars[IndexStar].indexOf(";",i2+1);
          Alignstar = true;
          String OBJECT_NAME = Stars[IndexStar].substring(i1+1,i2)+" from "+Stars[IndexStar].substring(0,i1);
          String OBJ_RA = Stars[IndexStar].substring(i2+1,i3);
          OBJECT_RA_H = OBJ_RA.substring(0,OBJ_RA.indexOf('h')).toFloat();
          OBJECT_RA_M = OBJ_RA.substring(OBJ_RA.indexOf('h')+1,OBJ_RA.length()-1).toFloat();
          String OBJ_DEC = Stars[IndexStar].substring(i3,Stars[IndexStar].length());
          String sign = OBJ_DEC.substring(0, 1);
          OBJECT_DEC_D = OBJ_DEC.substring(1,OBJ_DEC.indexOf('°')).toDouble();
          if (sign == "-"){ 
             OBJECT_DEC_D *= (-1);
          }


          //******Debug*******//
          Serial.println();
          Serial.println(OBJECT_NAME+";"+String(OBJECT_RA_H)+":"+String(OBJECT_RA_M));
          Serial.println(OBJECT_DEC_D);
          //Serial.print("continue with alignment processes...");
          

          //1. calling LMST function to calculate the HA of the object
          //2. checking if object is above horizon
          //3. checking if object is near local meridian
          //4. enabaling the GOTO function
          //5. enable tracking automatically
          //6. asking the observer to bring the object to the center of the FOV
          //7. when in center calculating current HA and DEC of the object
          //8. calling again (1) to calculate the exact HA, and claculate the delta for HA_object-HA_cur; DEC_object-DEC_cur
          //9. returning back to main function
         
          doHAcalc(OBJECT_RA_H ,OBJECT_RA_M, OBJECT_DEC_D, HA_H_dec ,IS_OBJ_VISIBLE, IS_MER_FLIP);
          
          
          //**********************Debug**************************

          int H_OBJ = int(HA_H_dec);
          int M_OBJ = int((HA_H_dec-H_OBJ)*60);
          int S_OBJ = int((HA_H_dec-H_OBJ-float(M_OBJ/60.0))*3600);
          
          Serial.println();
          Serial.print(OBJECT_NAME+";"+"Object HA  "+String(H_OBJ)+":"+String(M_OBJ)+":"+String(S_OBJ));
          Serial.println("Object is above horizon ? "+String(IS_OBJ_VISIBLE));
          
          doGOTO(HA_H_dec, OBJECT_DEC_D,RA_steps, DEC_steps, IS_OBJ_VISIBLE, IS_OBJ_VISIBLE, cur_RA_steps, cur_DEC_steps); 
          
          
        }
  
  
  break;



  case  3:    //GOTO state

  break;



  case  7:    //Fail state

  break;

  default:

  break;

  }
}

//*******************************************************************************************************************
  
  void doHAcalc(double RA_H, double RA_M, double DEC_D, double &HA_dec , bool &IS_OBJ_VISIBLE, bool &IS_MER_FLIP) {

  //calculating the LMST
    const double J2000d = 2451543.5;
    const double MJ2000d = 2400000.5;
    const double MJdUTC = 51544.5;
    int H_LMT = hour();
    int M_LMT = minute();
    int S_LMT = second();
    int Y = year();
    int M = month();
    int D = day();
    double H_LMT_dec = H_LMT+float(M_LMT/60.0)+float(S_LMT/3600.0);
    double H_GMT_dec = H_LMT_dec-H_timezone;
    if (day_saving == 1){
      H_GMT_dec += -1;
    }
    if (H_GMT_dec<0){
      H_GMT_dec+=24;
    }

     if (M < 3) {
       M = M + 12; 
       Y = Y - 1;
    }
    
    // Jd =INT(365.25*(Year+4716))+INT(30.6001*(Mount+1))+Day-1537.5+H_GMT_dec/24
    //JD = AA+BB+float(D)-float(1537.50)+float(H_GMT_dec/24.0);
    // since floating point in Arduino has limitation of 7 digits including the numbers to the right of the decimal point
    // a calculation of integer numbers must be implemented
    // dividing the JD to MSB = all the digits to the left and LSB all the digits to the right 
    unsigned long AA = (unsigned long)(365.25*(Y+4716));
    unsigned long BB = (unsigned long)(30.6001*(M+1));
    unsigned long JD_MSB = AA+BB+D-1537;
    double JD_LSB = (-0.5+H_GMT_dec/24.0);
    //double MJD = JD - MJ2000d;
    unsigned long MJD_MSB = JD_MSB - (unsigned long)MJ2000d;
    double MJD_LSB = JD_LSB - (MJ2000d-(unsigned long)MJ2000d);
    //double d2000 = JD - J2000d;
    if (MJD_LSB<0){
      MJD_LSB+=1;
      MJD_MSB+=-1;
    }
    double ut = (MJD_LSB)*24.0;
    double tau = (MJD_MSB - MJdUTC)/36525.0;
    //
    
    // formula to calculate wrapped GMST: Modulu(6.697374558+1.0027379093*ut+(8640184.812866+(0.093104-0.0000062*tau)*tau)*tau/3600,24)
    double H_GMST_dec = (6.697374558+1.0027379093*ut+(8640184.812866+(0.093104-0.0000062*tau)*tau)*tau/3600.0);
    // reduce to 24h
    int H_GMST_int = (int)H_GMST_dec;
    H_GMST_int/=24;
    H_GMST_dec = H_GMST_dec - (double) H_GMST_int * 24.0;
    //Changing to Local time zone
    double H_LMST_dec = H_GMST_dec+Observation_Long;
    int H_LMST = int(H_LMST_dec);
    int M_LMST = int((H_LMST_dec-H_LMST)*60);
    int S_LMST = int((H_LMST_dec-H_LMST-float(M_LMST/60.0))*3600);
    Serial.println();
    Serial.print(String(H_LMST)+":"+String(M_LMST)+":"+String(S_LMST));
    
    //calculating HA of the object, it is the hour RA of an object relative to the local meridian
    double RA_dec = RA_H + RA_M/60.0;
    
    HA_dec = H_LMST_dec - RA_dec;   //hour of an object relative to the LMST 12:00hr is the Home position of the mount
    if (HA_dec < 0){
      HA_dec+=24.0;             //hour should be between 0-24hr     
    }
    
    // Object RA & DEC in radians

    double RA_R = (HA_dec*15.0)*(pi/180.0); //converting (Hour2Deg)*(Deg2rad)
    
    double DEC_R = DEC_D*(pi/180.0);        //converting (Deg2radians)

    double rLAT = Observation_Lat*(pi/180);               //converting local Latitude in Deg2rad

    double sin_rDEC = sin(DEC_R);
    double cos_rDEC = cos(DEC_R);
    double sin_rLAT = sin(rLAT);
    double cos_rLAT = cos(rLAT);
    double cos_rHA = cos(RA_R);
    double sin_rHA = sin(RA_R);
    
    double ALT = sin_rDEC * sin_rLAT;
    ALT += (cos_rDEC * cos_rLAT * cos_rHA);
    double sin_rALT = ALT;
    ALT =  asin(ALT);
    double cos_rALT = cos(ALT);
    ALT *= 57.2958;
    
    double AZ = sin_rALT * sin_rLAT;
    AZ = sin_rDEC - AZ;
    AZ /= (cos_rALT * cos_rLAT);
    AZ = acos(AZ)*57.2957795;
    if (sin_rHA > 0){
      AZ = 360 - AZ;
    }

    if (ALT < 0){
      IS_OBJ_VISIBLE = false;
      
//      IS_OBJ_FOUND = true;
//      IS_OBJECT_RA_FOUND = true;
//      IS_OBJECT_DEC_FOUND = true;
//      Slew_RA_timer = 0;
//      RA_finish_last = 0;
     }
     else{
      IS_OBJ_VISIBLE = true;
     }

     //check if mount position is near the meridian flip
     //check if an object is about to cross from the East side to the West side of the meridian
     IS_MER_FLIP = false;
     if ((24.0-HA_dec) <= min_to_meridian/60.0){
     IS_MER_FLIP = true;
     }
  }

  void doGOTO(double HA_dec, double DEC_D, long RA_steps, long DEC_steps, bool IS_OBJ_VISIBLE, bool IS_MER_FLIP, long &cur_RA_steps, long &cur_DEC_steps){;
  int direct_DEC;     //DEC motor direction
  int direct_RA;       //RA motor direction
  double RA_deg;
  
  //Check in which side of the mridian and in which quarter the object is
  //First cur the meridian to East & West
  //then deciding through which side to go based on movement limitation of EQ5

  if (HA_dec>=12.0 && HA_dec<24.0){
      if (HA_dec>=19 && DEC_D<=80){
          RA_deg = (-1.0)*(HA_dec - 12.0)*15.0;     //converting from hours to deg and multiplying in (-1) to move to the left 
          DEC_D*=-1;
          direct_DEC = 0;
          direct_RA = 0;
      }else{
          RA_deg = (HA_dec - 12.0)*15.0;
          direct_DEC = 1;
          direct_RA = 1;}
  }
    
  if (HA_dec>=0.0 && HA_dec<12.){
      if(HA_dec>=7.0 && DEC_D<=80){
          RA_deg = (HA_dec)*15;
          DEC_D*=-1;
          direct_DEC = 1;
          direct_RA = 1;
      }else{
          RA_deg = (-1)*(HA_dec)*15;
          direct_DEC = 0;
          direct_RA = 0;}
  }

  double cmd_RA_deg = RA_deg-RA_steps/RA_D_const+delta_RA_algn;
  double cmd_RA_steps = cmd_RA_deg*RA_D_const;
  
// DEC cmd is in steps otherwise can't escape of ambiguity
// the cmd in steps is fron the NCP 90-DEC_d multipy by steps/deg constant
// multiply by the sign, to define the quarter, if direct_DEC=0 means negative direction through Q4
// if direct_DEC=1 the outcome is +1 therfore positive direction through Q1
//********this formula needs to be test carefully********
  
  double cmd_DEC_steps = (90-DEC_D)*DEC_D_const*(2*direct_DEC-1)-DEC_steps+delta_DEC_algn*DEC_D_const;
} 

  
