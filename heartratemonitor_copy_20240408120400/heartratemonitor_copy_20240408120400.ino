#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Initialization of Chip Select, Reset, and Master Input Slave Output
#define TFT_CS 10
#define TFT_RST 8
#define TFT_DC 9

//Initializes the display as variable tft
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Initializes singal input pin, and variables for frequency/BPM computation
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
  pinMode(2, OUTPUT);
}

void loop() {
  // Calls the functions for computing the frequency
  // and displaying the waveform on the plot
  computeFrequency();
  displayWaveform();
 togglePinNonBlocking();

}

void computeFrequency() {
  // Intializes variables for the last signal measurement and 
  // the slope of the line between the current signal reading and the last
  static int lastSignalReading = 0; 
  static int lastDerivative = 0;

  // Ensures computation is not performed on readings that are too close together  
  const unsigned long debounceTime = 200000; 

  // Reads the signal through the analog input and computes the slope
  // between the current reading and the last reading
  int signalReading = analogRead(signalPin);
  int derivative = signalReading - lastSignalReading;
  
  // Variable to store timestamp
  unsigned long currentTime = micros();

  // Check if the derivative has just changed from positive to negative
  if (lastDerivative >= 0 && derivative < 0) {

    //Only accept the peak if enough time has passed since the last peak, 
    //or if it is the first peak of the program
    if (!firstPeak || (currentTime - firstPeakTime > debounceTime)) {
      // If first peak has already occured
      if (firstPeak) {
        //Compute period by taking time between two peaks in seconds
        float period = (currentTime - firstPeakTime) * 0.000001f;
        int bpm = 60 / period; // Calculating BPM
        //Only display BPM on screen if reading is good
        if (bpm) {
          Serial.print("BPM: ");
          Serial.println(bpm);
          displayBPM(bpm);
          displayHeartRateInfo(bpm);
        }
        // Resetting for the next peak measurement
        firstPeakTime = currentTime; 
      } else {
        //Create the first peak
        firstPeak = true;
        firstPeakTime = currentTime;
      }
    }
  }
  // Store the derivative and signal reading in lastX variables for iterative computation
  lastSignalReading = signalReading;
  lastDerivative = derivative;
}

//Displays the BPM at the top of the screen
void displayBPM(int bpm) {
  // Initializes text and color
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(4); 
  // Displays "BPM: 
  tft.setCursor(10, 10); 

  tft.print("BPM: ");
  // Displays the BPM reading in large green text
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(6.5); 
  tft.fillRect(120, 15, 120, 50, ILI9341_BLACK);


  tft.setCursor(120, 10); 
  tft.println(bpm, 1); 
}

  // This function plots the waveform on the graph


void displayWaveform() {
  static unsigned long lastUpdateTime = 0; // Stores the last update time
  const long updateInterval = 2; // Update interval in milliseconds (adjust as needed)

  // Check if the update interval has passed
  if (millis() - lastUpdateTime > updateInterval) {
    // Initializes x value, reads the analog input, and scales the y value based on the signal reading
    static int x = 0;
    static int previousY = graphY + graphHeight / 2; // Initialize previousY if needed
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
      tft.fillRect(graphX+1, graphY+1, graphWidth-2, graphHeight-2, ILI9341_BLACK);
    }

    lastUpdateTime = millis(); // Update the last update time
  }
  // Now the function will only plot a point if the updateInterval has passed, without blocking.
}




  // Displays info about heart rate based on BPM reading
void displayHeartRateInfo(int bpm){
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2); 
  tft.setCursor(10, 240); 
  tft.print("Your Heart Rate is: "); 

  //Display "VERY LOW" If BPM reading is below 45
  if (bpm < 45){
  tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(4); 

  tft.setCursor(20, 260); 
  tft.print("VERY LOW"); 
  }

  // Display "LOW" If BPM reading is between 45 and 55
  if (bpm > 45 && bpm < 55){
  tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(4); 

  tft.setCursor(20, 260); 
  tft.print("LOW"); 
  }

  // Display "NORMAL" If BPM reading is between 55 and 80
  if (bpm > 55 && bpm < 80){
  tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setTextSize(4); 

  tft.setCursor(20, 260); 
  tft.print("NORMAL"); 
  }

  // Display "ELEVATED" If BPM reading is between 80 and 120
  if (bpm > 80 && bpm < 120){
  tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
  tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
  tft.setTextSize(4); 

  tft.setCursor(20, 260); 
  tft.print("ELEVATED"); 
  }

  // Display "HIGH" If BPM reading is over 120
  if (bpm > 120){
  tft.fillRect(0, 260, 260, 40, ILI9341_BLACK);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(4); 
  tft.setCursor(20, 260); 
  tft.print("HIGH"); 
  }
  
}


void togglePinNonBlocking() {
  static unsigned long lastToggleTime = 0; // Stores the last time the pin was toggled
  const unsigned long toggleInterval = 1000; // Toggle interval in microseconds
  static bool pinState = false; // Keeps track of the current pin state

  unsigned long currentMicros = micros(); // Get the current time in microseconds

  // Check if the toggle interval has passed
  if (currentMicros - lastToggleTime >= toggleInterval) {
    pinState = !pinState; // Toggle the state
    digitalWrite(2, pinState ? HIGH : LOW); // Set the pin to the new state

    // Save the last toggle time, taking into account overflow
    lastToggleTime = currentMicros;
  }
}

