#brief:
#Makro do FreeCADa testowane na wersji v0.17 na Ubuntu 14.04
#Makro sluzace do szybkiej generacji geometri cietej rury - docelowo na wycinarce plazmowej.
#Umozliwia narysowanie rury o dowolnej srednicy, grubosci, dlugosci, o osi cylindra wzdluz osi X.
#Ciecie plaszczyzna o dowolnym, wprowadzonym polozeniu, ciecie otworu druga rura o dowolnych, wprowadzonych wymiarach i polozeniu.
#Pierwsza czesc makra stanowi deklaracja klasy i utworzenie obiektu w postaci okna sluzacego do wprowadzania danych rury, ciecia etc. z uzyciem biblioteki PySide [wiecej na temat: https://www.freecadweb.org/wiki/PySide_Medium_Examples]
#Druga czesc makra sluzy do generacji geometrii we FreeCadzie za pomoca skryptow w Pythonie [wiecej na temat: https://www.freecadweb.org/wiki/Python_scripting_tutorial].


###1. Tworzenia okna --- wprowadzanie danych###

#import modulow
from PySide import QtGui, QtCore

# Definicja klasy okna sluzacego do wprowadzania danych rury do ciecia, polozenia plaszczyzn ciecia, wymiarow rury sluzacej do ciecia otworow przelotowych i nieprzelotowych
class Window(QtGui.QDialog):
	""""""
	def __init__(self):
		super(Window, self).__init__()
		self.initUI()
	def initUI(self):
		self.result = userCancelled
		#tworzenie okna		xLoc,yLoc,xDim,yDim
		self.setGeometry(	250, 150, 600, 600)
		self.setWindowTitle("Generator geometrii") #nazwa
		self.setWindowFlags(QtCore.Qt.WindowStaysOnTopHint) #okno zawsze na wierzchu


		#~~Dane cietej rury~~

		self.label1 = QtGui.QLabel("1. Wprowadz dane cietej rury:", self) #napisy informujace jakie dane trzeba wprowadzic
		self.label1.move(20, 10) #polozenie napisu

		self.label2 = QtGui.QLabel("Srednica - D [mm]:", self)
		self.label2.move(20, 30)

		self.label3 = QtGui.QLabel("Dlugosc rury - L [mm]:", self)
		self.label3.move(20, 60)

		self.label4 = QtGui.QLabel("Grubosc scianki - a [mm]:", self)
		self.label4.move(20, 90)

		#Srednica zewnetrzna rury
		self.pipeDiameter = QtGui.QLineEdit(self) #okno do wprowadzania danych
		self.pipeDiameter.setInputMask("999") #maska-czyli jaka maksymalna wartosc moze byc wprowadzona
		self.pipeDiameter.setText("100") #domyslna wartosc w oknie
		self.pipeDiameter.setFixedWidth(50) #szerokosc okna
		self.pipeDiameter.move(250, 30) #polozenie okna

		#Dlugosc rury		
		self.pipeLength = QtGui.QLineEdit(self)
		self.pipeLength.setInputMask("9999")
		self.pipeLength.setText("400")
		self.pipeLength.setFixedWidth(50)
		self.pipeLength.move(250, 60)

		#grubosc rury
		self.pipeThickness = QtGui.QLineEdit(self)
		self.pipeThickness.setInputMask("999")
		self.pipeThickness.setText("2")
		self.pipeThickness.setFixedWidth(50)
		self.pipeThickness.move(250, 90)



		#~~Dane 2-ej rury sluzacej do ciecia~~

		self.checkbox1 = QtGui.QCheckBox("Ciecie druga rura", self) #stworzenie checkboxa, jesli jest zaznaczony to uwzglednione bedzie ciecie druga rura
		self.checkbox1.clicked.connect(self.onCheckbox1)#polaczenie checkboxa z funkcja, ktora moze wykonywac dowolna rzecz
		self.checkbox1.move(300, 140) #polozenie checkboxa

		self.checkbox1v2 = QtGui.QCheckBox("Ciecie przelotowe", self)#checkbox z cieciem przelotowym
		self.checkbox1v2.move(300, 160)
		
		self.label5 = QtGui.QLabel("2. Wprowadz dane drugiej rury:", self) #napisy informujace jakie dane wprowadzac
		self.label5.move(20, 140)

		self.label6 = QtGui.QLabel("Srednica - D [mm]:", self)
		self.label6.move(20, 170)

		self.label7 = QtGui.QLabel("Dlugosc rury - L [mm]:", self)
		self.label7.move(20, 200)

		self.label8 = QtGui.QLabel("Grubosc scianki - a [mm]:", self)
		self.label8.move(20, 230)


		self.label9 = QtGui.QLabel("Obrot rury wzgledem", self)
		self.label9.move(20, 260)

		self.label91 = QtGui.QLabel("osi (X,Y,Z) [stopnie]", self) #druga czesc napisu, bo nie zmiescil sie w calosci
		self.label91.move(20, 280)

		self.label101 = QtGui.QLabel("Punkt przecieca", self)
		self.label101.move(20, 310)

		self.label101 = QtGui.QLabel("osi rur (X,Y,Z) [mm]", self)
		self.label101.move(20, 330)

		#srednica 2-ej rury sluzacej do ciecia
		self.secondPipeDiameter = QtGui.QLineEdit(self)#okna do wprowadzania danych jw.
		self.secondPipeDiameter.setInputMask("999")
		self.secondPipeDiameter.setText("50")
		self.secondPipeDiameter.setFixedWidth(50)
		self.secondPipeDiameter.move(250, 170)
		
		#dlugosc 2-ej rury, najlepiej jak najdluzsza, zeby pierwsza rura byla przecieta w calosci
		self.secondPipeLength = QtGui.QLineEdit(self)
		self.secondPipeLength.setInputMask("9999")
		self.secondPipeLength.setText("400")
		self.secondPipeLength.setFixedWidth(50)
		self.secondPipeLength.move(250, 200)

		#grubosc drugiej ruru->bez znaczenie
		self.secondPipeThickness = QtGui.QLineEdit(self)
		self.secondPipeThickness.setInputMask("999")
		self.secondPipeThickness.setText("2")
		self.secondPipeThickness.setFixedWidth(50)
		self.secondPipeThickness.move(250, 230)

		#kat obrotu 2-ej rury wzgledem osi X
		self.secondPipeAngleX = QtGui.QLineEdit(self)
		self.secondPipeAngleX.setInputMask("999")
		self.secondPipeAngleX.setText("0")
		self.secondPipeAngleX.setFixedWidth(50)
		self.secondPipeAngleX.move(250, 260)

		#kat obrotu 2-ej rury wzglem osi Y
		self.secondPipeAngleY = QtGui.QLineEdit(self)
		self.secondPipeAngleY.setInputMask("999")
		self.secondPipeAngleY.setText("0")
		self.secondPipeAngleY.setFixedWidth(50)
		self.secondPipeAngleY.move(310, 260)

		#kat obrotu 2-ej rury wzgledem osi Z
		self.secondPipeAngleZ = QtGui.QLineEdit(self)
		self.secondPipeAngleZ.setInputMask("999")
		self.secondPipeAngleZ.setText("45")
		self.secondPipeAngleZ.setFixedWidth(50)
		self.secondPipeAngleZ.move(370, 260)
		
		#punkt bazowy X 2-ej rury 
		self.secondPipeX = QtGui.QLineEdit(self)
		self.secondPipeX.setInputMask("999")
		self.secondPipeX.setText("150")
		self.secondPipeX.setFixedWidth(50)
		self.secondPipeX.move(250, 310)

		#punkt bazowy Y 2-ej rury
		self.secondPipeY = QtGui.QLineEdit(self)
		self.secondPipeY.setInputMask("999")
		self.secondPipeY.setText("0")
		self.secondPipeY.setFixedWidth(50)
		self.secondPipeY.move(310, 310)

		#punkt bazowy Z 2-ej rury
		self.secondPipeZ = QtGui.QLineEdit(self)
		self.secondPipeZ.setInputMask("999")
		self.secondPipeZ.setText("0")
		self.secondPipeZ.setFixedWidth(50)
		self.secondPipeZ.move(360, 310)


		#~~Dane plaszczyzny ciecia~~

		self.checkbox2 = QtGui.QCheckBox("Ciecie plaszczyzna", self)#checkbox do uruchomienia ciecia plaszczyzna
		self.checkbox2.clicked.connect(self.onCheckbox2)
		self.checkbox2.move(300,360)

		self.label11 = QtGui.QLabel("3. Wprowadz dane plaszczyzny ciecie:", self)#informacje
		self.label11.move(20, 360)

		self.label12 = QtGui.QLabel("Pkt przecieca plaszczyzny", self)
		self.label12.move(20, 450)

		self.label121 = QtGui.QLabel("i osi rury (X,Y,Z) [mm]", self)
		self.label121.move(20, 470)

		self.label13 = QtGui.QLabel("Obrot plaszczyzny wzgl.", self)
		self.label13.move(20, 400)

		self.label131 = QtGui.QLabel("osi (X,Y,Z) [stopnie]", self)
		self.label131.move(20, 420)


		#kat obrotu plaszczyzny wzgledem osi X
		self.planeAngleX = QtGui.QLineEdit(self) #okno do wprowadzania danych
		self.planeAngleX.setInputMask("999")
		self.planeAngleX.setText("0")
		self.planeAngleX.setFixedWidth(50)
		self.planeAngleX.move(250, 400)

		#kat obrotu plaszczyzny wzgledem osi Y
		self.planeAngleY = QtGui.QLineEdit(self)
		self.planeAngleY.setInputMask("999")
		self.planeAngleY.setText("45")
		self.planeAngleY.setFixedWidth(50)
		self.planeAngleY.move(310, 400)

		#kat obrotu plaszczyzny wzgledem osi Z
		self.planeAngleZ = QtGui.QLineEdit(self)
		self.planeAngleZ.setInputMask("999")
		self.planeAngleZ.setText("0")
		self.planeAngleZ.setFixedWidth(50)
		self.planeAngleZ.move(370, 400)

		#pkt bazowy X plaszczyzny
		self.planeX = QtGui.QLineEdit(self)
		self.planeX.setInputMask("999")
		self.planeX.setText("350")
		self.planeX.setFixedWidth(50)
		self.planeX.move(250, 450)

		#pkt bazowy Y plaszczyzny
		self.planeY = QtGui.QLineEdit(self)
		self.planeY.setInputMask("999")
		self.planeY.setText("0")
		self.planeY.setFixedWidth(50)
		self.planeY.move(310, 450)

		#pkt bazowy Z plaszczyzny
		self.planeZ = QtGui.QLineEdit(self)
		self.planeZ.setInputMask("999")
		self.planeZ.setText("0")
		self.planeZ.setFixedWidth(50)
		self.planeZ.move(370, 450)


		#~~Wlasne operacje ciecia~~
		#tutaj nic to nie robi
		self.checkbox3 = QtGui.QCheckBox("Wlasne operacje CAD", self)
		self.checkbox3.clicked.connect(self.onCheckbox3)
		self.checkbox3.move(300,490)

		self.label14 = QtGui.QLabel("4. Wlasne operacje na modelu", self)
		self.label14.move(20, 490)

		# przycisk cancel
		cancelButton = QtGui.QPushButton('Cancel', self) #utworzenie przycisku
		cancelButton.clicked.connect(self.onCancel) #polaczenie z funkcja cancel
		cancelButton.setAutoDefault(True) #domyslne ustawienie
		cancelButton.move(260, 550) #polozenie
		# OK przycisk
		okButton = QtGui.QPushButton('OK', self)
		okButton.clicked.connect(self.onOk)
		okButton.move(150, 550)

		# pokazanie okna
		self.show()

	#Definicje funkcji, do ktorych odwoluja sie checkboxy i przyciski
	def onCheckbox1(self):#tu funkcje checkboxow nie pelnia zadnych potrzebnych funkcji, zostawiono je jesli program bedzie rozwijany
		return True

	def onCheckbox2(self):
		return True

	def onCheckbox3(self):
		return True

	#definicje funkcji wywolywanych po przycisnieciu przyciskow Cancel i OK
	def onCancel(self):
		self.result = userCancelled # ustawienie zmiennej result jako "Cancelled" - anulowanie dalszych operacjiS
		self.close() #zamkniecie okna
	def onOk(self):
		self.result = "OKEY" #zmienna jako OKEY -> dalsze operacje sie odbeda
		self.close() #zamkniecie okna 


