#brief:
#Makro do FreeCADa testowane na wersji v0.17 na Ubuntu 14.04

#Makro sluzy do generacji prostego Gcode z wybranej krzywej (trzeba wybrac krzywa lub kilka krzywych z przyciskiem CTRL przed uruchomieniem makra,by zadzialalo)

#Makro pozwala na generacje z krzywej 3-wymiarowej (X,Y,Z) Gcode w formie 2-wymiarowej (X,Y'), gdzie X jest wspolrzedna pozostajaca bez zmian (w przypadku wycinarki plazmowej jest to os wzdluzna (wzdluz osi rury)),
#natomiast Y' powstaje w wyniku odpowiedniej transformacji wspolrzednych Y i Z i moze byc wyrazone jako: 
#-rozwiniecie krzywej na rurze (zakres od 0-2*Pi*srednica lub od -Pi*srednica do Pi*srednica)
#-kat od 0-360 stopni lub -180-180 stopni 
#-pozostawienie Y bez zmian, i calkowite pominiecie Z, przydatne do plaskich krzywych znajdujacych sie w plaszczyznie XY np.:do plotera

#Makro sklada sie zasadniczo z 3-ech czesci:
#1. odpowiedzialnej za generacje okna do wprowadzania danych (w PySide)
#2. odpowiedzialnej za dyskretyzacje krzywej o zadanej przez uzytkownika liczbie pkt-ow i transformacje wspolrzednych 
#3. zapis gcode do pliku->UWAGA! nalezy dostosowac go do wlasnych potrzeb, dodac wlasne komendy, ktore beda potrzebne, sprawdzic sciezke zapisu pliku-path (domyslnie to sciezka freecada), kat wlaczonego i wylaczonego serwa etc.


#---------------------------------------------------------------#
#import modulow
import Part
import FreeCAD
from FreeCAD import Base
from PySide import QtGui, QtCore


#~~~~~~~1. Wprowadzanie danych~~~~~~~~#

