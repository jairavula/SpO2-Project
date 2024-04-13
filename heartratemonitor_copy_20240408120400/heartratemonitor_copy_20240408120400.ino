#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
// Initialization of Chip Select, Reset, and Master Input Slave Output
#define TFT_CS 10
#define TFT_RST 8
#define TFT_DC 9
//Initializes the display as variable tft
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// Initializes signal input pin, and variables for frequency/BPM computation
const int signalPin = A0;
unsigned long firstPeakTime = 0;
bool firstPeak = false;
// Initializes the plot dimensions
const int graphHeight = 120;
const int graphWidth = 240;
const int graphX = 0;
const int graphY = 100;
//Ensures
int previousY = graphY + graphHeight / 2;
void setup() {
  // Sets up serial monitor,
  // initializes the screen and orientation,
  // and draws the plot outline
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(4);
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRect(graphX, graphY, graphWidth, graphHeight, ILI9341_WHITE);
}
void loop() {
  // Calls the functions for computing the frequency
  // and displaying the waveform on the plot
  computeFrequency();
  displayWaveform();
}

const unsigned long measureInterval = 1000000; // Measuring interval in microseconds (e.g., 1 second)
const unsigned long debounceTime = 200000; // Minimum time between peaks (200ms)
static unsigned long lastMeasurementStartTime = 0;
static unsigned long lastPeakTime = 0;
static int highestValue = 0;
static unsigned long highestValueTime = 0;

void computeFrequency() {
  static enum { FINDING_FIRST_PEAK, FINDING_SECOND_PEAK } state = FINDING_FIRST_PEAK;
  int signalReading = analogRead(signalPin); // Current signal reading
  unsigned long currentTime = micros(); // Current time in microseconds

  // Start of a new measurement interval
  if (currentTime - lastMeasurementStartTime >= measureInterval) {
    if (state == FINDING_FIRST_PEAK) {
      lastPeakTime = highestValueTime; // Store the time of the first peak
      state = FINDING_SECOND_PEAK; // Change state to find the second peak
    } else if (state == FINDING_SECOND_PEAK) {
      unsigned long period = highestValueTime - lastPeakTime; // Compute the period in microseconds
      if (period > 0) {
        float seconds = period * 1e-6; // Convert period to seconds
        int bpm = static_cast<int>(60.0 / seconds); // Calculate BPM
        if (bpm > 30 && bpm < 180) { // Filter unrealistic BPM values
          Serial.print("BPM: ");
          Serial.println(bpm);
          displayBPM(bpm);
          displaySpO2(bpm); // Assuming SpO2 calculation is related
          displayHeartRateInfo(bpm);
        }
      }
      state = FINDING_FIRST_PEAK; // Reset state to find the next first peak
    }
    // Reset for next interval
    lastMeasurementStartTime = currentTime;
    highestValue = 0;
    highestValueTime = 0;
  }

  // Find the highest value in the current interval
  if (signalReading > highestValue) {
    highestValue = signalReading;
    highestValueTime = currentTime;
  }
}
//Displays the BPM at the top of the screen
void displayBPM(int bpm) {
  // Initializes text and color
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(3);
  // Displays "BPM:
  tft.setCursor(10, 10);
  tft.print("BPM: ");
  // Displays the BPM reading in large green text
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(5);
  tft.fillRect(30, 40, 90, 40, ILI9341_BLACK);
  tft.setCursor(30, 40);
  tft.println(bpm, 1);
}