userCancelled = "Cancelled"
userOK = "OK"

#2. Utworzenie obiektu - okna wprowadzania danych we FreeCadzie, po wprowadzeniu danych do okien i zatwierdzeniu OK, dane wpisane w okna sa wpisane do zmiennych, ktore sluza do generacji geometrii.

form = Window() #utworzenie obiektu okna
form.exec_() #wywolanie 

if form.result == userCancelled:
	pass #jesli anulowane nic nie robi

if (form.result == "OKEY"):	
	print("OK") #jesli OK to przypisanie danych z okien do zmienncyh

	pipeDiameter=int(form.pipeDiameter.text())
	pipeLength=int(form.pipeLength.text())
	pipeThickness=int(form.pipeThickness.text())
	
	secondPipeDiameter=int(form.secondPipeDiameter.text())
	secondPipeLength=int(form.secondPipeLength.text())
	secondPipeThickness=int(form.secondPipeThickness.text())
	secondPipeAngleX=int(form.secondPipeAngleX.text())
	secondPipeAngleY=int(form.secondPipeAngleY.text())
	secondPipeAngleZ=int(form.secondPipeAngleZ.text())
	secondPipeX=int(form.secondPipeX.text())
	secondPipeY=int(form.secondPipeY.text())
	secondPipeZ=int(form.secondPipeZ.text())

	planeAngleX=int(form.planeAngleX.text())
	planeAngleY=int(form.planeAngleY.text())
	planeAngleZ=int(form.planeAngleZ.text())
	planeX=int(form.planeX.text())
	planeY=int(form.planeY.text())
	planeZ=int(form.planeZ.text())




