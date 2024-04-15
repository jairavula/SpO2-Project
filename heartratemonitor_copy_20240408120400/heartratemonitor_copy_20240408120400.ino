#include <SPI.h> // include SPI library
#include <Adafruit_GFX.h> // lcd display library
#include <Adafruit_ILI9341.h> // lcd display library
#include <Arduino.h> // arduino header file

// Initialization of Chip Select, Reset, and Master Input Slave Output
#define TFT_CS 10 // chip select
#define TFT_RST 8 // reset
#define TFT_DC 9 // master input slave output
//Initializes the display as variable tft
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// Initializes signal input pin, and variables for frequency/BPM computation
// Initializes the plot dimensions:
const int graphHeight = 120; // graph height
const int graphWidth = 240; // graph width
const int graphX = 0; // graph x value
const int graphY = 100; // graph y value
int previousY = graphY + graphHeight / 2; // previous y value

void setup() {
  // Sets up serial monitor,
  // initializes the screen and orientation,
  // and draws the plot outline
  Serial.begin(9600); // initialize serial monitor connection
  tft.begin(); // intialize LCD display connection
  tft.setRotation(4); // set orientation
  tft.fillScreen(ILI9341_BLACK); // clear display 
  tft.drawRect(graphX, graphY, graphWidth, graphHeight, ILI9341_WHITE); // initialize rectangle for drawing waveform into

  // initalizes bottom text that interprets heartrate
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK); // sets forground color to green and background color to black for printing text
  tft.setTextSize(2); // set text size to 2
  tft.setCursor(10, 240); // set cursor location for pritning
  tft.print("Your Heart Rate is: "); // print your heart rate is:

  // Initializes text and color
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // sets background to black and foreground to white
  tft.setTextSize(3); // set text size to three
  // Displays "BPM:
  tft.setCursor(10, 10); // set location of cursor for printing waveform
  tft.print("BPM: "); // print BPM

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // set foreground color to white and background color to black for printing
  tft.setTextSize(3); // set text size to three
  // Displays "BPM:
  tft.setCursor(120, 10); // set cursor for printing
  tft.print("SpO2: "); // print spo2
}
void loop() {
  // Calls the functions for computing the frequency
  // and displaying the waveform on the plot
  displayWaveform(); // display the waveform
  handleHeartbeatDetection(); // handle the heartbeat detection
  computeSpO2(); // compute the SPO2 value
}

const int signalPin = A0;           // Analog input pin that reads in the heartbeat wave
const int numPeaksToConsider = 5;   // Number of peaks to calculate average BPM
int peakTimes[numPeaksToConsider];  // Store times of peaks
int peakIndex = 0;  // indexes the peaktimes array
int lastReading = 0;  // holds the las treading
unsigned long lastPeakTime = 0; // holds the last peak time
float minimumPeakHeight = 50;       // Minimum change required to consider a peak
unsigned long debounceTime = 1000;  // Minimum time between peaks

void handleHeartbeatDetection() {
  static int maxReading = 0;                // Track the maximum reading since last peak
  static unsigned long maxReadingTime = 0;  // Time at which the maximum reading occurred

  int reading = analogRead(signalPin); // reads the value from the pin
  unsigned long currentTime = millis(); // holds the current time using the millis() function

  // Only consider a new peak if enough time has elapsed to debounce
  if (currentTime - lastPeakTime > debounceTime) {
    if (reading > maxReading) { // if current reading is greater than the max reading
      maxReading = reading; // update the max reading with the new one
      maxReadingTime = currentTime; // update max reading time with new time
    }

    // Check if the current reading drops significantly from the max
    if (maxReading - reading > minimumPeakHeight) {
      peakTimes[peakIndex % numPeaksToConsider] = maxReadingTime; // update peakTimes table with new peak value
      peakIndex++; // increment peakIndex
      lastPeakTime = maxReadingTime; // update the last peak time with new max peak

      // Reset maximum after a peak is detected
      maxReading = reading;

      if (peakIndex >= numPeaksToConsider) { // if peak index is greater tha nthe number of peaks to consider then calculate bpm
        calculateAverageBPM(); // calculate bpm
        peakIndex = 0;  // Reset index after calculation
      }
    }
  }
}

