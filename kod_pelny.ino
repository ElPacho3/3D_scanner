#include <Wire.h>
#include <VL53L1X.h>
#include <SPI.h>
#include <SD.h>
#include <Stepper.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


VL53L1X sensor;                               // Utworzenie obiektu czujnika odległości VL53L1X
Adafruit_SSD1306 display(128, 64, &Wire, -1); // Utworzenie obiektu wyświetlacza OLED
File scanFile;                                // Obiekt pliku wykorzystywany do zapisu danych skanu

Stepper motorTable(2048, 25, 27, 26, 14);     // Stół obrotow
Stepper motorZ(2048, 32, 4, 33, 2);           // Oś Z

#define SD_CS 5 
#define BUTTON_NEXT 16                        // Pin przycisku zmiany pozycji w menu
#define BUTTON_SELECT 17                      // Pin przycisku wyboru opcji menu

int menuIndex = 0;                            // Aktualnie wybrana pozycja menu

const int POINTS = 64;                        //ilość punktow pomiarowych
const int SAMPLES = 41;                       // ilość pomiarw na punkt
const float SENSOR_TO_CENTER = 113.0;         // Odległość od czujnika do osi stołu


float medianDistance(){
  int values[SAMPLES];

  for (int i = 0; i < SAMPLES; i++){
    values[i] = sensor.read(); 
    delay(25);
  }

//sortowanie 
  for (int i = 0; i < SAMPLES - 1; i++){
    for (int j = i + 1; j < SAMPLES; j++){
      if (values[j] < values[i]){
        int t = values[i];
        values[i] = values[j];
        values[j] = t;
      }
    }
  }
//średnia
return (values[18] + values[19] + values[20] + values[21] + values[22]) / 5.0;
}


void drawMenu(){                          // Funkcja wyświetlająca menu na ekranie OLED
  display.clearDisplay();                 // Wyczyść zawartość ekranu

  display.setTextSize(1);                 // Ustaw rozmiar tekstu na 1
  display.setTextColor(SSD1306_WHITE);    // Ustaw kolor tekstu na biały

  display.setCursor(0, 10);               // Ustaw pozycję kursora (x=0, y=10)

  if(menuIndex == 0){                     // Jeśli wybrana jest pierwsza pozycja menu
    display.println("> START SCAN");      // Wyświetl strzałkę przy START SCAN
    display.println("");                  // Pusta linia dla odstępu
    display.println("  CALIBRATION");     // Wyświetl CALIBRATION bez zaznaczenia
  }
  else{
    display.println("  START SCAN");      // Wyświetl START SCAN bez zaznaczenia
    display.println("");                  // Pusta linia dla odstępu
    display.println("> CALIBRATION");     // Wyświetl strzałkę przy CALIBRATION
  }

  display.display();                      // Wyślij przygotowany obraz na ekran OLED
}


void showScreen(String title, String value){      // Funkcja wyświetlająca komunikat na ekranie OLED
  display.clearDisplay();                         // Wyczyść ekran
  display.setCursor(0,0);                         // Ustaw kursor w lewym górnym rogu
  display.println(title);                         // Wyświetl tytuł komunikatu
  display.println();                              // Dodaj pustą linię
  display.println(value);                         // Wyświetl wartość lub dodatkową informację
  display.display();                              // Zaktualizuj ekran OLED
}




void calibrate(){                                 // Funkcja kalibracji wysokości skanera
while (true){
    float distance = medianDistance();            // Odczytaj przefiltrowaną odległość z czujnika
    showScreen("CALIBRATING", "DIST=" + String(distance));  // Wyświetl aktualną odległość na OLED

    Serial.print("Distance = ");
    Serial.println(distance);                     // Wyświetl zmierzoną odległość

    if (distance < 120){                          // wykrywa stół bliżej na odległości niż 120 mm
        Serial.println("TABLE FOUND");
        break;
    }

    motorZ.step(-4096);

    delay(500);
}
motorZ.step(4096);                                // Podnieś oś Z po zakończeniu kalibracji
Serial.println("CALIBRATION DONE");               

drawMenu();                                       //rysujem meni
}






