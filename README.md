# Wycinarka plazmowa do rur oparta na Arduino v0.1

![wycinarka](img/wycinarka.png)

## 1. Wstęp
Repozytorium zawiera kod oprogramowania wycinarki plazmowej do rur oparty na Arduino oraz makro do FreeCada do generacji geometrii ciętej rury oraz generacji GCode do tej konkretenej maszyny. 

Wyjściowym kodem do oprogramowania był kod SPhereBota z repozytorium: https://github.com/zaggo/SphereBot

Pod względem mechanicznym wycinarka była oparta na projekcie: https://www.youtube.com/watch?v=qnYNEfv5IaY

Rozwiązania mechaniczne, umiejscowienie silników i ogólny wygląd i opis maszyny można znaleźć w [Raport z części prac](przydatne_pdfy/Raport_z_czesci_prac.pdf) , przyczym pod względem sterowania program uległ znacznym zmianom w porównaniu do opis z przełomu 2017/2018, dodatkowo opracowano makra nie uwzględnione w tamtym raporcie.

Warto dodać główne założenie projektu: cięta rura jest nieruchoma, a wzdłuż się osi (za ruch wzdłuż odpowiada silnik związany z osią X) oraz po obwodzie rury (odpowiada za to silnik związany z osią Y).

## 2. Schemat podłączenia
Schemat podłączenia elementów do płytki Arduino i zestaw części można znaleźć na schemacie [Schemat podłączenia](projekt_sterowania_wycinarka_plazmowa.pdf)

## 3. [Kod Arduino](arduino)
### 3.1 Zalety i wady
+Wycinarka wykonuje swoją pracę na podstawie GCode, który jest przekazywany na karcie microSD (działa bez komputer).

+GCode może być napisany ręcznie/ otrzymany za pomocą makra, które zostanie później omówione lub utworzony ad hoc podczas pracy wycinarki w trybie wprowadzania (omówione niżej).Program wykorzystuje ekran LCD i klawiaturę membranową do wprowadzania danych.

+Dodatkowo wyposażony jest w funkcje zerowania położenia silników (autoHoming) za pomocą endstopów. 

+Serwomechanizm odpowiada natomiast za włączenie/wyłączanie palnika wycinarki.

+Krzywa może być wyrażona jako rozwinięcie na rurze, w zakresie 0-PI*ŚrednicaRury lub -PI*ŚrednicaRury/2, w [mm]

+Krzywa może być wyrażona w stopniach, w zakresie 0-360[deg] lub -180 - 180[deg]

+Krzywa może być wyrażona w radianach, w zakresie 0-2PI lub -Pi - Pi 

+Możliwość łatwego przekształcenia na ploter XY w [mm]

+Możliwość ustalenia maksymalnego zakresu obu silników -> zabezpieczenie przed zaplątaniem i zerwaniem kabli

-Konieczność zdefiniowania odpowiednich średnic, które informują o tym jaką zmianę położenia palnika powoduje jeden obrót silnika (omówione zostanie w rozdziale o uruchomieniu programu!!), by zapewnić odpowiednią synchronizację silników.

-Brak awaryjnego przycisku STOP, który wyłączyłby palnik (ruch serwem na 0) oraz silniki


### 3.2 Jak zacząć
1. Aby uruchomić program ściągnij pliki z [Kod Arduino](arduino) . Dodatkowo będzie potrzebna zmiana nazwy pobranego folderu z arduino na wycinarka_plazmowa.

2. Ściągnij i zainstaluj wszystkie niezbędne biblioteki:
,,,
	SoftwareServo: http://www.arduino.cc/playground/ComponentLib/Servo
	TimerOne: http://www.arduino.cc/playground/Code/Timer1
	Keypad.h https://playground.arduino.cc/Code/Keypad
	LiquidCrystal.h https://playground.arduino.cc/Main/LiquidCrystal
	SD.h https://www.arduino.cc/en/Reference/SD
	SPI.h https://www.arduino.cc/en/reference/SPI
,,,	
	
3. Połącz układ tak jak przedstawiono na [Schemat podłączenia](projekt_sterowania_wycinarka_plazmowa.pdf)

4. Ustaw odpowiednie napięcie na sterownikach silników w zależności od modelu:
(https://howtomechatronics.com/tutorials/arduino/how-to-control-stepper-motor-with-a4988-driver-and-arduino/)

5. Przejżyj kody programu, które zostały dokładnie opisane komentarzami.

Plik **config.h** zawiera ustawienia podłączenia pinów, deklaracje i ustawienia wartości domyślne wszystkich komponentów układu.

Plik **StepperModel** jest deklaracją klasy silnika i zawiera jego zmienne/ustawienia oraz funkcje za pomocą których możemy manipulować silnikiem (m.in. sterowanie ruchem, prędkością, rodzajem zmiennych, w których czytany jest GCode, autoHoming itd.)

Plik **wycinarka_plazmowa.ino** zawiera główny program wycinarki.



### 3.3 Tryby pracy
#### 3.3.1 Tryb odczytu
Polega na odczycie komend GCode z pliku zapisanego na karcie microSD. Gcode w takiej formie można otrzymać używając do tego stworznego w tym celu makra do FreeCada. Dostępne komendy:

**G0X...Y...** Szybki ruch na współrzędne X i Y

**G1X...Y...F...** Ruch roboczy na współrzędne X I Y

**G1F...** F-szybkość posuwu w mm/min

**G2X...Y...I...J...** Ruch po okręgu zgodnie z wskazówkami zegara, do punktu X,Y o środku w I,J (UWAGA, komenda dodana przeze mnie, więc też wymaga testów)

**G3X...Y...I...J...** Ruch po okręgu przeciwnie do wskazówek zegara, do punktu X,Y o środku w I,J

**G20** Oś Y wyrażona w mm, by odpowiednio odczytywać Gcode wyrażony w mm

**G21** Oś Y wyrazona w radianach, by odpowiednio odczytywać Gcode wyrażony w radianach 

**G22** Oś Y wyrazona w stopniach , by odpowiednio odczytywać Gcode wyrażony w stopniach

**G4P...** Przerwa ... milisekund

**G90** Pozycjonowanie absolutne

**G91** Pozycjonowanie przyrostowe

**G92** AutoHoming - zerowanie silników

**M18** Wyłączenie silników

**M300S...** Ustawienie serwa od 0-180

**M400S...** Zmiana domyślnej średnicy osi X (średnica wynika z konstrukcji przełożenia ruchu obrotowego na postępowy)

**M401S...** Zmiana domyślnej średnicy osi Y (średnica==średnica ciętej rury)

#### 3.3.2 Tryb wprowadzania (CamModule)
Tryb umożliwiający wycięcie połowy otworu prostopadłego do osi rury o wybranej średnicy na końcy rury lub ucięcie rury płaszczyzną, na podstawie danych wprowadzonych przez użytkownika w trybie interaktywnym za pomocą membranowej klawiatury. W tym przypadku nie potrzeba ręcznego pisania Gcode, ani jego generacji w komputerze, lecz jest on tworzony już w Arduino. Więcej na jego temat [Opis trybu wprowadznia](przydatne_pdfy/tryb_wprowadzania.pdf) oraz matematyczne zależności, na których został oparty [Artykuł o rozwijaniu krzywej na cylindrze](przydatne_pdfy/apostol_unwrapping.pdf.pdf)

 