#deklaracja klasy okna do wprowadzania danych (PySide; patrz opis makra do generacji geometrii)
class WindowGcode(QtGui.QDialog):
	""""""
	def __init__(self):
		super(WindowGcode, self).__init__()
		self.initUI()
	def initUI(self):
		self.result = userCancelled
		# create our window
		# define window		xLoc,yLoc,xDim,yDim
		self.setGeometry(	250, 150, 600, 600)
		self.setWindowTitle("Generator Gcode")
		self.setWindowFlags(QtCore.Qt.WindowStaysOnTopHint)


		#Dane cietej rury
		self.glabel1 = QtGui.QLabel("1. Podstawowe dane", self)
		self.glabel1.move(20, 10)

		self.glabel2 = QtGui.QLabel("Liczba pkt-ow dyskretyzacji:", self)
		self.glabel2.move(20, 30)

		self.glabel3 = QtGui.QLabel("Nazwa pliku do zapisu:", self)
		self.glabel3.move(20, 60)

		self.glabel4 = QtGui.QLabel("Sciezka dostepu NIE DZIALA:", self)
		self.glabel4.move(20, 90)

		#Liczba pkt-ow dyskretyzacji
		self.numberOfPoints = QtGui.QLineEdit(self)
		self.numberOfPoints.setInputMask("999")
		self.numberOfPoints.setText("100")
		self.numberOfPoints.setFixedWidth(50)
		self.numberOfPoints.move(250, 30)

		#Nazwa pliku
		self.fileName = QtGui.QLineEdit(self)
		self.fileName.setText("123")
		self.fileName.setFixedWidth(50)
		self.fileName.move(250, 60)
		
		#sciezka->nie dziala!!!
		self.path = QtGui.QLineEdit(self)
		self.path.setText("abc")
		self.path.setFixedWidth(50)
		self.path.move(250, 90)

		#Przyciski do wyboru rodzaju rozwiniecia wspolrzednej Y, RadioButton zapewnie, ze mozna wybrac tylko 1 rodzaj
		self.glabel5 = QtGui.QLabel("2. Wybierz rodzaj rozwiniecia Y:", self)
		self.glabel5.move(20, 140)
		self.gbutton1 = QtGui.QRadioButton("[1] Y w [stopnie] 0-360", self)
		#self.gcheckbox1.clicked.connect(self.gonCheckbox1)
		self.gbutton1.move(40, 170)
		
		self.gbutton2 = QtGui.QRadioButton("[2] Y w [stopnie] (-180)-(180)", self)
		#self.gcheckbox2.clicked.connect(self.gonCheckbox2)
		self.gbutton2.move(40, 200)
		
		self.gbutton3 = QtGui.QRadioButton("[3] Y rozwiniete w [mm] 0-(2*pi*R)", self)
		#self.gcheckbox3.clicked.connect(self.gonCheckbox3)
		self.gbutton3.move(40, 230)
		
		self.gbutton4 = QtGui.QRadioButton("[4] Y rozwiniete w [mm] (-pi*R)-(pi*R)", self)
		#self.gcheckbox4.clicked.connect(self.gonCheckbox4)
		self.gbutton4.move(40, 260)

		self.gbutton5 = QtGui.QRadioButton("[5] X i Y pozostawione bez zmian w [mm]", self)
		#self.gcheckbox5.clicked.connect(self.gonCheckbox5)
		self.gbutton5.move(40, 290)
		
		#Pozostale parametry obrobki
		self.glabel6 = QtGui.QLabel("3. Paramatery obrobki", self)
		self.glabel6.move(20, 320)

		self.glabel7 = QtGui.QLabel("Posuw [mm/min]-nie dziala", self)
		self.glabel7.move(20, 350)


		self.glabel8 = QtGui.QLabel("Srednica rury [mm]", self)
		self.glabel8.move(20, 380)


		#Posuw - nie dziala
		self.feedX= QtGui.QLineEdit(self)
		self.feedX.setInputMask("9999")
		self.feedX.setText("1000")
		self.feedX.setFixedWidth(50)
		self.feedX.move(250, 350)


		#srednica rury		
		self.diameter = QtGui.QLineEdit(self)
		self.diameter.setInputMask("9999")
		self.diameter.setText("100")
		self.diameter.setFixedWidth(50)
		self.diameter.move(250, 380)

		# cancel button
		gcancelButton = QtGui.QPushButton('Cancel', self)
		gcancelButton.clicked.connect(self.gonCancel)
		gcancelButton.setAutoDefault(True)
		gcancelButton.move(260, 550)
		# OK button
		gokButton = QtGui.QPushButton('OK', self)
		gokButton.clicked.connect(self.gonOk)
		gokButton.move(150, 550)
		# now make the window visible
		self.show()

	def gonCheckbox1(self):	
		return True

	def gonCheckbox2(self):
		return True

	def gonCheckbox3(self):
		return True

	def gonCheckbox4(self):
		return True

	def gonCheckbox5(self):
		return True


	def gonCancel(self):
		self.result = userCancelled
		self.close()
	def gonOk(self):
		self.result = "OKEY"
		self.close()


userCancelled = "Cancelled"
userOK = "OK"

#-----------------------------------------------------------------------------#

#Utworzenie obiektu, wywolanie okna do wprowadzania danych, przypisanie wprowadzonych danych do zmiennych

form2 = WindowGcode()
form2.exec_()

if form2.result == userCancelled:
	pass

if (form2.result == "OKEY"):	
	print("OK")
	
	#Przypisanie wprowadzonych danych do zmiennych
	name=str(form2.fileName.text()) #nazwa pliku z gcodem
	path=str(form2.path.text()) #sciezka- nie dziala
	feedX=int(form2.feedX.text()) #posuw - nie dziala
	diameter=int(form2.diameter.text()) #srednica rury
	numberOfPoints=int(form2.numberOfPoints.text()) #liczba pkt-ow dyskretyzacji


##########################################################################################3

#~~~~~~~2a. Funkcje do transformacji wspolrzednych z Y,Z na Y' (rozwiniete w [mm], [stopniach], [radianach])~~~~~~~~#

import numpy as np

#Funkcja tranformujaca wspolrzedne kartezjanskie Y,Z na kat w radianach (w zakresie 0-2*Pi, jesli negativeRangeIn..==False lub -Pi-Pi jest negativeRangeIn..==True)  i promien 
#cartesian to radius and angle (0-2*PI)  or (-Pi-Pi when negativeRangeIn.. is true)
def cart2pol(x, y, negativeRangeInRad=False, negativeRangeInDeg=False):
    rho = np.sqrt(x**2 + y**2)
    phi = np.arctan2(y, x)
    if (negativeRangeInRad or negativeRangeInDeg):
	if phi<0.0:
		phi=phi+2*np.pi
    return(rho, phi)

