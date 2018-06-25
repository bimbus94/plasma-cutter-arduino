//Zbior bibliotek potrzebnych do dzialania programu
#include "TimerOne.h"
#include "SoftwareServo.h"
#include "StepperModel.h"
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <SD.h>
#include <SPI.h>

//Wczytanie pliku z ustawieniami, pinami etc.
#include "config.h"

#define TIMER_DELAY 1024

#define VERSIONCODE "PlasmaCutter v0.1"


//Utworzenie obiektu i podlaczenie wyswietlacza (ustawienia w config.h)
LiquidCrystal lcd(LCD_RS_PIN,LCD_ENABLE_PIN,LCD_D4_PIN,LCD_D5_PIN,LCD_D6_PIN,LCD_D7_PIN);

//Utworzenie obiektu i podlaczenie klawiatury (ustawienia w config.h)
Keypad myKeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);

//Utworzenie obiektu osi X - postepowej (ustawienia w config.h)
StepperModel xAxisStepper(
    XAXIS_DIR_PIN, XAXIS_STEP_PIN, XAXIS_ENABLE_PIN, XAXIS_ENDSTOP_PIN,
    XAXIS_MS1_PIN, XAXIS_MS2_PIN, XAXIS_MS3_PIN,
    XAXIS_SLP_PIN, XAXIS_RST_PIN,
    XAXIS_VMS1, XAXIS_VMS2, XAXIS_VMS3,
    XAXIS_MIN_STEPCOUNT, XAXIS_MAX_STEPCOUNT,
    XAXIS_STEPS_PER_FULL_ROTATION, XAXIS_MICROSTEPPING, XAXIS_GEAR_RATIO, XAXIS_DEFAULT_DIAMETER,
    XAXIS_ENDSTOP_TYPE,
    XAXIS_MOVE_TYPE);

//Utworzenie obiektu osi Y - obrotowej (ustawienia w config.h)
StepperModel rotationStepper(
    YAXIS_DIR_PIN, YAXIS_STEP_PIN, YAXIS_ENABLE_PIN, YAXIS_ENDSTOP_PIN,
    YAXIS_MS1_PIN, YAXIS_MS2_PIN, YAXIS_MS3_PIN,
    YAXIS_SLP_PIN, YAXIS_RST_PIN,
    YAXIS_VMS1, YAXIS_VMS2, YAXIS_VMS3,
    YAXIS_MIN_STEPCOUNT, YAXIS_MIN_STEPCOUNT,
    YAXIS_STEPS_PER_FULL_ROTATION, YAXIS_MICROSTEPPING, YAXIS_GEAR_RATIO, YAXIS_DEFAULT_DIAMETER,
    YAXIS_ENDSTOP_TYPE,
    YAXIS_MOVE_TYPE);

//Utworzenie obiektu serva
SoftwareServo servo;
boolean servoEnabled=true;

//Zmienne do obliczenia przerw w dzialaniu silnika, by zachowac odpowiednia szybkosc posuwu
long intervals=0;
volatile long intervals_remaining=0;
volatile boolean isRunning=false;

// Zmienne sluzace do obslugi pliku z Gcode (zapis/odczyt), czytanie i przechowywanie komend Gcode
File myFile; // deklaracja zmiennej sluzacej do przechowywania/odczytu i zapisu pliku z Gcodem z karty SD
const int MAX_CMD_SIZE = 64; //max dlugosc komendy
char buffer[MAX_CMD_SIZE]; // bufor dla czytanych komend 
char fileNameChar[MAX_CMD_SIZE]; // zmienna do przechowywania nazwy pliku
char* fileName; //wskaznik do przekazywania nazwy pilku?
char entryStr[MAX_CMD_SIZE];

char serial_char; // kazdy pojedynczy czytany znak
int serial_count = 0; // dlugosc komedny
char *strchr_pointer; // wskaznik do znajdowania komend Gcode typu X, Y, Z, E, etc
boolean comment_mode = false; //tryb komentarza

