#include "Adafruit_mfGFX.h"   // Core graphics library
#include "RGBmatrixPanel.h" // Hardware-specific library
#include "math.h"

// LED Matrix Pins
#define CLK D6
#define OE  D7
#define LAT A4
#define A   A0
#define B   A1
#define C   A2
#define D	  A3

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

// Constants for Plasma function
static const int8_t PROGMEM sinetab[256] = {
     0,   2,   5,   8,  11,  15,  18,  21,
    24,  27,  30,  33,  36,  39,  42,  45,
    48,  51,  54,  56,  59,  62,  65,  67,
    70,  72,  75,  77,  80,  82,  85,  87,
    89,  91,  93,  96,  98, 100, 101, 103,
   105, 107, 108, 110, 111, 113, 114, 116,
   117, 118, 119, 120, 121, 122, 123, 123,
   124, 125, 125, 126, 126, 126, 126, 126,
   127, 126, 126, 126, 126, 126, 125, 125,
   124, 123, 123, 122, 121, 120, 119, 118,
   117, 116, 114, 113, 111, 110, 108, 107,
   105, 103, 101, 100,  98,  96,  93,  91,
    89,  87,  85,  82,  80,  77,  75,  72,
    70,  67,  65,  62,  59,  56,  54,  51,
    48,  45,  42,  39,  36,  33,  30,  27,
    24,  21,  18,  15,  11,   8,   5,   2,
     0,  -3,  -6,  -9, -12, -16, -19, -22,
   -25, -28, -31, -34, -37, -40, -43, -46,
   -49, -52, -55, -57, -60, -63, -66, -68,
   -71, -73, -76, -78, -81, -83, -86, -88,
   -90, -92, -94, -97, -99,-101,-102,-104,
  -106,-108,-109,-111,-112,-114,-115,-117,
  -118,-119,-120,-121,-122,-123,-124,-124,
  -125,-126,-126,-127,-127,-127,-127,-127,
  -128,-127,-127,-127,-127,-127,-126,-126,
  -125,-124,-124,-123,-122,-121,-120,-119,
  -118,-117,-115,-114,-112,-111,-109,-108,
  -106,-104,-102,-101, -99, -97, -94, -92,
   -90, -88, -86, -83, -81, -78, -76, -73,
   -71, -68, -66, -63, -60, -57, -55, -52,
   -49, -46, -43, -40, -37, -34, -31, -28,
   -25, -22, -19, -16, -12,  -9,  -6,  -3
};

const float radius1  = 16.3, radius2  = 23.0, radius3  = 40.8, radius4  = 44.2,
            centerx1 = 16.1, centerx2 = 11.6, centerx3 = 23.4, centerx4 =  4.1,
            centery1 =  8.7, centery2 =  6.5, centery3 = 14.0, centery4 = -2.9;
float       angle1   =  0.0, angle2   =  0.0, angle3   =  0.0, angle4   =  0.0;
long        hueShift =  0;

#define debug false

// temperature service constants
#define openweatherdomain ""
#define openweatherkey "bd9bd69df190b94430443d2566227376"

String currentFeature;

void setup() {
  if (debug) {
    Serial.begin(115200);

    delay(4000);
    Serial.println("Debugging Magic Box...");
  }

  //Spark functions
  Spark.function("showWeather", drawTemp);
  Spark.function("setText", drawText);
  Spark.function("showPlasma", drawPlasma);
  Spark.function("clearScreen", clearScreen);

  matrix.begin();

  drawText("LED Box v0.2");

  prepScreenForText();
}

void loop() {
  if (currentFeature == "plasma") {
    drawPlasma("none");
  }
}