#Funkcja transformujace wspolrzedne kartezjanskie Y,Z na rozwiniecie Y' w [mm], zwraca Y' w zakresie 0-2Pi*R lub -Pi*R-Pi*R, w zaleznosci czy negativeRange.. jest False lub True 
#cartesian to curve_lenght=radius*degree and z coordinates; 0-2Pi*R or -Pi*R-Pi*R when negativeRange... is true
def cart2curve_len(x,y, negativeRangeInLen=False):
	rho, phi = cart2pol(x,y,negativeRangeInLen)
	curve_len=rho*phi
	return (curve_len)

#Funkcja transformujace wspolrzedne kartezjanskie Y,Z na Y' w [stopniach], zwraca Y' w zakresie 0-360 [deg] lub -180-180[deg], w zaleznosci czy negativeRange.. jest False lub True 
#cartesian to radius and angle in degree (0-360 deg) or (-180-180deg when negativeRangeIn.. is true)
def cart2polDeg(x,y, negativeRangeInRad=False, negativeRangeInDeg=False):
    rho = np.sqrt(x**2 + y**2)
    phi = np.arctan2(y, x)
    if (negativeRangeInRad or negativeRangeInDeg):    
	if phi<0.0:
		phi=phi+2*np.pi
    phi=phi*180.0/np.pi
    #return(rho, phi)
    return phi

#Funkcja do transformacji zmiennych polarnych na kartezjanskie
#polar to cartesian x,y
def pol2cart(rho, phi):
    x = rho * np.cos(phi)
    y = rho * np.sin(phi)
    return(x, y)

###################################################################################

#~~~~~~~2b. Pobranie i dyskretyzacja krzywej~~~~~~~~~#
#import modulow
import Draft

#---------Dyskretyzacja zaznaczonych krzywych, uzyskanie macierzy z punktami, ktore beda pozniej poddane transformacji---------#

s=0
allPoints=[] #lista do przechowywania wspolrzednych punktow wszystkich krzywych po dyskretyzacji
wire=[] #lista pomocnicza do przechowywania wspolrzednych punktow, by pozniej narysowac krzywa
for wires in FreeCADGui.Selection.getSelectionEx()[0].SubObjects:#petla po wszystkich zaznaczonych w oknie krzywych
	selectedEdge = wires.copy()#skopiowanie krzywej, by moc dalej na niej dzialac
	points = selectedEdge.discretize(numberOfPoints)#dyskretyzacja krzywej na zadana przez uzytkownika liczbe punktow		
	i=0 #iterator
	row=[] #lista zawierajaca wspolrzedne 1 punktu
	pointsList=[] #lista zawierajaca wszystkich punktow pojedynczej krzywej
	for k in points:#iteracja po wszystkich punktach powstalych po dyskretyzacji
		row=[i, k.x, k.y, k.z]#pojedynczy wiersz [nr pktu, X,Y,Z]
		wire.append(k) # dodanie punktu do listy, by pozniej ja narysowac
		pointsList.append(row) #dodanie pojedynczego punktu(pojedynczego wiersza) do listy z wszystkimi punktami danej krzywej
		i+=1
		print i, " X", k.x, " Y", k.y, " Z", k.z #drukowanie wspolrzednych na ekranie
	allPoints.append(pointsList) # dodanie punktow pojedynczej krzywej do listy zawierajacej punkty wszystkich zaznaczonych krzywych
	wireAll=Draft.makeWire(wire,closed=False,face=False,support=None)  # stworzenie krzywej z punktow, by je narysowac dla podgladu we Frecadzie
	#Draft.makeWire(wire,closed=True,face=False,support=None)   # create the wire closed (uncomment for use)
	wire=[] #czyszczenie listy


#########################################################################################

#---------Transformacja punktow powstalych po dyskretyzacji w zaleznosci od wybranego rodzaju rozwiniecia Y'----------#