int next_command_request_counter = 0;	//jesli licznik osiagnie max to wysyla "ok", by czytac kolejna komende
int next_command_request_maximum = 1000;


// Wybrane ustawienie GCode
double currentOffsetX = 0.; // offset poczatkowy X
double currentOffsetY = 0.; // offset poczatkowy Y
boolean absoluteMode = true; //domyslnie tryb absolutny
double feedrate = 1000.; // szybkosc posuwu [mm/minute]

//Zoomy - przemnazaja czytane wartosci Gcode przez zoom
//OBECNIE NIE UZYWANE - ZOOM==1
double zoom = DEFAULT_ZOOM_FACTOR;
double xscaling = X_SCALING_FACTOR;
double yscaling = Y_SCALING_FACTOR;

//Maksymalna szybkosc posuwu
const double maxFeedrate = 2000.;

//Deklaracja wartosci Pi
const float M_Pi = 3.141593;
// ------

void setup()
{
    
    lcd.begin(16,2);//Zalacznie wyswietlacza 16x2
    Serial.begin(BAUDRATE); //Utworzenie polaczenia z PC, nieuzywane w trybie pracy
    Serial.print(VERSIONCODE);
    Serial.print("\n");

    clear_buffer(); //Wyczyszczenie bufora do odczytu Gcode

    servo.attach(SERVO_PIN_1); //Podpiecie serva
    servo.write(DEFAULT_PEN_UP_POSITION); //Ustawienie serva w domyslnej pozycji

    if(servoEnabled) // Odswiezanie serva 
    {
        for(int i=0; i<100; i++)
        {
            SoftwareServo::refresh();
            delay(4);
        }
    }

    //Uruchomienie timera PWM do zapewnia odpowiedniego posuwu silnikow
    Timer1.initialize(TIMER_DELAY); // Timer do update'u pinow PWM
    Timer1.attachInterrupt(doInterrupt);

    //Zerowanie silnikow (do czasu zalacznie endstopow)
    xAxisStepper.autoHoming(); 
    rotationStepper.autoHoming();
    //Ustawienie pozycji po zerowaniu jako 0
    xAxisStepper.setTargetPosition(0.);
    rotationStepper.setTargetPosition(0.);
    //Commit steppers
    commitSteppers(maxFeedrate);
    delay(2000);
    xAxisStepper.enableStepper(false); //Wylaczenie silnika X
    rotationStepper.enableStepper(false); //Wylaczenie silnika Y
    
    
    checkSD(); //Sprawdzenie czy jest karta SD
    lcd.clear(); //Wyczyszczenie ekranu
    startMenu(); //Uruchomienie menu startowego, tryb odczytu lub wprowadzania Gcode, podanie nazwy pliku etc.
    myFile = SD.open(fileName,FILE_READ); //Zmienna sluzacaa do odczytu Gcode z karty SD
}

void loop() //Glowna petla
{

    get_command(); // odczyt komend z pliku na karcie SD i ich wykonywanie
    if(servoEnabled) //odswiezanie serva
        SoftwareServo::refresh();
    Serial.flush();
}


//Rutyna przerwan - Ruch silnikow
void doInterrupt()
{
    if(isRunning) //jesli silniki dzialaja
    {
        if(next_command_request_counter++ > next_command_request_maximum) // zerowanie licznika komend
        {
            next_command_request_counter = 0;
        }
        if (intervals_remaining-- == 0) //jesli brak kolejnych przerwan - ruchu silnika to wylacza silniki
            isRunning = false;
        else
        {
            rotationStepper.doStep(intervals); // robi stepy (ruch silnikow) w zaleznosci od przerwan (szybkosci posuwu i odleglosci)
            xAxisStepper.doStep(intervals);
        }
    }
}