// calculates average BPM 
void calculateAverageBPM() {
  unsigned long totalInterval = 0; // initialize totalInterval to 0
  for (int i = 1; i < numPeaksToConsider; i++) { // calculates average
    totalInterval += peakTimes[i] - peakTimes[i - 1]; // calculates average
  }
  float averageInterval = (float)totalInterval / (numPeaksToConsider - 1); // calculates average time interval
  float averageBPM = 60000.0 / averageInterval;  // Convert ms to BPM

  Serial.print("Average BPM: "); // prints bpm to serial monitor for debugging
  Serial.println(averageBPM); // prints bpm value
  displayBPM(static_cast<int>(averageBPM)); // prints bpm information to LCD display
  displayHeartRateInfo(static_cast<int>(averageBPM)); // displays if the heartrate is high low or normal
}

//Displays the BPM at the top of the screen
void displayBPM(int bpm) {
  // Displays the BPM reading in large green text
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK); // sets foreground to green and background to black for text
  tft.setTextSize(5); // set text size to five
  tft.fillRect(30, 40, 90, 40, ILI9341_BLACK); // clear display by drawing black rectangle
  tft.setCursor(30, 40); // reset cursor for printing waveform
  tft.println(bpm, 1); // print bpm to display
}

// prints the SPO2 information
void displaySpO2(int SpO2) {
  if (SpO2 < 100 && SpO2 > 94){ // if spo2 value is valid then print text to LCD screen
  // Displays the SpO2 reading in large blue text
  tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK); // set foreground color to white and bckground color to black for printing
  tft.setTextSize(5); // set text size to five
  tft.fillRect(160, 40, 90, 40, ILI9341_BLACK); // clear display by drawing rectangle over it
  tft.setCursor(160, 40); // set cursor for printing
  tft.println(SpO2, 1); // print spo2
  }
}