#----------Transformacja na Y' wyrazone w STOPNIACH---------#
###Zmiana wspolrzednych kartenzjanskich na polarne; X pozostaje takie samo; zmiana YZ na kat 0-360 stopni lub -180-180
###W Gcodze X to wspolrzedna wzdluz rury w [mm], przy Y stoi kat obrotu

allPointsPolDeg=[]#lista do przechowywania wspolrzednych po transformacji
wire=[]#lista do przechowywania pkt-ow, by je pozniej narysowac we FreeCadzie w celach pogladowych

for p in allPoints:#petla po liscie z punktami wszystkich krzywych
	pointsList=[]#lista z pkt-ami pojedynczej krzywej
	row=[]#pojedynczy punkt (wiersz)
	for r in p:#petla po liscie z pkt-ami pojedynczej krzywej; r - to pojedynczy wiersz (punkt)
		print r[0], " X", r[1], " Y" ,cart2polDeg(r[2], r[3], form2.gbutton1.isChecked(),form2.gbutton3.isChecked())#wydruk pogladowy
		row=[r[0], r[1], cart2polDeg(r[2], r[3], form2.gbutton1.isChecked(),form2.gbutton3.isChecked())]#row - pojedynczy wiersz, pojednczy punkt po transformacji #r[0]-nr pkt-u, r[1] - wspolrzedna X[mm] - pozostaje bez zmian, cart2polDeg bierze wspolrzedne Y oraz Z, a zwraca Y' wyrazone w stopniach (-180-180 albo 0-360 w zaleznosci, ktory przycisk byl zaznaczony w oknie wyboru)
		wire.append(Base.Vector(r[1],cart2polDeg(r[2], r[3],form2.gbutton1.isChecked(),form2.gbutton3.isChecked()),0)) # wire-pomocnicza lista z punktami, by narysowac krzywa w oknie FreeCada w celach pogladowych
		pointsList.append(row) #dolacznie pojedynczego wiersza z pkt-em do listy zbiorczej pkt-ow po transformacji
	allPointsPolDeg.append(pointsList) #dodanie pkt-ow po (transformacji) pojedynczej krzywej do listy zbiorczej
	wireDeg=Draft.makeWire(wire,closed=False,face=False,support=None)   # utworzenie krzywej pogladowej w oknie FreeCada
	#Draft.makeWire(wire,closed=True,face=False,support=None)   # create the wire closed (uncomment for use)
	wire=[]


####################################################################################################

#--------------Transformacja na Y' wyrazone jako rozwiniecie krzywej na na cylindrze w [mm]---------#

###W Gcodze X to wspolrzedna wzdluz rury w [mm], przy Y' to rozwiniecie rury w [mm]=r*kat obrotu[rad]
###Y' w zakresie od -Pi*r do Pi*r lub 0-2Pi*r w zaleznosci od wyboru uzytkownika###

allPointsCurve_Len=[] #lista do przechowywania pkt-ow po transformacji
wire=[] #lista pomocnicza do
for p in allPoints:#petla po liscie z punktami wszystkich krzywych
	pointsList=[]#lista z pkt-ami pojedynczej krzywej
	row=[]#pojedynczy punkt (wiersz)
	for r in p:#petla po liscie z pkt-ami pojedynczej krzywej; r - to pojedynczy wiersz (punkt)
		print r[0], " X", r[1], " Y" ,cart2curve_len(r[2], r[3], form2.gbutton3.isChecked())#wydruk pogladowy do konsoli
		row=[r[0], r[1], cart2curve_len(r[2], r[3], form2.gbutton3.isChecked())]#row - pojedynczy wiersz, pojednczy punkt po transformacji #r[0]-nr pkt-u, r[1] - wspolrzedna X[mm] - pozostaje bez zmian, cart2curve_Len bierze wspolrzedne Y(r[2]) oraz Z(r[3]), a zwraca Y' wyrazone w [mm] jako rozwiniecie krzywej na cylindrze R*kat (-Pi*R-Pi*R albo 0-2Pi*R w zaleznosci, ktory przycisk byl zaznaczony w oknie wyboru)
		wire.append(Base.Vector(r[1],cart2curve_len(r[2], r[3], form2.gbutton3.isChecked()),0)) # wire-pomocnicza lista z punktami, by narysowac krzywa w oknie FreeCada w celach pogladowych
		pointsList.append(row)#dolacznie pojedynczego wiersza z pkt-em do listy zbiorczej pkt-ow po transformacji
	allPointsCurve_Len.append(pointsList)#dodanie pkt-ow po (transformacji) pojedynczej krzywej do listy zbiorczej
	wireLen=Draft.makeWire(wire,closed=False,face=False,support=None)   # utworzenie krzywej pogladowej w oknie FreeCada
	#Draft.makeWire(wire,closed=True,face=False,support=None)   # create the wire closed (uncomment for use)
	wire=[]