//Oblicza liczbe interwalow (przerwan) dla silnikow w zaleznosci od szybkosci posuwu i odleglosci, ktora ma byc przebyta,
//odleglosc jest przeliczana na [mm] bez wzgledu czy Gcode jest w rad czy stopniach - koniecznosc deklaracji srednicy dla osi X i Y, 
//by przeliczac obroty na mm
void commitSteppers(double speedrate)
{
    long deltaStepsX = xAxisStepper.delta; // sprawdzenie przyrostu liczby krokow dla osi X (roznica miedzy akutalnym polozeniem, a wczytanym z Gcode)
    if(deltaStepsX != 0L)
    {
        xAxisStepper.enableStepper(true); //jesli !=0 to wlaczenie silnikow
    }

    long deltaStepsY = rotationStepper.delta; // analogiczenie dla osi Y
    if(deltaStepsY != 0L)
    {
        rotationStepper.enableStepper(true);
    }
    long masterSteps = (deltaStepsX>deltaStepsY)?deltaStepsX:deltaStepsY; //ustalenie masterStepa (???)

    //obliczenie odleglosci do pokonania w [mm] w osi X i Y, przy czym odleglosci jest zawsze w [mm], bo szybkosci posuwu jest w [mm/min]
    //dlatego radiany i stopnie zamieniane sa na [mm], konieczne jest zatem podanie srednicy dla osi X i Y, by przeliczyc obroty na [mm]
    double deltaDistanceX = xAxisStepper.targetPositionInMM-xAxisStepper.getCurrentPositionInMM(); 
    double deltaDistanceY = rotationStepper.targetPositionInMM-rotationStepper.getCurrentPositionInMM(); 

    // calkowita dlugosc lini do pokonania z Pitagorasa sqrt(deltaDistanceX^2+deltaDistanceY^2)
    double distance = sqrt(deltaDistanceX*deltaDistanceX+deltaDistanceY*deltaDistanceY);

    // Obliczenie liczby interwalow dla silnikow do pokonania odleglosci z odpowiednia predkoscia posuwu
    //Tu nie wnikałem jak to do konca jest policzone - TRZEBA PRZETESTOWAC
    double sub1 = (60000.* distance / speedrate);
    double sub2 = sub1 * 1000.;
    intervals = (long)sub2/TIMER_DELAY;
    intervals_remaining = intervals;
    const long negative_half_interval = -intervals / 2;
    rotationStepper.counter = negative_half_interval;
    xAxisStepper.counter = negative_half_interval;

    isRunning=true; 
}


//Funkcja do sprawdzania i zalaczania karty SD
void checkSD()
{
    while(!SD.begin(SD_CS_PIN))
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

//Funkcja w postaci menu startowego
//Wybranie 1 i zatwierdzenie # - uruchomienie trybu odczytu pliku z karty SD, wiecej w opisie funkcji getFileName
//Wybranie 2 i zatwierdzenie # - uruchomienie trybu wprowadzania, wiecej w opisie CamModule
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

    if(inputType == 1) //uruchomienie funkcji do pobierania nazwy
    {
        getFileName();
    }

    else if( inputType == 2) //uruchomienie trybu wprowadzania
    {
        CamModule();
    }
}


//Funkcja do generacji Gcode na podstawie wprowadzonych danych:
//srednicy rury, poczatkowe polozenia X,Y, kata ciecia lub promienia ciecia
//program oblicza polozenie z zaleznosci analitycznych - wiecej o sposobie obliczen w zalaczniku
//mozliwe albo ciecie pod katem albo wyciecie polowy okregu na koncu rury
//jesli promien ciecie ==0 wtedy ciecie pod katem
//jesli promien ciecia !=0 wtedy ciecie otworu
//Zatwierdzenie danych #, kasowanie *
//Polozenie X wyrazone jest w [mm], a Y to rozwiniecie ksztaltu na rurze, czyli w formie kat/(2PI)*srednica, także w [mm]
//Fukcja genereuje gcode, ktory jest zapisany na SD pod nazwa cam_mod, a pozniej jest wczytywany przez funkcje do interpretacji Gcode
//UWAGA! Program moze wykonywac wiecej niz jeden obrot, by dotrzec do zadanych polozen - RYZYSKO ZAPLATANIA KABLI -- mozliwosc modyfikacji przez zmiane G03 na G02