#3.~~~~~~~~~GENERACJA GEOMETRII~~~~~~~~~~~~~~~~

#import modulow FreeCada
import Part
import FreeCAD
from FreeCAD import Base

#utworzenie nowego "dokumentu" o nazwie "plasma_cutter", w ktorym bedzie tworzona geometria
doc = FreeCAD.newDocument("plasma_cutter")

#tworzenie glownej rury wzdluz osi X
pipe1Dir=Base.Vector(1,0,0) #wektor kierunku cylindra wzdluz X [X=1,Y=0,Z=0]
pipe1pnt=Base.Vector(0,0,0) #punkt poczatka cylindra (lezy na osi cylindra) (X=0,Y=0,Z=0)
pipe1angle=360 #kat cylindra 360-pelny cylinder
pipe1=Part.makeCylinder(pipeDiameter/2, pipeLength, pipe1pnt ,pipe1Dir, pipe1angle) #utworzenie cylindra(promien,dlugosc,pkt poczatkowy,kierunek,kat)

#robienie rury z pelnego preta o zadanej grubosci scianki
pipe1cutTool=Part.makeCylinder(pipeDiameter/2-pipeThickness, pipeLength, pipe1pnt ,pipe1Dir, pipe1angle) #cylinder pomocniczy o promieniu pomniejszonym o grubosc rury
pipe1 = pipe1.cut(pipe1cutTool) #wyciecie srodka z cylindra->stworzenie rury o zdanej grubosci

