#include <Keypad.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(32,30,22,24,26,28);

const byte numRows= 4;
const byte numCols= 4;

char keymap[numRows][numCols]=
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};


byte rowPins[numRows] = {35,37,39,41};
byte colPins[numCols]= {43,45,47,49};
Keypad myKeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);



#include <SD.h>
#include <SPI.h>
File myFile;
int pinCS = 53;

#include "TimerOne.h"
#include "SoftwareServo.h"
#include "StepperModel.h"
#include "config.h"


#define TIMER_DELAY 1024

#define VERSIONCODE "PlasmaCutter v0.1"

StepperModel xAxisStepper(
    XAXIS_DIR_PIN, XAXIS_STEP_PIN, XAXIS_ENABLE_PIN, XAXIS_ENDSTOP_PIN,
    XAXIS_MS1_PIN, XAXIS_MS2_PIN, XAXIS_MS3_PIN,
    XAXIS_SLP_PIN, XAXIS_RST_PIN,
    XAXIS_VMS1, XAXIS_VMS2, XAXIS_VMS3,
    XAXIS_MIN_STEPCOUNT, XAXIS_MAX_STEPCOUNT,
    XAXIS_STEPS_PER_FULL_ROTATION, XAXIS_MICROSTEPPING, XAXIS_GEAR_RATIO, DEFAULT_DIAMETER);

StepperModel rotationStepper(
    YAXIS_DIR_PIN, YAXIS_STEP_PIN, YAXIS_ENABLE_PIN, YAXIS_ENDSTOP_PIN,
    YAXIS_MS1_PIN, YAXIS_MS2_PIN, YAXIS_MS3_PIN,
    YAXIS_SLP_PIN, YAXIS_RST_PIN,
    YAXIS_VMS1, YAXIS_VMS2, YAXIS_VMS3,
    YAXIS_MIN_STEPCOUNT, YAXIS_MIN_STEPCOUNT,
    YAXIS_STEPS_PER_FULL_ROTATION, YAXIS_MICROSTEPPING, YAXIS_GEAR_RATIO, DEFAULT_DIAMETER);

SoftwareServo servo;
boolean servoEnabled=true;

long intervals=0;
volatile long intervals_remaining=0;
volatile boolean isRunning=false;

// comm variables
const int MAX_CMD_SIZE = 64;
char buffer[MAX_CMD_SIZE]; // buffer for serial commands
char fileNameChar[MAX_CMD_SIZE];
char* fileName;
char entryStr[MAX_CMD_SIZE];

char serial_char; // value for each byte read in from serial comms
int serial_count = 0; // current length of command
char *strchr_pointer; // just a pointer to find chars in the cmd string like X, Y, Z, E, etc
boolean comment_mode = false;

int next_command_request_counter = 0;	//if this counter reaches the maximum then a "ok" is sent to request the nex command
int next_command_request_maximum = 1000;
// end comm variables

// GCode States
double currentOffsetX = 0.;
double currentOffsetY = 0.;
boolean absoluteMode = true;
double feedrate = 1000.; // mm/minute

//ZOOMY AKTUALNIE NIEUZYWANE 
double zoom = DEFAULT_ZOOM_FACTOR;
double xscaling = X_SCALING_FACTOR;
double yscaling = Y_SCALING_FACTOR;

const double maxFeedrate = 2000.;
const float M_Pi = 3.141593;
// ------

void setup()
{

//DODANE SETUP
    lcd.begin(16,2); // deklaracja typu



    Serial.begin(BAUDRATE);
    Serial.print(VERSIONCODE);
    Serial.print("\n");

    clear_buffer();

    servo.attach(SERVO_PIN_1);
    servo.write(DEFAULT_PEN_UP_POSITION);

    if(servoEnabled)
    {
        for(int i=0; i<100; i++)
        {
            SoftwareServo::refresh();
            delay(4);
        }
    }

    //--- Activate the PWM timer
    Timer1.initialize(TIMER_DELAY); // Timer for updating pwm pins
    Timer1.attachInterrupt(doInterrupt);

#ifdef AUTO_HOMING
    xAxisStepper.autoHoming();
    xAxisStepper.setTargetPosition(0.);
    commitSteppers(maxFeedrate);
    delay(2000);
    xAxisStepper.enableStepper(false);
#endif
    checkSD();
    lcd.clear();
    startMenu();
    myFile = SD.open(fileName,FILE_READ);
}

