#define BAUDRATE 9600

/*
 * PINS
 */

// Y-Axis - oś obrotowa
#define YAXIS_DIR_PIN 7 //Pin kierunku DIR
#define YAXIS_STEP_PIN 8 //Pin stepow STEP
#define YAXIS_ENABLE_PIN 9 //Pin wlaczenia ENABLE
#define YAXIS_RST_PIN -1 //Pin reset - niepodlaczony (-1 przy pinach oznacza, ze sa niepodlaczone)
#define YAXIS_SLP_PIN -1 //Pin sleep - niepodloczony -> sleep i reset zwarte razem
#define YAXIS_MS1_PIN 28 //pin MS1 do mikrokow
#define YAXIS_MS2_PIN 30 //pin MS2 do mikrokow
#define YAXIS_MS3_PIN 32 //pin MS3 do mikrokow
#define YAXIS_ENDSTOP_PIN 5 //pin podlaczenia endstopu
#define YAXIS_VMS1 HIGH //wartosci MS1, MS2, MS3 HIGH lub LOW -> patrz tabele do ustawienia mikrokow na dole pliku
#define YAXIS_VMS2 HIGH //jw
#define YAXIS_VMS3 HIGH //jw
#define YAXIS_MIN_STEPCOUNT 0    // minimalna liczba krokow, jesli 0 wtedy nie jest uwzgledniania
#define YAXIS_MAX_STEPCOUNT 0	//maksymalna liczba krokow jesli 0 wtedy nie jest uwzgledniania
#define YAXIS_STEPS_PER_FULL_ROTATION 200.0 // ilosc krokow na obrot silnika
#define YAXIS_MICROSTEPPING 16 // liczba mikrokrokow 1,2,4,8,16
#define YAXIS_GEAR_RATIO 1 // przelozenie mechaniczne
#define YAXIS_ENDSTOP_TYPE 1 // rozdzaj endstopu; 0-mechaniczny, 1-optyczny
#define YAXIS_MOVE_TYPE 1 //domyslnie ruch w 1 -mm, 2- rad, 3 -degree
#define YAXIS_DEFAULT_DIAMETER 100 //domyslna srednica rury w [mm], wplywa na przeliczanie stopni i radianow na [mm], by zachowac odpowiedni posuw, trzeba tak dobrac srednice, zeby 1 obrot silnika odpowiedal Pi*Srednica - bez uwzglednienia przelozenia mechaniczego

//X-Axis - oś wzdłużna, ustawienie analogiczne jak w YAxis
#define XAXIS_DIR_PIN 10
#define XAXIS_STEP_PIN 11
#define XAXIS_ENABLE_PIN 12
#define XAXIS_RST_PIN -1
#define XAXIS_SLP_PIN -1
#define XAXIS_MS1_PIN 22
#define XAXIS_MS2_PIN 24
#define XAXIS_MS3_PIN 26
#define XAXIS_ENDSTOP_PIN 6  
#define XAXIS_VMS1 HIGH
#define XAXIS_VMS2 HIGH
#define XAXIS_VMS3 HIGH
#define XAXIS_MIN_STEPCOUNT 0   
#define XAXIS_MAX_STEPCOUNT 0	 
#define XAXIS_STEPS_PER_FULL_ROTATION 200.0
#define XAXIS_MICROSTEPPING 16
#define XAXIS_GEAR_RATIO 1 // przelozenie mechaniczne
#define XAXIS_ENDSTOP_TYPE 0 //endstop mechaniczny
#define XAXIS_MOVE_TYPE 1 //domyslnie ruch w mm
#define XAXIS_DEFAULT_DIAMETER 100 //domyslna srednica rury w [mm], wplywa na przeliczanie stopni i radianow na [mm], by zachowac odpowiedni posuw, trzeba tak dobrac srednice, zeby 1 obrot silnika odpowiedal Pi*Srednica - bez uwzglednienia przelozenia mechaniczego



//Ustawienie serwa
#define SERVO_PIN_1 13 //pin serwa
#define DEFAULT_PEN_UP_POSITION 0 //domyslne ustawienie  kata serwa [deg]


//Ustawienie LCD
#define LCD_RS_PIN 48
#define LCD_ENABLE_PIN 46
#define LCD_D4_PIN 44
#define LCD_D5_PIN 42
#define LCD_D6_PIN 40
#define LCD_D7_PIN 38

//Ustawienie czytnika SD
#define SD_CS_PIN 53

//Ustawienia klawiatury

const byte numRows= 4;
const byte numCols= 4;

char keymap[numRows][numCols]= // Mapa klawiatury
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

byte rowPins[numRows] = {35,37,39,41}; //Piny dla wierszy od 35 do 41 
byte colPins[numCols]= {43,45,47,49}; //Piny dla kolumn od 43 do 49 




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


//obecnie nieuzywane zoom-y
#define DEFAULT_ZOOM_FACTOR 1 //0.1808 // With a Zoom-Faktor of .65, I can print gcode for Makerbot Unicorn without changes. 
                              // The zoom factor can be also manipulated by the propretiary code M402
#define X_SCALING_FACTOR     1 //1.65/2    //this factor is for correction to meet the unicorn coordinates 
#define Y_SCALING_FACTOR    1

