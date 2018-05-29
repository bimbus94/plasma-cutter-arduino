#define BAUDRATE 9600
//#define BAUDRATE 57600
//#define BAUDRATE 115200
//#define BAUDRATE 256000

/*
 * PINS
 */



// Y-Axis - oś obrotowa
#define YAXIS_DIR_PIN 4
#define YAXIS_STEP_PIN 3
#define YAXIS_ENABLE_PIN 2
#define YAXIS_RST_PIN -1
#define YAXIS_SLP_PIN -1
#define YAXIS_MS1_PIN -1
#define YAXIS_MS2_PIN -1
#define YAXIS_MS3_PIN -1
#define YAXIS_ENDSTOP_PIN 9     // -1 -> No Endstop
#define YAXIS_VMS1 HIGH
#define YAXIS_VMS2 HIGH
#define YAXIS_VMS3 HIGH
#define YAXIS_MIN_STEPCOUNT 0    // minimalna liczba krokow
#define YAXIS_MAX_STEPCOUNT 0	//maksymalna liczba krokow
#define YAXIS_STEPS_PER_FULL_ROTATION 200.0 // ilosc krokow na obrot silnika
#define YAXIS_MICROSTEPPING 1 // liczba mikrokrokow 1,2,4,8,16
#define YAXIS_GEAR_RATIO 1 // przelozenie mechaniczne
#define YAXIS_ENDSTOP_TYPE 1 // rozdzaj endstopu; 0-mechaniczny, 1-optyczny

//X-Axis - oś wzdłużna
#define XAXIS_DIR_PIN 7
#define XAXIS_STEP_PIN 6
#define XAXIS_ENABLE_PIN 5
#define XAXIS_RST_PIN -1
#define XAXIS_SLP_PIN -1
#define XAXIS_MS1_PIN -1
#define XAXIS_MS2_PIN -1
#define XAXIS_MS3_PIN -1
#define XAXIS_ENDSTOP_PIN 8     // -1 -> No Endstop
#define XAXIS_VMS1 HIGH
#define XAXIS_VMS2 HIGH
#define XAXIS_VMS3 HIGH
#define XAXIS_MIN_STEPCOUNT 0   
#define XAXIS_MAX_STEPCOUNT 0	 
#define XAXIS_STEPS_PER_FULL_ROTATION 200.0
#define XAXIS_MICROSTEPPING 1
#define XAXIS_GEAR_RATIO 1 // przelozenie mechaniczne
#define XAXIS_ENDSTOP_TYPE 0 //endstop mechaniczny

#define SERVO_PIN_1 13

/*
 * Other Configuration
 */
#define DEFAULT_DIAMETER 100 //domyslna srednica rury
#define DEFAULT_PEN_UP_POSITION 45 //domyslne ustawienie serwa - wlacznika rury

//obecnie nieuzywane zoom-y
#define DEFAULT_ZOOM_FACTOR 1 //0.1808 // With a Zoom-Faktor of .65, I can print gcode for Makerbot Unicorn without changes. 
                               // The zoom factor can be also manipulated by the propretiary code M402
#define X_SCALING_FACTOR     1 //1.65/2    //this factor is for correction to meet the unicorn coordinates 
#define Y_SCALING_FACTOR    1
/*
 * Microstepping Information
 */     
                               
//MS1, MS2 and MS3 are optional. You can simply make these settings by hardwiring the pins to high or low
 
   /* MS1  |  MS2  |  MS3      Microstepping Resolution
    -----------------------
      L    |  L    |  L     ->  Full Step
      H    |  L    |  L     ->  Half Step 
      L    |  H    |  L     ->  Quarter Step
      H    |  H    |  L     ->  Eighth Step
      H    |  H    |  H     ->  Sixteenth Step
  */