################################################################################################

#-----------Wybranie rodzaju punktow do zapisu w zaleznosci od wybranej w oknie opcji-----------------#

generatePoints=[] #lista z punktami do zapisu

if form2.gbutton1.isChecked() or form2.gbutton2.isChecked():#jesli zaznaczony jest przycisk 1 lub 2
	generatePoints=allPointsPolDeg #pkty do zapisu do pkt-y wyrazone w postaci x[mm] i Y'[deg]
	print("wybrano allPointsPolDeg")
	moveType=2 #ustawienie ruchu w stopniach; potrzebne do ustawien programu w Arduino
	#Part.show(wireDeg)
elif form2.gbutton3.isChecked() or form2.gbutton4.isChecked():#jesli zaznaczony jest przycisk 3 lub 4
	generatePoints=allPointsCurve_Len
	print("wybrano allPointsPolLen") #pkty do zapisu do pkt-y wyrazone w postaci x[mm] i Y' [mm] (rozwiniecie krzywej na rurze)
	moveType=0 #ustawienie ruchu w [mm]; potrzebne do ustawien programu w Arduino
	#Part.show(wireLen)
else:
	generatePoints=allPoints 
	print("wybrano allPoints") #pkty do zapisu do pkt-y wyrazone w postaci x[mm] i Y [mm]; X i Y pozostaja bez zmian, pominieta zostala wspolrzedna Z 
	#Part.show(wireAll)
	moveType=0 #ustawienie ruchu w [mm]; potrzebne do ustawien programu w Arduino

##############################################################################################

#~~~~~~~~~~~~~~3. ZAPIS GCODE DO PLIKU~~~~~~~~~#

#Tresc Gcode mozna spersonalizowac wg wlasnego uznania, dodac wlasne komendy lub usunac niepotrzebne
#Jesli wystepuja bledy z zapisem zmienic sciezke path, w ktorej bedzie zapisywany nasz plik
#Zmienic pozycje serwa dla wlaczonego palnika - zmienna servoON (domyslnie 90 stopni) i wylaczonego - zmienna servoOFF (domyslnie 0 stopni) 

import os
path=os.getcwd()+"/"+name #domyslnie bierze sciezke do FreeCada i tworzy plik o podanej przez uzytkownika nazwie
#dodac zmienna path
#path='./1234.txt'


servoON=90 #pozycja serwa dla wlaczonego palnika w stopniach
servoOFF=0 #pozycja serwa dla wylaczonego palnika w stopniach


#Zapis Gcode do pliku podanego w path
with open(path, 'w') as gcode_file: #otworzeniu pliku
	gcode_file.write('G90\n') ##tryb absolutny
        gcode_file.write('M401S'+str(diameter)+'\n') ##ustawienie srednicy dla osi Y
        gcode_file.write('M2'+str(moveType)+'\n') ##ustawienie trybu ciecie M20 w [mm], M22 w [stopniach]
	gcode_file.write('G92\n') ##autohoming
	for p in generatePoints: #petla przez punkty		
		gcode_file.write('G00 X%.1f Y%.2f\n' % (float(p[0][1]), float(p[0][2]))) ###najazd szybki na punkt poczatkowy####
		gcode_file.write('M300S'+str(servoON)+'\n') ####wlaczenie palinka####
		for r in p[1:]: 
			gcode_file.write("G01 X%.1f Y%.2f\n" % (float(r[1]), float(r[2])))#ruch roboczy na wspolrzedne G01X..Y...	
		gcode_file.write('M300S'+str(servoOFF)+'\n') #wylaczenie palnika
	gcode_file.write('G92\n') ##autohoming
	gcode_file.write('M18\n') ##wylaczenie silnikow
		

print("Sciezka dostepu do pliku to " + path)