// This function plots the waveform on the graph
void displayWaveform() {
  static unsigned long previousMillis = 0;  // Stores the last time the waveform was updated
  const long interval = 20;                 // Interval at which to refresh the waveform (milliseconds)

  unsigned long currentMillis = millis(); // get current time in milliseconds
  if (currentMillis - previousMillis >= interval) { // uf current time - previous time is greater than sampling interval display the waveform
    // Save the last update time
    previousMillis = currentMillis;

    // Initializes x value, reads the analog input, and scales the y value based on the signal reading
    static int x = 0;
    static int previousY = 0;  // Ensure this variable is static or global to retain its value between calls
    int readValue = analogRead(signalPin); // read signal input
    int y = map(readValue, 0, 1023, graphY + graphHeight, graphY); // set y value
    // Draws the (x, y) point on the graph
    tft.drawPixel(x + graphX, y, ILI9341_WHITE);
    // Connects two readings with a line 4 pixels wide, to construct the waveform, skips first point
    if (x > 0) {
      tft.drawLine(x + graphX - 1, previousY, x + graphX, y, ILI9341_GREEN); // Connects two readings with a line 4 pixels wide, to construct the waveform, skips first point
      tft.drawLine(x + graphX - 2, previousY, x + graphX, y, ILI9341_GREEN); // Connects two readings with a line 4 pixels wide, to construct the waveform, skips first point
      tft.drawLine(x + graphX - 3, previousY, x + graphX, y, ILI9341_GREEN); // Connects two readings with a line 4 pixels wide, to construct the waveform, skips first point
      tft.drawLine(x + graphX - 4, previousY, x + graphX, y, ILI9341_GREEN); // Connects two readings with a line 4 pixels wide, to construct the waveform, skips first point
    }
    // Stores the y coordinate and increments x coordinate
    previousY = y; 
    x++; // increment x value
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
int state = 0; // initialize switch statement value
int acRedAmplitude = 0; // initialize AC amplitude for RED led
int acIrAmplitude = 0; // intialize AC amplitude for IR led
const int redACPin = 0; // Red LED AC analog input
const int irACPin = 1;  // IR LED AC analog input
const int redDCPin = 2; // Red LED DC analog input
const int irDCPin = 3;  // IR LED DC analog input

// function that finds the amplitude for both leds
void measureAmplitude() {
  // ac amplitude FSM
  switch (state) {
    case 0:  // Reading peak from wave 1
      acRedAmplitude = detectPeak(redACPin); // get peak value
      state = 1; // go to next state
      break;
    case 1:  // Reading peak from wave 2
      acIrAmplitude = detectPeak(irACPin); // get peak value
      state = 0; // restart state to 0
      break;
  }
}

// Function to detect peak value from the input pin
int detectPeak(int pin) {
  static int peak = 0; // initialize local peak value
  static unsigned long startTime = 0; // initialize start time to 0
  unsigned long currentTime = millis(); // initialize current time to current time

  if (currentTime - startTime >= 100) { // if current time - start time is greater than 100 sample the next potential peak
    int val = analogRead(pin); // get current value of waveform
    if (val > peak) { // if value is greater than previous peak update the new peak
      peak = val;
    } else {  
      peak = max(peak - 1, 0); // adjust peak gradually if the current value is smaller
    }
    startTime = currentTime; // reset the start time to new current time
  }
  // return the peak value
  return peak;
}

// Function to compute SpO2
void computeSpO2() {
  static int timer = 0; // initialize timer value to 0
  measureAmplitude();  // calculate red ac amplitude and ir ac amplitude
  float dcRed = analogRead(redDCPin); // get dc red led value
  float dcIr = analogRead(irDCPin); // get dc ir led value

  float R = (acRedAmplitude / dcRed) / (acIrAmplitude / dcIr);  // calculate R ratio
  float SpO2 = 110 - 25 * R;  // Calculate SpO2 from R
  if (timer > 10000){ // only display every so often
  displaySpO2(static_cast<int>(SpO2)); // display spo2 value
  timer=0; // reset timer value to 0
  }
  timer++; // increment timer value
}

// Displays info about heart rate based on BPM reading
void displayHeartRateInfo(int bpm) {
  //Display "VERY LOW" If BPM reading is below 45
  if (bpm < 45) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK); // clear display by drawing rectangle over
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK); // set foreground color to red and background color to black for printing
    tft.setTextSize(4); // set text size to 4
    tft.setCursor(20, 260); // set cursor for printing
    tft.print("VERY LOW"); // print very low
  }
  // Display "LOW" If BPM reading is between 45 and 55
  if (bpm > 45 && bpm < 55) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK); // clear display by drawing rectangle over
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // set foreground color to white and background color to black for printing text
    tft.setTextSize(4); // set text size to 4
    tft.setCursor(20, 260); // set cursor for printing
    tft.print("LOW"); // print lowf
  }
  // Display "NORMAL" If BPM reading is between 55 and 80
  if (bpm > 55 && bpm < 80) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK); // clear display by drawing rectangle over
    tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK); // set foreground color to cyan and background color to black for printing text
    tft.setTextSize(4); // set text size to four
    tft.setCursor(20, 260); // set cursor for printing
    tft.print("NORMAL"); // print normal
  }
  // Display "ELEVATED" If BPM reading is between 80 and 120
  if (bpm > 80 && bpm < 120) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK); // clear display by drawing rectangle over
    tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK); // set foreground color to orange and background color to black for printing text
    tft.setTextSize(4); // set text size to four
    tft.setCursor(20, 260); // set cursor for printing
    tft.print("ELEVATED"); // print elevated
  }
  // Display "HIGH" If BPM reading is over 120
  if (bpm > 120) {
    tft.fillRect(0, 260, 260, 40, ILI9341_BLACK); // clear display by drawing rectangle over
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK); // set foreground color to red and background color to black for printing text
    tft.setTextSize(4); // set text size to four
    tft.setCursor(20, 260); // set cursor for printing
    tft.print("HIGH"); // print high
  }
}
