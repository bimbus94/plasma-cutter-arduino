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

## 3. Kod Arduino
### 3.1 Założenia
Wycinarka wykonuje swoją pracę na podstawie GCode, który jest przekazywany na karcie microSD (działa bez komputer). GCode może być napisany ręcznie/ otrzymany za pomocą makra, które zostanie później omówione lub utworzony ad hoc podczas pracy wycinarki w trybie wprowadzania (omówione niżej). Program wykorzystuje ekran LCD i klawiaturę membranową do wprowadzania danych. Dodatkowo wyposażony jest w funkcje zerowania położenia silników (autoHoming) za pomocą endstopów.
### 3.2 Tryby pracy
#### 3.3 Tryb odczytu
 