void displaySpO2(int SpO2) {
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(3);
  // Displays "BPM:
  tft.setCursor(120, 10);
  tft.print("SpO2: ");
  // Displays the BPM reading in large green text
  tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
  tft.setTextSize(5);
  tft.fillRect(160, 40, 90, 40, ILI9341_BLACK);
  tft.setCursor(160, 40);
  tft.println(96, 1);
}
// This function plots the waveform on the graph
void displayWaveform() {
  // Initializes x value, reads the analog input, and scales the y value based on the signal reading
  static int x = 0;
  int readValue = analogRead(signalPin);
  int y = map(readValue, 0, 1023, graphY + graphHeight, graphY);
  // Draws the (x,y) point on the graph
  tft.drawPixel(x + graphX, y, ILI9341_WHITE);
  //Connects two readings with a line 4 pixels wide, to construct the waveform, skips first point
  if (x > 0) {
    tft.drawLine(x + graphX - 1, previousY, x + graphX, y, ILI9341_GREEN);
    tft.drawLine(x + graphX - 2, previousY, x + graphX, y, ILI9341_GREEN);
    tft.drawLine(x + graphX - 3, previousY, x + graphX, y, ILI9341_GREEN);
    tft.drawLine(x + graphX - 4, previousY, x + graphX, y, ILI9341_GREEN);
  }
  // Stores the y coordinate and increments x coordinate
  previousY = y;
  x++;
  // If wave reaches the end of the graph, reset the graph
  if (x >= graphWidth) {
    // Set x coordinate back to left side of screen
    x = 0;
    //Clears the graph by drawing a black rectangle over it
    tft.fillRect(graphX + 1, graphY + 1, graphWidth - 2, graphHeight - 2, ILI9341_BLACK);
  }
  // Controls speed of plotting
  delay(20);
}
// Displays info about heart rate based on BPM reading
void displayHeartRateInfo(int bpm) {
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 240);
  tft.print("Your Heart Rate is: ");
  //Display "VERY LOW" If BPM reading is below 45
  if (bpm < 45) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.setTextSize(4);
    tft.setCursor(20, 260);
    tft.print("VERY LOW");
  }
  // Display "LOW" If BPM reading is between 45 and 55
  if (bpm > 45 && bpm < 55) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(4);
    tft.setCursor(20, 260);
    tft.print("LOW");
  }
  // Display "NORMAL" If BPM reading is between 55 and 80
  if (bpm > 55 && bpm < 80) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
    tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
    tft.setTextSize(4);
    tft.setCursor(20, 260);
    tft.print("NORMAL");
  }
  // Display "ELEVATED" If BPM reading is between 80 and 120
  if (bpm > 80 && bpm < 120) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
    tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
    tft.setTextSize(4);
    tft.setCursor(20, 260);
    tft.print("ELEVATED");
  }
  // Display "HIGH" If BPM reading is over 120
  if (bpm > 120) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.setTextSize(4);
    tft.setCursor(20, 260);
    tft.print("HIGH");
  }
}



const int redACPin = 0; // Red LED AC analog input
const int irACPin = 1;  // IR LED AC analog input
const int redDCPin = 2; // Red LED DC analog input
const int irDCPin = 3;  // IR LED DC analog input

// Function to calculate the amplitude of the AC signal
float calculateACAmplitude(int acPin) {
  unsigned long startTime = millis(); // initialize timer
  int peak = 0; // set baseline peak (to be updated)
  int trough = 1023; // set baseline trough (to be updated)
  // Measure for a short period to find peak and trough
  while (millis() - startTime < 1000) { // find peak and trough in 1 second measurement
    int reading = analogRead(acPin); // read an AC signal
    if (reading > peak) {
      peak = reading; // set peak to reading value if it is the highest so far
    }
    if (reading < trough) {
      trough = reading; // set trough to reading value if it is the lowest so far
    }
  }
  return (peak - trough) / 2.0; // Return the amplitude
}

// Function to compute SpO2
float computeSpO2() {
  float acRedAmplitude = calculateACAmplitude(redACPin); // calculate red ac amplitude
  float acIrAmplitude = calculateACAmplitude(irACPin); // calculate ir ac amplitude
  float dcRed = analogRead(redDCPin);
  float dcIr = analogRead(irDCPin);

  float R = (acRedAmplitude / dcRed) / (acIrAmplitude / dcIr); // calculate R ratio
  float SpO2 = 110 - 25 * R; // Calculate SpO2 from R
  return SpO2;
}