void loop() // input loop, looks for manual input and then checks to see if and serial commands are coming in
{

    get_command(); // check for Gcodes
    if(servoEnabled)
        SoftwareServo::refresh();
    Serial.flush();
}

//--- Interrupt-Routine: Move the steppers
void doInterrupt()
{
    if(isRunning)
    {
        if(next_command_request_counter++ > next_command_request_maximum)
        {
            //Serial.print("forced ok\n");
            next_command_request_counter = 0;
        }
        if (intervals_remaining-- == 0)
            isRunning = false;
        else
        {
            rotationStepper.doStep(intervals);
            xAxisStepper.doStep(intervals);
        }
    }
}


//SPRAWDZIC CZY BEDZIE DZIALAC Z STOPNIAMI I RADIANIAMI
void commitSteppers(double speedrate)
{
    long deltaStepsX = xAxisStepper.delta;
    if(deltaStepsX != 0L)
    {
        xAxisStepper.enableStepper(true);
    }

    long deltaStepsY = rotationStepper.delta;
    if(deltaStepsY != 0L)
    {
        rotationStepper.enableStepper(true);
    }
    long masterSteps = (deltaStepsX>deltaStepsY)?deltaStepsX:deltaStepsY;

    double deltaDistanceX = xAxisStepper.targetPosition-xAxisStepper.getCurrentPosition();
    double deltaDistanceY = rotationStepper.targetPosition-rotationStepper.getCurrentPosition();

    // how long is our line length?
    double distance = sqrt(deltaDistanceX*deltaDistanceX+deltaDistanceY*deltaDistanceY);

    // compute number of intervals for this move
    double sub1 = (60000.* distance / speedrate);
    double sub2 = sub1 * 1000.;
    intervals = (long)sub2/TIMER_DELAY;

    intervals_remaining = intervals;
    const long negative_half_interval = -intervals / 2;

    rotationStepper.counter = negative_half_interval;
    xAxisStepper.counter = negative_half_interval;


    isRunning=true;
}

void checkSD()
{


    while(!SD.begin(53))
    {
        lcd.clear();
        lcd.print("Wloz karte SD...\n");
        delay(1000);
    }
    lcd.setCursor(0,1); // ustawienie kursora
    lcd.print("Karta SD OK...\n");
    delay(1000);
    lcd.clear();
}

void startMenu()
{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Wybierz tryb");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("1 -tryb odczytu z SD");
    lcd.setCursor(0,1);
    lcd.print("2 -tryb wprowadzania");
    int inputType=0;

    int k=0;
    while( k != -1  )
    {
        char key = myKeypad.getKey();
        if (key != NO_KEY)
        {
            if ( key == '1' )
            {
                k=-1;
                lcd.clear();
                inputType=1;
                break;

            }
            else if (key == '2')
            {
                k=-1;
                lcd.clear();
                inputType=2;
                break;
            }
        }

    }

    if(inputType == 1)
    {
        getFileName();
    }

    else if( inputType == 2)
    {
        CamModule();
    }



}


