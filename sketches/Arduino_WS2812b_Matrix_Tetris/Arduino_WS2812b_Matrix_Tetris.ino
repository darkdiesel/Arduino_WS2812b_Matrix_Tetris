#define DEBUG_MODE false

#define START_TOP_RIGHT 1
#define START_TOP_LEFT 0

// WS2812B vars
#include "FastLED.h"

#define LED_TYPE WS2811 // led type
#define LED_PIN 12       // Pin to cotrol ws812b leds 

#define LED_START_DIRECTION START_TOP_RIGHT // first point (led)

#define NUM_LEDS 256     // Count of leds
#define NUM_COL_LEDS 8 // Count of leds in one row

const int NUM_ROW_LEDS = NUM_LEDS / NUM_COL_LEDS; // Count of led rows 

#define MATRIX_COUNT 1 // count of matrix

CRGB leds[NUM_LEDS];
CLEDController* controllers[2];

bool cup_leds[NUM_ROW_LEDS][NUM_COL_LEDS];
byte cup_leds_collors[NUM_ROW_LEDS][NUM_COL_LEDS];

int cur_brightness = 30;

unsigned long global_timer; // global timer

#define GLOBAL_LOOP 50 // global timer loop

int random_loop = 300;
unsigned long random_timer; // random timer

int collors[][3] =
{
  {255, 0, 0}, // red
  {255, 128, 0}, // orange
  {255, 255, 0}, // yellow 
  {0, 255, 0}, // green
  {0, 255, 255}, // cyan
  {0, 0, 255}, // blue
  {128, 0, 128}, // purple
};

const byte collors_count = sizeof(collors) / sizeof(collors[0]);

// joystick vars
#define JOYSTICK_VRX_PIN A1
#define JOYSTICK_VRY_PIN A0
#define JOYSTICK_SWT_PIN 9

bool btn_joystick_last_state = LOW;
bool btn_joystick_current_state = LOW;

// Tetris vars
#include <math.h>

#define FIGURE_SIZE 4 // max with or height for figure
#define FIGURE_COUNT 6 // Count of figures for random

bool figure[FIGURE_SIZE][FIGURE_SIZE];

int figure_x;
int figure_y;
byte figure_color;

byte game_speed = 1;
bool game_over = false;

void init_figure(byte figure_num = 0) {
  int _y;

  figure_y = -1;
  figure_x = (int)round(NUM_COL_LEDS / 2);
  figure_x = random(NUM_COL_LEDS - 1);
  figure_color = random(collors_count);

  // clear previous figure from array
  for (int i = 0; i < FIGURE_SIZE; i++) {
    for (int j = 0; j < FIGURE_SIZE; j++) {
      figure[i][j] = false;
    }
  }

  // write new figure to array
  switch (figure_num) {
    case 0: // "L"
      figure[1][0] = true;
      figure[2][0] = true;
      figure[3][0] = true;
      figure[3][1] = true;
      break;
    case 1: // "J"
      figure[1][1] = true;
      figure[2][1] = true;
      figure[3][1] = true;
      figure[3][0] = true;
      break;
    case 2: // "I"
      figure[0][0] = true;
      figure[1][0] = true;
      figure[2][0] = true;
      figure[3][0] = true;
      break;
    case 3: // ""
      figure[1][1] = true;
      figure[2][0] = true;
      figure[2][1] = true;
      figure[3][0] = true;
      break;
    case 4: // ""
      figure[1][0] = true;
      figure[2][0] = true;
      figure[2][1] = true;
      figure[3][1] = true;
      break;
    case 5: // "cube"
      figure[2][0] = true;
      figure[2][1] = true;
      figure[3][0] = true;
      figure[3][1] = true;
      break;
  }

  // for (int i = FIGURE_SIZE - 1; i >=0 ; i--) {
  //   for (int j = FIGURE_SIZE - 1; j >=0; j--) {
  //     if (figure[i][j]) {
  //       _y = (figure_y + 1 - FIGURE_SIZE + (i + 1));

  //       if (_y >= 0) {
  //         if (cup_leds[_y][j + figure_x]) {
  //           game_over = true;
  //           return false;
  //         }
  //       }
  //     }
  //   }
  // }

  // if (DEBUG_MODE) {
  //   Serial.println("init figure");

  //   Serial.print("figure num: ");
  //   Serial.println(figure_num);

  //   Serial.print("init figure y: ");
  //   Serial.println(figure_y);
  //   Serial.print("init figure x: ");
  //   Serial.println(figure_x);
  //   Serial.print("init figure c: ");
  //   Serial.println(figure_color);
    
  //   // print_figure();
  //   // print_color_matrix();
  // }
}

// check lines to destroy
void check_lines(){
  
}