void scanObject(){                                                // Funkcja wykonująca proces skanowania obiektu
  scanFile = SD.open("/pierwszy.txt", FILE_WRITE);                // Otwiera plik do zapisu danych

  if (!scanFile){                                                 // Zabiezpieczenie dla pliku
    Serial.println("FILE ERROR");
    drawMenu();
    return;
  }

  Serial.println("SCAN START");

  for (int layer = 0; layer < 37; layer++){                   // Pętla kolejnych warstw skanowania
    float z = layer * 3.0;                                    // Wysokość aktualnej warstwy [mm]
    float minDistance = 9999;                                 // Najmniejsza odległość w danej warstwie


    for (int point = 0; point < POINTS; point++){             // Pętla punktów pomiarowych
      float distance = medianDistance();                      // Odczytuje filtrowaną odległość

      showScreen("SCANNING...", "L:" + String(layer) + " D:" + String(distance));     // wyświetla status

      if(distance < minDistance){                             // Szukaj najmniejszej odległości
        minDistance = distance;
      }

      float angleDeg = point * (360.0 / POINTS);              // Kąt punktu w stopniach
      float angleRad = angleDeg * PI / 180.0;                 // Zamiana stopni na radiany

      float r = SENSOR_TO_CENTER - distance;

      float x = r * cos(angleRad);                            // Oblicz współrzędnych x i y
      float y = r * sin(angleRad);

      scanFile.print(x, 3);
      scanFile.print(" ");
      scanFile.print(y, 3);                                   //ich zapis do pliku
      scanFile.print(" ");
      scanFile.println(z, 3);

      int stepCount =
        round((point + 1) * 2048.0 / POINTS) -
        round(point * 2048.0 / POINTS);                       // Oblicz liczbę kroków silnika

      motorTable.step(stepCount);                             // Obracamy stół

      delay(100);
    }
    scanFile.flush();                                         // Zapisujemy dane z bufora na kartę SD

    scanFile.close();
    break;
    if(minDistance > 200){                                     // Sprawdzamy czy to jest koniec obiektu
      Serial.println("OBJECT END");
      break;
    }

    motorZ.step(4096);
    delay(400);
  }
  scanFile.close();
  Serial.println("SCAN FINISHED");
  Serial.println("Saved: pierwszy.txt");
  drawMenu();
}





void setup(){
  Serial.begin(115200);
  Wire.begin(21, 22);

//oled
if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){   // Inicjalizacja wyświetlacza OLED
  Serial.println("OLED ERROR");
  while(1);                                       // Zatrzymanie programu
}

display.clearDisplay();
display.display();                                // Wyczysciamy OLED i Wysyłamy pusty obraz na ekran

pinMode(BUTTON_NEXT, INPUT_PULLUP);               // Konfiguracja przycisków
pinMode(BUTTON_SELECT, INPUT_PULLUP);

  if (!sensor.init()){                            // Inicjalizacja czujnika VL53L1X
    Serial.println("VL53L1X ERROR");
    while (1);
  }

  sensor.setDistanceMode(VL53L1X::Short);         // Ustawienie trybu krótkiego zasięgu
  sensor.startContinuous(20);                     // Ciągły pomiar co 20 ms

if (!SD.begin(SD_CS)){                            // Inicjalizacja karty SD

  Serial.println("SD ERROR");
  showScreen("ERROR", "NO SD CARD");

  while(1);
}

  motorTable.setSpeed(8);                         // Ustawienie prędkości silników
  motorZ.setSpeed(8);
  drawMenu();
}

void loop(){
  if (digitalRead(BUTTON_NEXT) == LOW){         // Sprawdzamy czy naciśnięto przycisk "NEXT"
    menuIndex++;

    if(menuIndex > 1){
      menuIndex = 0;
    }
    drawMenu();

    Serial.print("MENU = ");
    Serial.println(menuIndex);

    delay(150);
  }

  if (digitalRead(BUTTON_SELECT) == LOW){      // Sprawdzamy czy naciśnięto przycisk "Select"
    if(menuIndex == 0){
      scanObject();
    }

    if(menuIndex == 1){
      calibrate();
    }

    delay(150);
  }
}