void CamModule()
{
    double diameter;
    double start_pointX;
    double cut_angle;
    double cut_radius;
    double start_pointY;

    lcd.setCursor(0,0);
    lcd.print("Podaj srednice:");
    lcd.setCursor(0,1);

    char* diameter_temp;
    char* start_pointX_temp;
    char* start_pointY_temp;
    char* cut_angle_temp;
    char* cut_radius_temp;

    int i=0;
    int k=0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podaj srednice:");
    lcd.setCursor(0,1);
    while(k !=-1)
    {
        char key = myKeypad.getKey();

        if (key != NO_KEY)
        {
            if (key == 42)
            {

                i=0;
                key=0;
                entryStr[i]=key;
                lcd.println("");
                lcd.clear();
                lcd.println("Canceled");
                delay(500);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Podaj srednice: \n");
                lcd.setCursor(0,1);


            }

            else if (key != 35)
            {


                entryStr[i]= key;
                i++;
                lcd.print(key);
            }
            else
            {

                key=0;
                entryStr[i]=key;
                i=0;


                k=-1;
                break;
            }
        }
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podana srednica:");
    lcd.setCursor(0,1);
    diameter_temp = entryStr;
    diameter=atof(diameter_temp);
    lcd.println(diameter_temp);
    delay(2000);






    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podaj startowy X:");
    lcd.setCursor(0,1);
    k=0;
    while(k !=-1)
    {
        char key = myKeypad.getKey();

        if (key != NO_KEY)
        {
            if (key == 42)
            {

                i=0;
                key=0;
                entryStr[i]=key;
                lcd.println("");
                lcd.clear();
                lcd.println("Canceled");
                delay(500);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Podaj startowy X: \n");
                lcd.setCursor(0,1);


            }

            else if (key != 35)
            {


                entryStr[i]= key;
                i++;
                lcd.print(key);
            }
            else
            {

                key=0;
                entryStr[i]=key;
                i=0;


                k=-1;
                break;
            }
        }
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podany X:");
    lcd.setCursor(0,1);
    start_pointX_temp = entryStr;

    start_pointX=atof(start_pointX_temp);
    lcd.println(start_pointX_temp);
    delay(2000);




    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podaj startowy Y:");
    lcd.setCursor(0,1);
    k=0;
    while(k !=-1)
    {
        char key = myKeypad.getKey();

        if (key != NO_KEY)
        {
            if (key == 42)
            {

                i=0;
                key=0;
                entryStr[i]=key;
                lcd.println("");
                lcd.clear();
                lcd.println("Canceled");
                delay(500);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Podaj startowy Y: \n");
                lcd.setCursor(0,1);


            }

            else if (key != 35)
            {


                entryStr[i]= key;
                i++;
                lcd.print(key);
            }
            else
            {

                key=0;
                entryStr[i]=key;
                i=0;


                k=-1;
                break;
            }
        }
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podany Y:");
    lcd.setCursor(0,1);
    start_pointY_temp = entryStr;

    start_pointY=atof(start_pointY_temp);
    lcd.println(start_pointY_temp);
    delay(2000);



    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podaj kat ciecia:");
    lcd.setCursor(0,1);
    k=0;
    while(k !=-1)
    {
        char key = myKeypad.getKey();

        if (key != NO_KEY)
        {
            if (key == 42)
            {

                i=0;
                key=0;
                entryStr[i]=key;
                lcd.println("");
                lcd.clear();
                lcd.println("Canceled");
                delay(500);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Podaj kat cieca: \n");
                lcd.setCursor(0,1);


            }

            else if (key != 35)
            {


                entryStr[i]= key;
                i++;
                lcd.print(key);
            }
            else
            {

                key=0;
                entryStr[i]=key;
                i=0;


                k=-1;
                break;
            }
        }
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podany kat ciecia 0-90:");
    lcd.setCursor(0,1);
    cut_angle_temp = entryStr;

    lcd.println(cut_angle_temp);
    cut_angle=atof(cut_angle_temp);
    cut_angle=cut_angle/180.0*M_PI;
    Serial.print(cut_angle);
    delay(2000);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podaj promien:");
    lcd.setCursor(0,1);
    k=0;
    while(k !=-1)
    {
        char key = myKeypad.getKey();

        if (key != NO_KEY)
        {
            if (key == 42)
            {

                i=0;
                key=0;
                entryStr[i]=key;
                lcd.println("");
                lcd.clear();
                lcd.println("Canceled");
                delay(500);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Podaj promien: \n");
                lcd.setCursor(0,1);


            }

            else if (key != 35)
            {


                entryStr[i]= key;
                i++;
                lcd.print(key);
            }
            else
            {

                key=0;
                entryStr[i]=key;
                i=0;


                k=-1;
                break;
            }
        }
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Podaj promien:");
    lcd.setCursor(0,1);
    cut_radius_temp = entryStr;

    cut_radius = atof( cut_radius_temp);
    lcd.println(cut_radius_temp);
    Serial.println(cut_radius);
    delay(2000);

    double  tempX1;
    double  tempX2;
    double  tempY1;
    double  tempY2;
    double  tempI1;
    double tempI2;
    double tempJ1;
    double tempJ2;

    double  tempX3;
    double tempY3;



    SD.remove("cam_mod");
    myFile = SD.open("cam_mod", FILE_WRITE);
    myFile.println("G90");
    myFile.println("G20");

    myFile.print("M401S");
    myFile.println(diameter);
    myFile.print("G0X");
    myFile.print(start_pointX);
    myFile.print("Y");
    myFile.println(start_pointY);
    myFile.println("M300S90");


double d=0; // przesuniecie osi miedzy otworem a cylindrem
if( cut_radius != 0)
{		
		for(double alpha_y=0;alpha_y<2*M_PI;alpha_y=alpha_y+2*M_PI/40){
		tempX1=start_pointX-sqrt(cut_radius*cut_radius-(d-diameter/2*sin(alpha_y))*(d-diameter/2*sin(alpha_y)));
		tempY1=start_pointY+alpha_y*diameter/2;
		myFile.print("G1X");
        myFile.print(tempX1);
        myFile.print("Y");
        myFile.println(tempY1);
		}
		
}




    
    else if(cut_radius == 0)
    {

	   for(double alpha_y=0;alpha_y<2*M_PI;alpha_y=alpha_y+2*M_PI/20){
	   tempX3=start_pointX+diameter/2*tan(cut_angle)*sin(alpha_y);
	   tempY3=start_pointY+alpha_y*diameter/2;
        myFile.print("G1X");
        myFile.print(tempX3);
        myFile.print("Y");
        myFile.println(tempY3);
	   }



    }

    myFile.println("M300S0");
    myFile.println("G0X0Y0");
    myFile.println("M18");
    myFile.close();
    fileName="cam_mod";


}






void getFileName()
{

    int i=0;
    int k=1;
    lcd.setCursor(0,0);
    lcd.print("Podaj nazwe:");
    lcd.setCursor(0,1);

    while(k !=-1)
    {
        char key = myKeypad.getKey();

        if (key != NO_KEY)
        {
            if (key == 42)
            {

                i=0;
                key=0;
                entryStr[i]=key;
                lcd.println("");
                lcd.clear();
                lcd.println("Canceled");
                delay(500);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Podaj nazwe: \n");
                lcd.setCursor(0,1);


            }

            else if (key != 35)
            {


                entryStr[i]= key;
                i++;
                lcd.print(key);
            }
            else
            {

                key=0;
                entryStr[i]=key;
                i=0;


                k=-1;
                break;
            }
        }
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Wpisany program:");
    lcd.setCursor(0,1);
    fileName = entryStr;


    lcd.println(fileName);
    delay(2000);




}


void get_command()
{







    while(myFile.available())
    {
        if (!isRunning && myFile.available() > 0) // each time we see something
        {
           
            serial_char = myFile.read(); // read individual byte from serial connection
            Serial.print(serial_char);
            lcd.clear();
            lcd.print(serial_char);
            if (serial_char == '\n' || serial_char == '\r') // end of a command character
            {
                next_command_request_counter = 0;
                buffer[serial_count]=0;
                process_commands(buffer, serial_count);
                clear_buffer();
                comment_mode = false; // reset comment mode before each new command is processed
                //Serial.write("process: command");
            }
            else // not end of command
            {
                if (serial_char == ';' || serial_char == '(') // semicolon signifies start of comment
                {
                    comment_mode = true;
                }

                if (comment_mode != true) // ignore if a comment has started
                {
                    buffer[serial_count] = serial_char; // add byte to buffer string
                    serial_count++;
                    if (serial_count > MAX_CMD_SIZE) // overflow, dump and restart
                    {
                        clear_buffer();
                        Serial.flush();
                        Serial.print("Overflow Error\n");
                    }
                }
                else
                {

                }
            }
        }
    }
//myFile.close();
}

void clear_buffer() // empties command buffer from serial connection
{
    serial_count = 0; // reset buffer placement
}

boolean getValue(char key, char command[], double* value)
{
    // find key parameter
    strchr_pointer = strchr(buffer, key);
    if (strchr_pointer != NULL) // We found a key value
    {
        *value = (double)strtod(&command[strchr_pointer - command + 1], NULL);
        return true;
    }
    return false;
}

void check_for_version_controll(char command)
{
    if(command == 'V')
    {
        Serial.print(VERSIONCODE);
        Serial.print("\n");
    }
}


void process_commands(char command[], int command_length) // deals with standardized input from serial connection
{

    double nVal;
    boolean hasNVal = getValue('N', command, &nVal);
    //if(hasNVal) {Serial.println("linenumber detected");};

    double getcs;
    boolean hasCS = getValue('*', command, &getcs);
    //if(hasCS) {Serial.println("checksum detected");};

    // checksum code from reprap wiki
    int cs = 0;
    int j = 0;
    for(j = 0; command[j] != '*' && command[j] != NULL; j++)
        cs = cs ^ command[j];
    cs &= 0xff;  // Defensive programming...

    if(!(cs == (int)getcs || hasCS == false)) // if checksum does not match
    {
        Serial.print("rs ");
        Serial.print((int)getcs);
        //Serial.print((int)nVal);
        Serial.print("\n");
        //Serial.flush();
    }
    else	//continue if checksum matches or none detected
    {
        //Serial.println("checksum match ");
        j=0;
        while (j < MAX_CMD_SIZE )
        {
            if ((command[j] == 'G') || command[j] == 'M')
            {
                break;
            }
            j++;
        }

        if(command_length == 1)
        {
            check_for_version_controll(command[0]);
        }
        if (command_length>0 && command[j] == 'G') // G code
        {
            //Serial.print("process G: \n");
            int codenum = (int)strtod(&command[j+1], NULL);

            double tempX = xAxisStepper.getCurrentPosition();
            double tempY = rotationStepper.getCurrentPosition();

            double xVal;
            boolean hasXVal = getValue('X', command, &xVal);
            if(hasXVal) xVal*=zoom*xscaling;
            double yVal;
            boolean hasYVal = getValue('Y', command, &yVal);
            if(hasYVal) yVal*=zoom;//*yscaling;
            double iVal;
            boolean hasIVal = getValue('I', command, &iVal);
            if(hasIVal) iVal*=zoom;
            double jVal;
            boolean hasJVal = getValue('J', command, &jVal);
            if(hasJVal) jVal*=zoom;
            double rVal;
            boolean hasRVal = getValue('R', command, &rVal);
            if(hasRVal) rVal*=zoom;
            double pVal;
            boolean hasPVal = getValue('P', command, &pVal);

            getValue('F', command, &feedrate);

            xVal+=currentOffsetX;
            yVal+=currentOffsetY;

            if(absoluteMode)
            {
                if(hasXVal)
                    tempX=xVal;
                if(hasYVal)
                    tempY=yVal;
            }
            else
            {
                if(hasXVal)
                    tempX+=xVal;
                if(hasYVal)
                    tempY+=yVal;
            }

            switch(codenum)
            {
            case 0: // G0, Rapid positioning
                xAxisStepper.setTargetPosition(tempX);
                rotationStepper.setTargetPosition(tempY);
                commitSteppers(maxFeedrate);
                break;
            case 1: // G1, linear interpolation at specified speed
                xAxisStepper.setTargetPosition(tempX);
                rotationStepper.setTargetPosition(tempY);
                commitSteppers(feedrate);
                break;
            case 2: // G2, Clockwise arc
                if(hasIVal && hasJVal)
                {
                    double centerX=xAxisStepper.getCurrentPosition()+iVal;
                    double centerY=rotationStepper.getCurrentPosition()+jVal;
                    drawArc(centerX, centerY, tempX, tempY, (codenum==2));
                }
                else if(hasRVal)
                {
                    //drawRadius(tempX, tempY, rVal, (codenum==2));
                }
            case 3: // G3, Counterclockwise arc
                if(hasIVal && hasJVal)
                {
                    double centerX=xAxisStepper.getCurrentPosition()+iVal;
                    double centerY=rotationStepper.getCurrentPosition()+jVal;
                    drawArc(centerX, centerY, tempX, tempY, (codenum==2));
                }
                else if(hasRVal)
                {
                    //drawRadius(tempX, tempY, rVal, (codenum==2));
                }
                break;
            case 4: // G4, Delay P ms
                if(hasPVal)
                {
                    unsigned long endDelay = millis()+ (unsigned long)pVal;
                    while(millis()<endDelay)
                    {
                        delay(1);
                        if(servoEnabled)
                            SoftwareServo::refresh();
                    }
                }
                break;
                
            case 20: // G20, Y in mm
                rotationStepper.resetSteppersForMoveType(1);
                break;
            case 21: // G21, Y in rad
            	rotationStepper.resetSteppersForMoveType(2);
                break;
            case 22: // G22, Y in deg
            	rotationStepper.resetSteppersForMoveType(3);
                break;
            case 90: // G90, Absolute Positioning
                absoluteMode = true;
                break;
            case 91: // G91, Incremental Positioning
                absoluteMode = false;
                break;
            case 92: // G92 homing
                xAxisStepper.autoHoming();
                rotationStepper.autoHoming();
                break;
            }
        }
        else if (command_length>0 && command[j] == 'M') // M code
        {
            //Serial.print("proces M:\n");
            double value;
            int codenum = (int)strtod(&command[j+1], NULL);
            switch(codenum)
            {
            case 18: // Disable Drives
                xAxisStepper.resetStepper();
                rotationStepper.resetStepper();
                break;

            case 300: // Servo Position
                if(getValue('S', command, &value))
                {
                    servoEnabled=true;
                    if(value<0.)
                        value=0.;
                    else if(value>180.)
                    {
                        value=DEFAULT_PEN_UP_POSITION;
                        servo.write((int)value);
                        for(int i=0; i<100; i++)
                        {
                            SoftwareServo::refresh();
                            delay(2);
                        }
                        servoEnabled=false;
                    }
                    servo.write((int)value);
                }
                break;

            case 400: // Propretary: Reset X-Axis-Stepper settings to new object diameter
                if(getValue('S', command, &value))
                {
                    xAxisStepper.resetSteppersForObjectDiameter(value);
                    xAxisStepper.setTargetPosition(0.);
                    commitSteppers(maxFeedrate);
                    delay(2000);
                    xAxisStepper.enableStepper(false);
                }
                break;

            case 401: // Propretary: Reset Y-Axis-Stepper settings to new object diameter
                if(getValue('S', command, &value))
                {
                    rotationStepper.resetSteppersForObjectDiameter(value);
                    rotationStepper.setTargetPosition(0.);
                    commitSteppers(maxFeedrate);
                    delay(2000);
                    rotationStepper.enableStepper(false);
                }
                break;

            //DO USUNIECIA
            case 402: // Propretary: Reset Y-Axis-Stepper settings to new object diameter
                if(getValue('S', command, &value))
                {
                    zoom = value/100;
                }
                break;
            default:
                break;

            }
        }

        //done processing commands
        //if (Serial.available() <= 0) {
        Serial.print("ok ");
        //Serial.print((int)getcs);
        Serial.println(command);
        Serial.print("\n");
        //Serial.flush();
        //}

    }
}

/* This code was ported from the Makerbot/ReplicatorG java sources */
void drawArc(double centerX, double centerY, double endpointX, double endpointY, boolean clockwise)
{
    // angle variables.
    double angleA;
    double angleB;
    double angle;
    double radius;
    double length;

    // delta variables.
    double aX;
    double aY;
    double bX;
    double bY;

    // figure out our deltas
    double currentX = xAxisStepper.getCurrentPosition();
    double currentY = rotationStepper.getCurrentPosition();
    aX = currentX - centerX;
    aY = currentY - centerY;
    bX = endpointX - centerX;
    bY = endpointY - centerY;

    // Clockwise
    if (clockwise)
    {
        angleA = atan2(bY, bX);
        angleB = atan2(aY, aX);
    }
    // Counterclockwise
    else
    {
        angleA = atan2(aY, aX);
        angleB = atan2(bY, bX);
    }

    // Make sure angleB is always greater than angleA
    // and if not add 2PI so that it is (this also takes
    // care of the special case of angleA == angleB,
    // ie we want a complete circle)
    if (angleB <= angleA)
        angleB += 2. * M_PI;
    angle = angleB - angleA;

    // calculate a couple useful things.
    radius = sqrt(aX * aX + aY * aY);
    length = radius * angle;

    // for doing the actual move.
    int steps;
    int s;
    int step;

    // Maximum of either 2.4 times the angle in radians
    // or the length of the curve divided by the curve section constant
    steps = (int)ceil(max(angle * 2.4, length));

    // this is the real draw action.
    double newPointX = 0.;
    double newPointY = 0.;

    for (s = 1; s <= steps; s++)
    {
        // Forwards for CCW, backwards for CW
        if (!clockwise)
            step = s;
        else
            step = steps - s;

        // calculate our waypoint.
        newPointX = centerX + radius * cos(angleA + angle * ((double) step / steps));
        newPointY= centerY + radius	* sin(angleA + angle * ((double) step / steps));

        // start the move
        xAxisStepper.setTargetPosition(newPointX);
        rotationStepper.setTargetPosition(newPointY);
        commitSteppers(feedrate);

        while(isRunning)
        {
            delay(1);
            if(servoEnabled)
                SoftwareServo::refresh();
        };
    }
}