void CamModule()
{

    int pointsNumber=40; //liczba odcinkow no ktore dzielimy rure w zakresie 0-2PI
    //Deklaracja zmiennych
    double diameter;
    double start_pointX;
    double cut_angle;
    double cut_radius;
    double start_pointY;


    //Wczytanie danych - SREDNICA
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





    //WCZYTANIE DANYCH PKT STARTOWY X
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



    //WCZYTANIE DANYCH PUNKT STARTOWY Y
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


    //WCZYTANIE DANYCH - KAT CIECIE 0-90 STOPNI
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




    //WCZYTANIE DANYCH - PROMIEN OTWORU
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
    //KONIEC WCZYTYWANIA DANYCH

    //GENERACJA GCODE
    //ZMIENNE POMOCNICZE
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
    
    //WYCZYSZENIE POPRZEDNIEGO PLIKU I POCZATEK ZAPISU NOWEGO
    SD.remove("cam_mod");
    myFile = SD.open("cam_mod", FILE_WRITE);
    myFile.println("G90"); //TRYB ABSOLUTNY
    myFile.println("G20"); //TRYB W [MM]

    myFile.print("M401S"); //USTAWIENIE SREDNICY
    myFile.println(diameter);
    myFile.print("G0X"); //SZYBKI PRZEJAZD NA PUNKT POCZATKOWY X
    myFile.print(start_pointX);
    myFile.print("Y"); //SZYBKI PRZEJAZD NA PUNKT POCZATKOWY Y
    myFile.println(start_pointY); 
    myFile.println("M300S90"); // ZALACZENIE SERWA W POZYCJI 90 st. [DO USTALENIA EWENTUALNEGO!!!]


    double d=0; // przesuniecie osi miedzy otworem a cylindrem
    if( cut_radius != 0) //CIECIE OTWOREM
    {
        for(double alpha_y=0; alpha_y<2*M_PI; alpha_y=alpha_y+2*M_PI/pointsNumber) //GENERACJA PUNKTOW W ZAKRESIE 0 -2PI Z PODZIALEM 40 (alpha_y=alpha_y+2*M_PI/40)
        {
            tempX1=start_pointX-sqrt(cut_radius*cut_radius-(d-diameter/2*sin(alpha_y))*(d-diameter/2*sin(alpha_y))); //OBLICZENIE x
            tempY1=start_pointY+alpha_y*diameter/2; //OBLICZENIE POLOZENIA Y
            myFile.print("G1X"); //RUCH ROBOCZNY NA OBLICZONY X
            myFile.print(tempX1);
            myFile.print("Y"); //RUCH ROBOCZNY NA OBLICZONY Y
            myFile.println(tempY1);
        }

    }
    else if(cut_radius == 0) //CIECIE PLASZCZYZNA POD KATEM
    {

        for(double alpha_y=0; alpha_y<2*M_PI; alpha_y=alpha_y+2*M_PI/(pointsNumber/2)) //GENERACJA PUNKTOW W ZAKRESIE 0-2PI Z PODZIALEM 20
        {
            tempX3=start_pointX+diameter/2*tan(cut_angle)*sin(alpha_y); //GENERACJA PUNKTOW X
            tempY3=start_pointY+alpha_y*diameter/2; // GENERACJA PUNKTOW Y
            myFile.print("G1X"); // RUCH ROBOCZNY X
            myFile.print(tempX3);
            myFile.print("Y"); // RUCH ROBOCZY Y
            myFile.println(tempY3);
        }

    }

    myFile.println("M300S0"); // SERWO NA POZYCJE 0
    //myFile.println("G0X0Y0"); // RUCH NA POZYCJE X0Y0
    myFile.println("G92"); //AUTOHOMING
    myFile.println("M18"); //WYLACZENIE SILNIKOW
    myFile.close(); // ZAMKNIECIE PILKU
    fileName="cam_mod"; //USTAWIENIE NAZWY PLIKU DO ODCZYTU PRZEZ PROGRAM

}


