#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

int W, H;
int waveMovement = 0;


// ----------------------------------------------------
// FISH
// ----------------------------------------------------

const int NUM_FISH = 4;

int fishX[NUM_FISH];
int fishY[NUM_FISH];
int fishSize[NUM_FISH];
int fishSpeed[NUM_FISH];
int fishDir[NUM_FISH];        // 1 = swimming right, -1 = swimming left
bool fishActive[NUM_FISH];
uint16_t fishColor[NUM_FISH];

uint16_t palette[] = {
  0xFD20,   // orange
  0xFFE0,   // yellow
  0xF81F,   // pink
  0xFA60,   // coral
  0xAFE5    // light green
};


// ----------------------------------------------------
// SHARK
// ----------------------------------------------------

int sharkX = 0;
int sharkSpeed = 2;
bool sharkActive = false;
int calmTimer = 0;


// ----------------------------------------------------
// BUBBLES
// ----------------------------------------------------

const int NUM_BUBBLES = 6;

int bubbleX[NUM_BUBBLES];
int bubbleY[NUM_BUBBLES];
int bubbleSize[NUM_BUBBLES] = { 3, 4, 2, 3, 4, 3};


// ----------------------------------------------------
// SETUP
// ----------------------------------------------------

void setup() {

  tft.init();
  tft.setRotation(1);

  W = tft.width();
  H = tft.height();

  spr.setColorDepth(16);

  if (spr.createSprite(W, H) == nullptr) {
    tft.fillScreen(TFT_RED);   // not enough memory for the buffer
    while (true) {}
  }

  randomSeed(analogRead(0));

  for (int i = 0; i < NUM_FISH; i++) {
    resetFish(i, true);
  }

  for (int i = 0; i < NUM_BUBBLES; i++) {
    bubbleX[i] = random(10, W - 10);
    bubbleY[i] = random(30, H - 5);
  }
}


// ----------------------------------------------------
// LOOP
// ----------------------------------------------------

void loop() {

  drawOcean();
  drawBubbles();
  drawFish();
  drawShark();

  spr.pushSprite(0, 0);

  waveMovement++;

  delay(30);
}


// ----------------------------------------------------
// OCEAN (smooth ombre + waves at top)
// ----------------------------------------------------

void drawOcean() {

  // top color and bottom color, everything between is blended
  int r1 = 90,  g1 = 215, b1 = 225;
  int r2 = 30,  g2 = 120, b2 = 180;

  for (int y = 0; y < H; y++) {

    int r = r1 + (r2 - r1) * y / (H - 1);
    int g = g1 + (g2 - g1) * y / (H - 1);
    int b = b1 + (b2 - b1) * y / (H - 1);

    spr.drawFastHLine(0, y, W, spr.color565(r, g, b));
  }

  // waves
  uint16_t waveColor = spr.color565(180, 240, 245);

  for (int x = 0; x < W; x += 15) {

    int y = 12 + sin((x + waveMovement) * 0.08) * 3;

    spr.drawLine(x, y, x + 8, y, waveColor);
  }
}


// ----------------------------------------------------
// BUBBLES
// ----------------------------------------------------

void drawBubbles() {

  uint16_t bubbleColor = spr.color565(170, 240, 245);

  for (int i = 0; i < NUM_BUBBLES; i++) {

    // float up slowly
    if (waveMovement % 2 == 0) {
      bubbleY[i] -= 1;
    }

    // start over at the bottom
    if (bubbleY[i] < 22) {
      bubbleY[i] = H - random(5, 20);
      bubbleX[i] = random(10, W - 10);
    }

    // tiny side to side sway
    int x = bubbleX[i] + sin(bubbleY[i] * 0.15) * 2;

    spr.drawCircle(x, bubbleY[i], bubbleSize[i], bubbleColor);
    spr.drawPixel(x - 1, bubbleY[i] - 1, TFT_WHITE);
  }
}


// ----------------------------------------------------
// FISH
// ----------------------------------------------------

void resetFish(int i, bool startOnScreen) {

  fishSize[i]   = random(6, 13);
  fishSpeed[i]  = random(1, 3);
  fishY[i]      = random(28, H - 30);
  fishColor[i]  = palette[random(0, 5)];
  fishDir[i]    = 1;
  fishActive[i] = true;

  if (startOnScreen) {
    fishX[i] = random(10, W - 60);
  } else {
    fishX[i] = random(-70, -15);
  }
}


void drawFish() {

  for (int i = 0; i < NUM_FISH; i++) {

    if (!fishActive[i]) continue;

    // turn around when the shark gets close
    if (sharkActive && fishDir[i] == 1 && (sharkX - 42 - fishX[i]) < 70) {
      fishDir[i] = -1;
    }

    // move
    if (fishDir[i] == 1) {

      fishX[i] += fishSpeed[i];

      // wrap around, but only when no shark is around
      if (!sharkActive && fishX[i] > W + 20) {
        fishX[i] = -20;
        fishY[i] = random(28, H - 30);
      }

    } else {

      // swim away faster than the shark
      fishX[i] -= fishSpeed[i] + 2;

      if (fishX[i] < -30) {
        fishActive[i] = false;
        continue;
      }
    }

    int x = fishX[i];
    int y = fishY[i];
    int s = fishSize[i];
    int d = fishDir[i];

    // body
    spr.fillEllipse(x, y, s, s * 2 / 3, fishColor[i]);

    // tail (always on the back end)
    spr.fillTriangle(
      x - d * s,       y,
      x - d * s * 2,   y - s * 2 / 3,
      x - d * s * 2,   y + s * 2 / 3,
      fishColor[i]
    );

    // eye (front end)
    spr.fillCircle(x + d * s / 2, y - s / 4, 1, TFT_BLACK);
  }
}


// ----------------------------------------------------
// SHARK (swims right to left)
// ----------------------------------------------------

void drawShark() {

  // wait a bit, then send the shark in
  if (!sharkActive) {

    calmTimer++;

    if (calmTimer > 150) {
      sharkActive = true;
      sharkX = W + 100;
    }
    return;
  }

  sharkX -= sharkSpeed;

  int y = H / 2 + 5;
  uint16_t sharkColor = spr.color565(120, 135, 150);

  // body
  spr.fillEllipse(sharkX, y, 42, 17, sharkColor);

  // tail (on the right, since the shark faces left)
  spr.fillTriangle(
    sharkX + 36, y,
    sharkX + 62, y - 16,
    sharkX + 62, y + 16,
    sharkColor
  );

  // fin
  spr.fillTriangle(
    sharkX - 5,  y - 15,
    sharkX + 12, y - 15,
    sharkX + 10, y - 32,
    sharkColor
  );

  // eye
  spr.fillCircle(sharkX - 28, y - 4, 2, TFT_BLACK);

  // mouth
  spr.drawLine(sharkX - 40, y + 5, sharkX - 26, y + 8, TFT_BLACK);

  // shark is gone, bring the fish back
  if (sharkX < -80) {

    sharkActive = false;
    calmTimer = 0;

    for (int i = 0; i < NUM_FISH; i++) {
      resetFish(i, false);
    }
  }
}
