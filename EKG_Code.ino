// -------------------------------- //
// Variabeldeklarationen/Definition

// ! Messung
const int MessPin = 26;
volatile int EKG_Rohwert;

volatile int interruptCounter = 0;  // Zu jedem Interrupt soll ein Messwert in loop() gemessen werden. (Zeitlicher Abstand der Messwerte = konstant)
// "volatile", weil die Variabel im Interrupt geändert wird.

// Interrupt-Einstellungen für den ESP32
hw_timer_t *timer = NULL;      // timer object
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; // object to "lock" operation during/against interrupt





// ! Datenverarbeitung
// Kernel für den digitalen Tiefpassfilter (Savitzky Golay-Filter  für  51 Datenpunkte mit einem Polynom 8.ten Grades)
#define KernelLaenge 51
double Kernel[KernelLaenge] = {                    0.0200847, -0.0167373, -0.0214748, -0.011272, 0.00281464, 0.0143496, \
                                                   0.020287, 0.0199519, 0.0142123, 0.00482058, -0.00609625, -0.0164156, \
                                                   -0.0242861, -0.0282982, -0.0275757, -0.0218008, -0.0111881, \
                                                   0.00358084, 0.0214518, 0.0411087, 0.0610939, 0.0799288, 0.0962271, \
                                                   0.108796, 0.116722, 0.119429, 0.116722, 0.108796, 0.0962271, \
                                                   0.0799288, 0.0610939, 0.0411087, 0.0214518, 0.00358084, -0.0111881, \
                                                   -0.0218008, -0.0275757, -0.0282982, -0.0242861, -0.0164156, \
                                                   -0.00609625, 0.00482058, 0.0142123, 0.0199519, 0.020287, 0.0143496, \
                                                   0.00281464, -0.011272, -0.0214748, -0.0167373, 0.0200847
                              };

int Feld_EKGRohwerte[KernelLaenge];
int I_EKGRohwerte = 0;
double Sum;
int r = 0;
double  EKG_Endwert = 0;





// -------------------------------- //
// Funktionsdefinition

// Funktion, die während des Interrupts ausgeführt wird
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  EKG_Rohwert = analogRead(MessPin);
  portEXIT_CRITICAL_ISR(&timerMux);
}

// -------------------------------- //






void setup()
{
  Serial.begin(115200);

  timer = timerBegin(0, 8000, true); // Wie ist die zeitliche Distanz zwischen zwei Clock Ticks? (Prescalar 80 -> 1 µs pro Clock Tick, 8000 -> 100µs)
  timerAlarmWrite(timer, 25, true);  // Sampel-Rate:  Wieviele Ticks soll die Clock durchführen bis zum Interrupt ausführen? (Prescalar = 8000, Tickanzahl = 25, => 400 Hz)
  timerAttachInterrupt(timer, &onTimer, true);  // Welche Funktion soll beim Interrupt ausgeführt werden?
  timerAlarmEnable(timer);           // Timer Start
}






void loop()
{
  if (interruptCounter > 0)
  {
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
    if (interruptCounter > 0)
    {
      Serial.println("Timing problem!");
    }
    else
    {
      // FIR Filter
      Feld_EKGRohwerte[I_EKGRohwerte] = EKG_Rohwert;
      // I_EKGRohwerte gibt immer die Stelle des ältesten Messwertes im Feld an. Dieser ist immer eine Stelle nach dem neusten Messwert. Der I_EKGRohwerte rotiert durch das Feld.
      I_EKGRohwerte = (I_EKGRohwerte + 1) % (KernelLaenge - 1);
      Sum = 0;
      for (r = 0; r < KernelLaenge; r++)
      {
        // Begonnen von I_EKGRohwerte werden alle EKG-Rohwerte durchgegangen und mit einem Kernel-Wert jeweils multipliziert.
        Sum += Kernel[r] * Feld_EKGRohwerte[(I_EKGRohwerte + r) % (KernelLaenge - 1)];
      }
      EKG_Endwert = Sum;


      // Ausgabe im seriellen Monitor
      Serial.println(EKG_Endwert);

    }
  }
}