void game_tick(){
  int _y;

  figure_y++;

  if (figure_y >= NUM_ROW_LEDS) {
    // figure can't moving down more. Limit of cup height
    
    if (DEBUG_MODE) {
      Serial.println("Figure on the end of cup. Init new figure.");
    }

    // TODO: Check lines for destroy

    init_figure(random(FIGURE_COUNT)); // init new figure
    return;
  }
  
  bool check_col_fig[FIGURE_SIZE] = {false, false, true, true};

  // check figure can move more to down
  for (int i = FIGURE_SIZE - 1; i >=0 ; i--) {
    for (int j = FIGURE_SIZE - 1; j >=0; j--) {
      if (figure[i][j]) {
        if (!check_col_fig[j] && figure[i][j]) {
           _y = (figure_y - FIGURE_SIZE + (i + 1));

           if (_y >= 0 && cup_leds[_y][j + figure_x]) {
            // figure can't moving down more

            // TODO: Check lines for destroy

            init_figure(random(FIGURE_COUNT)); // init new figure
            return;
           } else {
             check_col_fig[j] = true;
           }
        }
      }
    }
  }

  // check if cup have empty space to start draw figure if not game is over
  for (int i = FIGURE_SIZE - 1; i >=0 ; i--) {
    for (int j = FIGURE_SIZE - 1; j >=0; j--) {
      if (figure[i][j]) {
        _y = (figure_y - FIGURE_SIZE + (i + 1));

        if (_y >= 0) {
          if (_y > 0) {
            cup_leds[_y-1][j + figure_x] = false;
          }
          
          cup_leds[_y][j + figure_x] = true;
          cup_leds_collors[_y][j + figure_x] = figure_color;

          // Serial.print("figure color y: ");
          // Serial.println(_y);
          // Serial.print("figure color x: ");
          // Serial.println(j + figure_x);
          // Serial.print("color: ");
          // Serial.println(figure_color);
        }
      }
    }
  }
}

// convert i, j coordinates to led num for ws2812b 
int get_led_num(int i = 0, int j = 0){
  switch (LED_START_DIRECTION) {
    case START_TOP_RIGHT:
      return NUM_COL_LEDS * i + ((i % 2) ? j : (NUM_COL_LEDS - j - 1));
      break;
    case START_TOP_LEFT:
    default:
      return 0;
      break;
  }
}

void print_matrix(){
  for (int i = 0 ; i < NUM_ROW_LEDS; i++ ) {
    for (int j = 0 ; j < NUM_COL_LEDS; j++ ) {
      Serial.print(cup_leds[i][j]);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void print_color_matrix(){
  for (int i = 0 ; i < NUM_ROW_LEDS; i++ ) {
    for (int j = 0 ; j < NUM_COL_LEDS; j++ ) {
      Serial.print(cup_leds_collors[i][j]);
      //Serial.print(" ");
    }
    Serial.println();
  }
}

void print_figure(){
  for (int i = 0 ; i < FIGURE_SIZE; i++ ) {
    for (int j = 0 ; j < FIGURE_SIZE; j++ ) {
      Serial.print(figure[i][j]);
      Serial.print(" ");
    }
    Serial.println();
  }
}

// update leds in library
void update_matrix_leds() {
  for (int i = 0; i < NUM_ROW_LEDS; i++) {
    for (int j = 0; j < NUM_COL_LEDS; j++) {
      if (cup_leds[i][j]) {
        leds[get_led_num(i, j)].setRGB(collors[cup_leds_collors[i][j]][0], collors[cup_leds_collors[i][j]][1], collors[cup_leds_collors[i][j]][2]);
      } else {
        leds[get_led_num(i, j)].setRGB(0, 0, 0);
      }
    }
  }

  FastLED.show();
}

// for debounce buttons
bool debounce(uint8_t btn_pin, bool last) {
  bool current = digitalRead(btn_pin);

  if (last != current)
  {
    delay(5);
    current = digitalRead(btn_pin);
  }

  return current;
}

void setup() {
  if (DEBUG_MODE) {
    Serial.begin(9600);

    Serial.print(" ROWS: ");
    Serial.println(NUM_ROW_LEDS);
  }

  // print_matrix();
  // update_matrix_leds();

    // setup led
  FastLED.setBrightness(cur_brightness);  // set leds brightness
  FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, NUM_LEDS);

  // make lead black before start
  //FastLED.clear();
  //FastLED.show();

  // setup led
  //FastLED.setBrightness(cur_brightness);  // set leds brightness
  //controllers[0] = &FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, NUM_LEDS);

  randomSeed(analogRead(A3));

  // joystick
  pinMode(JOYSTICK_SWT_PIN, INPUT_PULLUP);

  //tetris
  init_figure(random(FIGURE_COUNT));
}

void loop() {
  // check if setup Joystick BTN was pressed
  btn_joystick_current_state = debounce(JOYSTICK_SWT_PIN, btn_joystick_last_state);

  if (btn_joystick_last_state == HIGH && btn_joystick_current_state == LOW) {
    if (DEBUG_MODE) {
      Serial.print("Joystick Pressed: ");
      Serial.println(btn_joystick_last_state);
    }
  }

  btn_joystick_last_state = btn_joystick_current_state;

  int x=analogRead(JOYSTICK_VRX_PIN);
  int y=analogRead(JOYSTICK_VRY_PIN);

  // Serial.print("X: ");
  // Serial.print(x);
  // Serial.print(" Y: ");
  // Serial.println(y);

  if (millis() - global_timer > GLOBAL_LOOP * game_speed) {
    global_timer = millis();  // reset main timer
    
    game_tick();
    update_matrix_leds();
    FastLED.show();

    if (DEBUG_MODE) {
      // Serial.println("tick");

      // Serial.print("figure y: ");
      // Serial.println(figure_y);
      // Serial.print("figure x: ");
      // Serial.println(figure_x);

      // print_matrix();
      // print_color_matrix();
    }
  }
 
  // FastLED.show();
}