int drawText(String command) {
  String text;
  int lastWordLength = 0;
  int currentWordLength = 0;
  int line = 1;

  currentFeature = "text";

  if (command.length() == 0) {
      text = "ENTER TEXT?";
  } else {
    text = command;
  }

  if (debug) {
      Serial.print("Text sent to panel: ");
      Serial.println(text);
  }

  prepScreenForText();

  char textArray[1024];
  strncpy(textArray, text.c_str(), sizeof(text));
  textArray[sizeof(text) - 1] = 0;
  char* word = strtok(textArray, " ");

  while (word != 0) {
    currentWordLength = strlen(word);

    if (line == 5) {
      line = 1;
      delay(3000);
      prepScreenForText();
    }

    if (lastWordLength == 0) {
      if (currentWordLength >= 4) {
        matrix.println(word);
        line++;
        currentWordLength = 0;
      } else {
        matrix.print(word);
      }
    } else {
      if ( currentWordLength > (5 - lastWordLength) ) {
        matrix.println();
        line++;
      } else if ( currentWordLength + lastWordLength >= 5 ) {
        matrix.println();
        line++;
        currentWordLength = 0;
      } else {
        currentWordLength += lastWordLength+1;
        matrix.print(" ");
      }
      matrix.print(word);
    }

    lastWordLength = currentWordLength;
    word = strtok(0, " ");
  }

  delay(3000);

  return 1;
}

int drawTemp(String city) {
  String text;

  currentFeature = "temp";

  if (city.length() == 0) {
      text = "";
  } else {
    text = city;
  }

  // fill the screen with 'black'
  matrix.fillScreen(matrix.Color333(0, 0, 0));

  matrix.setCursor(1, 0);
  matrix.setTextSize(2);

  matrix.setTextColor(matrix.Color333(0,0,7));
  matrix.print("73");

  matrix.drawCircle(26, 2, 2, matrix.Color333(0, 0, 7));
  matrix.println();

  matrix.setTextSize(1);
  matrix.println(city);

  delay(5000);

  return 1;
}

int drawPlasma(String command) {
  int           x1, x2, x3, x4, y1, y2, y3, y4, sx1, sx2, sx3, sx4;
  unsigned char x, y;
  long          value;

  currentFeature = "plasma";

  sx1 = (int)(cos(angle1) * radius1 + centerx1);
  sx2 = (int)(cos(angle2) * radius2 + centerx2);
  sx3 = (int)(cos(angle3) * radius3 + centerx3);
  sx4 = (int)(cos(angle4) * radius4 + centerx4);
  y1  = (int)(sin(angle1) * radius1 + centery1);
  y2  = (int)(sin(angle2) * radius2 + centery2);
  y3  = (int)(sin(angle3) * radius3 + centery3);
  y4  = (int)(sin(angle4) * radius4 + centery4);

  for(y=0; y<matrix.height(); y++) {
    x1 = sx1; x2 = sx2; x3 = sx3; x4 = sx4;
    for(x=0; x<matrix.width(); x++) {
      value = hueShift
        + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x1 * x1 + y1 * y1) >> 2))
        + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x2 * x2 + y2 * y2) >> 2))
        + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x3 * x3 + y3 * y3) >> 3))
        + (int8_t)pgm_read_byte(sinetab + (uint8_t)((x4 * x4 + y4 * y4) >> 3));
      matrix.drawPixel(x, y, matrix.ColorHSV(value * 3, 255, 255, true));
      x1--; x2--; x3--; x4--;
    }
    y1--; y2--; y3--; y4--;
  }

  angle1 += 0.03;
  angle2 -= 0.07;
  angle3 += 0.13;
  angle4 -= 0.15;
  hueShift += 2;

  delay(100);

  return 1;
}

void prepScreenForText() {
  // fill the screen with 'black'
  matrix.fillScreen(matrix.Color333(0, 0, 0));

  // draw some text!
  matrix.setCursor(1, 0);    // start at top left, with one pixel of spacing
  matrix.setTextSize(1);     // size 1 == 8 pixels high
  matrix.setTextWrap(false);

  matrix.setTextColor(matrix.Color333(7,7,7));
}

int clearScreen(String command) {
  currentFeature = "clear";

  prepScreenForText();

  return 1;
}