//Funkcja do pobierania i wyswietlania nazwy pliku do odczytu Gcode z karty SD
//zapisuje nazwe do zmiennej fileName
//Nazwa moze skladac sie tylko z cyfr 1-9
//Nazwe zatwierdzamy #
//Anulujemy i kasujemy *
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
            if (key == 42) //kasowanie *
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

            else if (key != 35) //wczytanie znakow dopóki rozne od #
            {
                entryStr[i]= key;
                i++;
                lcd.print(key);
            }
            else //zatwierdzenie #
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
    lcd.println(fileName); //wyswietlenie nazwy pliku
    delay(2000);
}


//Funkcja do odczytywania pliku z Gcode, wyluskiwania komend i przerzucania ich do zmiennej
//Glownia modyfikacjia z porownaniu do SphereBota to zmiana odczytu z serial na odczyt z pliku
void get_command()
{
    while(myFile.available()) // jesli plik jest dostepny
    {
        if (!isRunning && myFile.available() > 0) // sprawdzamy czy dziala plik
        {

            serial_char = myFile.read(); // czytamy pojedynczo kazdy znak
            Serial.print(serial_char); //wydruk do komputera w trybie testowym
            lcd.clear();
            lcd.print(serial_char); //niedziala???
            if (serial_char == '\n' || serial_char == '\r') // koniec komedy jesli znak \n lub \r
            {
                next_command_request_counter = 0;
                buffer[serial_count]=0;
                process_commands(buffer, serial_count);
                clear_buffer(); //wyczyszczenie bufora i licznikow
                comment_mode = false; 
            }
            else // jesli nie koniec
            {
                if (serial_char == ';' || serial_char == '(') // komentarz w gcodzie jesli jest ; - ignorowanie
                {
                    comment_mode = true;
                }

                if (comment_mode != true) 
                {
                    buffer[serial_count] = serial_char; // dodanie znaku do buforu
                    serial_count++;
                    if (serial_count > MAX_CMD_SIZE) // przepelnienie bufory->restart
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


//Funckja - czyszczenie buforu
void clear_buffer() // empties command buffer from serial connection
{
    serial_count = 0; // reset buffer placement
}


//Funkcja zwracaja polozenie slowa klucza
boolean getValue(char key, char command[], double* value)
{
    //szukanie kluczy komend
    strchr_pointer = strchr(buffer, key);
    if (strchr_pointer != NULL) // Znaleziony znak klucz
    {
        *value = (double)strtod(&command[strchr_pointer - command + 1], NULL);
        return true;
    }
    return false;
}


//Funkcja nieuzywana - dekoracyjna do zwracania wersji programu
void check_for_version_controll(char command)
{
    if(command == 'V')
    {
        Serial.print(VERSIONCODE);
        Serial.print("\n");
    }
}


//Glowna funkcja interpretujaca Gcode, wykonuje zadanie po wczytaniu slowa KLUCZA
void process_commands(char command[], int command_length)
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
        if (command_length>0 && command[j] == 'G') // Przechywcenie wartosci X,Y,I,J,R,P jesli stoi przed nimi G
        {
            //Serial.print("process G: \n");
            int codenum = (int)strtod(&command[j+1], NULL);
            
            //Pobranie tymczasowego polozenia silnikow X i Y do obliczen
            double tempX = xAxisStepper.getCurrentPosition();
            double tempY = rotationStepper.getCurrentPosition();
            
	    //Pobranie wartosci X,Y,I,J,R i przemnozenie ich przez skale i zoom-y jesli sa ustawione - domyslnie sa 1
	    //oraz P- czas opoznienia, F -szybkosc posuwu
            double xVal;
            boolean hasXVal = getValue('X', command, &xVal);
            if(hasXVal) xVal*=zoom*xscaling;
            double yVal;
            boolean hasYVal = getValue('Y', command, &yVal);
            if(hasYVal) yVal*=zoom*yscaling;
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
	    
            //Obliczenie wartosci X i Y, jesli jest ustawiony offset
            xVal+=currentOffsetX;
            yVal+=currentOffsetY;
            
	    //Obliczenie wartosci X i Y w zaleznosci czy to tryb absolutny czy przyrostowy
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
            
            //Interpretacja komend GCode
            switch(codenum)
            {
            case 0: // G0, szybkie najazda na pozycje X i Y z maksymlanym posuwem
                xAxisStepper.setTargetPosition(tempX);
                rotationStepper.setTargetPosition(tempY);
                commitSteppers(maxFeedrate);
                break;
            case 1: // G1X..Y.., liniowa interpolacja na pozycje X,Y z zadanym posuwem roboczym
                xAxisStepper.setTargetPosition(tempX);
                rotationStepper.setTargetPosition(tempY);
                commitSteppers(feedrate);
                break;
            case 2: // G2I..J.., ruch zgodnie ze wskazowkami zegara, gdy zadany srodek promienia I,J
                if(hasIVal && hasJVal)
                {
                    double centerX=xAxisStepper.getCurrentPosition()+iVal;
                    double centerY=rotationStepper.getCurrentPosition()+jVal;
                    drawArc(centerX, centerY, tempX, tempY, (codenum==2));
                }
                else if(hasRVal) //dla wartosci promieniA NIE DZIALA
                {
                    //drawRadius(tempX, tempY, rVal, (codenum==2));
                }
            case 3: // G3I...J..., ruch przeciwnie do wskazowek zegara, gdy zadany srodek promienia I,J
                if(hasIVal && hasJVal)
                {
                    double centerX=xAxisStepper.getCurrentPosition()+iVal;
                    double centerY=rotationStepper.getCurrentPosition()+jVal;
                    drawArc(centerX, centerY, tempX, tempY, (codenum==2));
                }
                else if(hasRVal) //dla promienia NIE DZIALA
                {
                    //drawRadius(tempX, tempY, rVal, (codenum==2));
                }
                break;
            case 4: // G4P..., P opóznienie P  w ms
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

            case 20: // G20, oś Y wyrazona w mm [mm]
                rotationStepper.resetSteppersForMoveType(1);
                break;
            case 21: // G21, oś Y wyrazona w radianach [rad]
                rotationStepper.resetSteppersForMoveType(2);
                break;
            case 22: // G22, os Y wyrazona w stopniach [deg]
                rotationStepper.resetSteppersForMoveType(3);
                break;
            case 90: // G90, Pozycjonowanie absolutne
                absoluteMode = true;
                break;
            case 91: // G91, Pozycjonowanie przyrostowe
                absoluteMode = false;
                break;
            case 92: // G92 AutoHoming - zerowanie polozenia silnikow z wykorzystanie endstopow
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
            case 18: // M18 Wylaczenie silnikow 
                xAxisStepper.resetStepper();
                rotationStepper.resetStepper();
                break;

            case 300: // M300S... Pozycja serwa
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

            case 400: // M400S... przypisanie nowej srednicy dla osi X
                if(getValue('S', command, &value))
                {
                    xAxisStepper.resetSteppersForObjectDiameter(value);
                    xAxisStepper.setTargetPosition(0.);
                    commitSteppers(maxFeedrate);
                    delay(2000);
                    xAxisStepper.enableStepper(false);
                }
                break;

            case 401: // M401S... przypisanie nowej srednicy dla osi Y
                if(getValue('S', command, &value))
                {
                    rotationStepper.resetSteppersForObjectDiameter(value);
                    rotationStepper.setTargetPosition(0.);
                    commitSteppers(maxFeedrate);
                    delay(2000);
                    rotationStepper.enableStepper(false);
                }
                break;

            //NIEUZYWANE!!!
            case 402: // M402S... przypisanie nowego zoomu
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


//Funkcja do obliczania promienia, uzywana w G02 i G03
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



