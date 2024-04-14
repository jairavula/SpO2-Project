#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>

// Initialization of Chip Select, Reset, and Master Input Slave Output
#define TFT_CS 10
#define TFT_RST 8
#define TFT_DC 9
//Initializes the display as variable tft
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// Initializes signal input pin, and variables for frequency/BPM computation
// Initializes the plot dimensions
const int graphHeight = 120;
const int graphWidth = 240;
const int graphX = 0;
const int graphY = 100;
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
  displayWaveform();
  handleHeartbeatDetection();
  computeSpO2();
}



const int signalPin = A0;           // Analog input pin
const int numPeaksToConsider = 5;   // Number of peaks to calculate average BPM
int peakTimes[numPeaksToConsider];  // Store times of peaks
int peakIndex = 0;
int lastReading = 0;
unsigned long lastPeakTime = 0;
float minimumPeakHeight = 50;       // Minimum change required to consider a peak
unsigned long debounceTime = 1000;  // Minimum time between peaks



void handleHeartbeatDetection() {
  static int maxReading = 0;                // Track the maximum reading since last peak
  static unsigned long maxReadingTime = 0;  // Time at which the maximum reading occurred

  int reading = analogRead(signalPin);
  unsigned long currentTime = millis();

  // Only consider a new peak if enough time has elapsed to debounce
  if (currentTime - lastPeakTime > debounceTime) {
    if (reading > maxReading) {
      maxReading = reading;
      maxReadingTime = currentTime;
    }

    // Check if the current reading drops significantly from the max
    if (maxReading - reading > minimumPeakHeight) {
      peakTimes[peakIndex % numPeaksToConsider] = maxReadingTime;
      peakIndex++;
      lastPeakTime = maxReadingTime;

      // Reset maximum after a peak is detected
      maxReading = reading;

      if (peakIndex >= numPeaksToConsider) {
        calculateAverageBPM();
        peakIndex = 0;  // Reset index after calculation
      }
    }
  }
}

void calculateAverageBPM() {
  unsigned long totalInterval = 0;
  for (int i = 1; i < numPeaksToConsider; i++) {
    totalInterval += peakTimes[i] - peakTimes[i - 1];
  }
  float averageInterval = (float)totalInterval / (numPeaksToConsider - 1);
  float averageBPM = 60000.0 / averageInterval;  // Convert ms to BPM

  Serial.print("Average BPM: ");
  Serial.println(averageBPM);
  displayBPM(static_cast<int>(averageBPM));
  displayHeartRateInfo(static_cast<int>(averageBPM));
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
  if (SpO2 < 100 && SpO2 > 94){
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
  tft.println(SpO2, 1);
  }
 
}










// This function plots the waveform on the graph
void displayWaveform() {
  static unsigned long previousMillis = 0;  // Stores the last time the waveform was updated
  const long interval = 20;                 // Interval at which to refresh the waveform (milliseconds)

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Save the last update time
    previousMillis = currentMillis;

    // Initializes x value, reads the analog input, and scales the y value based on the signal reading
    static int x = 0;
    static int previousY = 0;  // Ensure this variable is static or global to retain its value between calls
    int readValue = analogRead(signalPin);
    int y = map(readValue, 0, 1023, graphY + graphHeight, graphY);
    // Draws the (x, y) point on the graph
    tft.drawPixel(x + graphX, y, ILI9341_WHITE);
    // Connects two readings with a line 4 pixels wide, to construct the waveform, skips first point
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
      // Clears the graph by drawing a black rectangle over it
      tft.fillRect(graphX + 1, graphY + 1, graphWidth - 2, graphHeight - 2, ILI9341_BLACK);
    }
  }
}



const int threshold = 50;  // Threshold for peak detection
int state = 0;
int acRedAmplitude = 0;
int acIrAmplitude = 0;

const int redACPin = 0; // Red LED AC analog input
const int irACPin = 1;  // IR LED AC analog input
const int redDCPin = 2; // Red LED DC analog input
const int irDCPin = 3;  // IR LED DC analog input






void measureAmplitude() {
  switch (state) {
    case 0:  // Reading peak from wave 1
      acRedAmplitude = detectPeak(redACPin);
      state = 1;
      break;
    case 1:  // Reading peak from wave 2
      acIrAmplitude = detectPeak(irACPin);
      state = 2;
      break;
    case 2:  // Check if peaks are close in value

      state = 0;
      break;
  }
}

// Function to detect peak value from the input pin
int detectPeak(int pin) {
  static int peak = 0;
  static unsigned long startTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - startTime >= 100) {  // Adjust the sampling rate here
    int val = analogRead(pin);
    if (val > peak) {
      peak = val;
    } else {  // Decrease peak gradually if the current value is smaller
      peak = max(peak - 1, 0);
    }
    startTime = currentTime;
  }

  return peak;
}

// Function to compute SpO2
void computeSpO2() {
  static int x = 0;
  measureAmplitude();  // calculate red ac amplitude
  float dcRed = analogRead(redDCPin);
  float dcIr = analogRead(irDCPin);

  float R = (acRedAmplitude / dcRed) / (acIrAmplitude / dcIr);  // calculate R ratio
  float SpO2 = 110 - 25 * R;                                    // Calculate SpO2 from R
  if (x > 10000){
  displaySpO2(static_cast<int>(SpO2));
  x=0;
  }
  x++;
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
