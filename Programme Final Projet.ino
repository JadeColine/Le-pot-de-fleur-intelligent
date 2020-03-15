

#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

#include <Adafruit_NeoPixel.h>

#include <SoftwareSerial.h>                          
SoftwareSerial HC06(2,3); 

//capteur niveau d'eau
const byte PIN_CAPTEUR=8; // Déclaration de la broche du capteur
String capteur;

//capteur humidité 
int PinSensor = A1;
int ValeurSensor = 0;
int pourcentage = 0;

//pompe
int TauxHumiditeMin=400;
int ENA=9; //Connecté à Arduino pin 9(sortie PWM)
int IN1=4; //Connecté à Arduino pin 4
int IN2=5; //Connecté à Arduino pin 5

//luminosité
// Parameter 32 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
int X=12;


Adafruit_NeoPixel strip = Adafruit_NeoPixel(X, 6, NEO_GRB + NEO_KHZ800);

int LuminositeS=100;
int valeurTot=0;
int LuminositeMoyenne=0;

bool releveJournalier = false;
bool releveMinute = false;
int minuteCounter = 0;
int lumiereCounter = 0;

tmElements_t tm;


void setup() {
  Serial.begin(9600);
  HC06.begin(9600);

  //capteur niveau d'eau
  pinMode(PIN_CAPTEUR,INPUT_PULLUP);

  //pompe
  pinMode(ENA,OUTPUT);
  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  digitalWrite(ENA,LOW); 
  digitalWrite(IN1,LOW); 
  digitalWrite(IN2,HIGH);

  //LED 
  strip.begin();
  strip.show(); // Initialise toute les led à 'off'

}

void loop() {

  //configuration du module RTC
  if (RTC.read(tm)) {
    Serial.print("Ok, Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.println();
  } 
  else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } 
    else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }
    delay(9000);
  } 

  releveLumiere();

  calculMoyenne();
      
  if (LuminositeMoyenne>LuminositeS) {
    Serial.println("Taux d'éclairage correct, allumage d'ambiance");

    if (lumiereCounter==3) {
      lumiereCounter = 0;
    }

    if (lumiereCounter==0) {                                    // On fait une boucle pour définir la couleur de chaque led
      for(int i = 0; i < X; i++ ) strip.setPixelColor(i, 223, 0, 0);  // setPixelColor(n° de led, Rouge, Vert, Bleu)
    }

    else if (lumiereCounter==1) {                               // On fait une boucle pour définir la couleur de chaque led
      for(int i = 0; i < X; i++ ) strip.setPixelColor(i, 0, 223, 0);  // setPixelColor(n° de led, Rouge, Vert, Bleu)  
    }

    else if (lumiereCounter==2) {                               // On fait une boucle pour définir la couleur de chaque led
      for(int i = 0; i < X; i++ ) strip.setPixelColor(i, 0, 0, 223);  // setPixelColor(n° de led, Rouge, Vert, Bleu)  
    }

    lumiereCounter++;

    strip.show(); // on affiche
  }
      
  else {
    Serial.println("Taux d'éclairage incorrect, allumage des LED blanches");
    for(int i = 0; i < X; i++ ) strip.setPixelColor(i, random(0, 223), random(0, 223), random(0, 223));
                                  // On fait une boucle pour définir la couleur de chaque led
                                  // setPixelColor(n° de led, Rouge, Vert, Bleu)

    strip.show(); // on affiche 
 }    

  //capteur luminosité 
  int valLum=analogRead(0);

  //capteur humidité
  ValeurSensor = analogRead(PinSensor);

  //pompe 
  if (TauxHumiditeMin<ValeurSensor){
    Serial.println("Taux d'humidité correct, votre plante n'a pas besoin d'être arrosée.");
  }
  else {
    Serial.println("Taux d'humidité incorrect, arrosage automatique lancé.");
    analogWrite(ENA,255);
    delay(1000);
    analogWrite(ENA,0);
  }
  
  //capteur de niveau d'eau
  if (digitalRead(PIN_CAPTEUR)==LOW) { //si le pin du capteur est LOW, flotteur bas
    capteur="capteur bas";
  }
  else { //sinon, le flotteur est en haut
    capteur="capteur haut";
  }
 

  HC06.print(ValeurSensor);
  HC06.print("x");
  HC06.print(valLum);
  HC06.print("x");
  HC06.print(capteur); 
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}

void calculMoyenne(){
  if((tm.Hour==13)&&(tm.Minute==43)&&not(releveJournalier)){
    LuminositeMoyenne=valeurTot/minuteCounter;
    releveJournalier = true;
    Serial.print("La valeur moyenne de la luminosité cette journée est de : ");
    Serial.println(LuminositeMoyenne);
  }
  if (tm.Hour==19 && tm.Minute==55 && releveJournalier) {
    releveJournalier = false;
    valeurTot=0;
  }
}

void releveLumiere() {
  if(tm.Second<=10 && not(releveMinute)){
    releveMinute = true;
    Serial.println("Relevé effectué");
    minuteCounter+=1;
    valeurTot+=analogRead(0);
  }
  if (tm.Second>=50 && releveMinute) releveMinute = false; 
}