#Part.show(pipe1) #pokazuje dany obiekt w oknie FreeCada
Gui.ActiveDocument.ActiveView.setAxisCross(True) #ustawienie widoku


if form.checkbox1.isChecked():#jesli checkbox wyciecia druga rura jest zaznaczony
	#tworzenie drugiego cylindra, ktory sluzy do wyciecia otworu w rurze
	pipe2Dir=Base.Vector(1,0,0)
	pipe2pnt=Base.Vector(0,0,0)
	pipe2angle=360
	pipe2=Part.makeCylinder(secondPipeDiameter/2, secondPipeLength, pipe2pnt ,pipe2Dir, pipe2angle)

	#obrot drugiego cylindra wzgledem osi X, Y i Z
	pipe2.rotate(pipe2pnt,Base.Vector(1,0,0),secondPipeAngleX)
	pipe2.rotate(pipe2pnt,Base.Vector(0,1,0),secondPipeAngleY)
	pipe2.rotate(pipe2pnt,Base.Vector(0,0,1),secondPipeAngleZ)

	#przesuniecie poczatku drugiego cylindra do zadanych pkt-ow X,Y,Z
	pipe2.translate(Base.Vector(secondPipeX,secondPipeY,secondPipeZ))

	#cylinder bedacy lustrzanym odbiciem pipe2, ktory sluzy do wyciecia otworu z drugiej strony ruru, jesli bedzie zaznoczony checkbox cieci z 2 stron pipe2v2
	pipe2v2=Part.makeCylinder(secondPipeDiameter/2, secondPipeLength, pipe2pnt ,(-1)*pipe2Dir, pipe2angle)

	pipe2v2.rotate(pipe2pnt,Base.Vector(1,0,0),secondPipeAngleX)
	pipe2v2.rotate(pipe2pnt,Base.Vector(0,1,0),secondPipeAngleY)
	pipe2v2.rotate(pipe2pnt,Base.Vector(0,0,1),secondPipeAngleZ)

	pipe2v2.translate(Base.Vector(secondPipeX,secondPipeY,secondPipeZ))


	pipe1 = pipe1.cut(pipe2)#ciecie rury cylinder

	if form.checkbox1v2.isChecked(): #jesli checkbox ciecia przelotowego zaznaczony to ciecie 2-ego otworu
		#ciecie z 2 stron - przelotowo
		pipe1=pipe1.cut(pipe2v2)


if form.checkbox2.isChecked():#checkbox ciecia plaszczyzna
	#utworzenie plaszczyznay rownoleglej do YZ, X normalna, 10x wieksza wzdluz i wszerz nic srednica rury
	plane1=Part.makePlane(10*pipeDiameter,10*pipeDiameter,Base.Vector(0,-5*pipeDiameter,-5*pipeDiameter), Base.Vector(1,0,0),Base.Vector(0,1,0))
	#obrot plaszczyzny o zadany kat wzgledem osi X,Y i Z
	plane1.rotate(Base.Vector(0,0,0),Base.Vector(1,0,0),planeAngleX)
	plane1.rotate(Base.Vector(0,0,0),Base.Vector(0,1,0),planeAngleY)
	plane1.rotate(Base.Vector(0,0,0),Base.Vector(0,0,1),planeAngleZ)

	#przesuniecie plaszczyzny do pktu (X,Y,Z)
	plane1.translate(Base.Vector(planeX, planeY, planeZ))

	#Part.show(plane1)
	#App.ActiveDocument.ActiveObject.Label = "Plane1"

	#Utworzenie normalnej do plaszczyzny i wyciagniecie plaszczyzny wzdluz normalnej
	normalPlane1=plane1.normalAt(0,0)
	plane1=plane1.extrude(normalPlane1*pipeLength*4)
	#Part.show(plane1)
	
	#obciecie rury prostopadloscianem powstalem po wyciagnieciu plaszczyzny
	pipe1=pipe1.cut(plane1)

#pokazanie rury w oknie freecada
Part.show(pipe1)
#zmiana nazwy i ustawienie widoku
App.ActiveDocument.ActiveObject.Label = "Pipe1"
Gui.activeDocument().activeView().viewAxonometric()
Gui.SendMsgToActiveView("ViewFit")



############################################

#Utworzenie okna informujacego o koniecznosci wybrania krawedzi przed uruchomieniem makra do generacji FreeCada
from PySide import QtGui
QtGui.QMessageBox.information(None,"Wybierz krawedzie","Wybierz krawedzie, z ktorych bedzie generowany GCode. Kolejnosc wyboru krawedzi to kolejnosc ciecia. Uzyj CTRL do zaznaczenia wiekszej ilosci krawedzi. Po wybraniu krawedzi uruchom makro do generacji GCode.") 


