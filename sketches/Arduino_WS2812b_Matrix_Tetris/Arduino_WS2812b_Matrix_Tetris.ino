#define DEBUG_MODE false

#include <EEPROM.h>

#define START_TOP_RIGHT 1
#define START_TOP_LEFT 0

// WS2812B vars
#include "FastLED.h"

#define LED_TYPE WS2811 // led type
#define LED_PIN 12       // Pin to cotrol ws812b leds 

#define LED_START_DIRECTION START_TOP_RIGHT // first point (led)

#define NUM_LEDS 256     // Count of leds
#define NUM_LED_COLS 8 // Count of leds in one row

const int NUM_LED_ROWS = NUM_LEDS / NUM_LED_COLS; // Count of led rows 

#define MATRIX_COUNT 1 // count of matrix

CRGB leds[NUM_LEDS];
CLEDController* controllers[2];

bool cup_leds[NUM_LED_ROWS][NUM_LED_COLS];
byte cup_led_collors[NUM_LED_ROWS][NUM_LED_COLS];

int current_brightness = 30;

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

#define JOYSTICK_X_CENTER 510
#define JOYSTICK_Y_CENTER 511
#define JOYSTICK_DEAD_ZONE 70

bool btn_joystick_last_state = LOW;
bool btn_joystick_current_state = LOW;

// Tetris vars
#include <math.h>

#define FIGURE_SIZE 4 // max with or height for figure
#define FIGURE_COUNT 7 // Count of figures for random

bool figure[FIGURE_SIZE][FIGURE_SIZE];

int figure_x; 
int figure_y;
byte figure_color;

int game_speed = 100;

// Snake vars
byte snake_direction; // 1 - up, 2 - down, 3 - left, 4 - right
int snake_length;

// common games vars
byte game_num;
bool game_over = false;

unsigned long global_timer; // global timer
unsigned long game_timer; // game timer
unsigned long controls_timer; // key timer for controls

#define GLOBAL_LOOP 5 // global timer loop
#define CONTROLS_LOOP 90 // key timer loop for controls

void init_figure(byte figure_num = 0) {
  int _y;

  // sen start position for figure
  figure_y = -1; // generate new figure out of cup for next step it will move to 0 by Y Axis
  figure_x = (int)round(NUM_LED_COLS / 2) - 1;
  //figure_x = random(NUM_LED_COLS - 1); // random x
  
  // set random figure color
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
    case 3: // "/\/"
      figure[1][1] = true;
      figure[2][0] = true;
      figure[2][1] = true;
      figure[3][0] = true;
      break;
    case 4: // "\/\"
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
    case 6: // "T"
      figure[2][0] = true;
      figure[2][1] = true;
      figure[2][2] = true;
      figure[3][1] = true;
      break;
  }

  // Check if we canno't move more down figure set game over TODO: think to move to tick
  for (int i = FIGURE_SIZE - 1; i >=0 ; i--) {
    for (int j = FIGURE_SIZE - 1; j >=0; j--) {
      if (figure[i][j]) {
        _y = (figure_y + 1 - FIGURE_SIZE + (i + 1));

        if (_y >= 0) {
          if (cup_leds[_y][j + figure_x]) {
            game_over = true;
            return false;
          }
        }
      }
    }
  }
}

// clear line on cup on start possion and move down all points
void clean_move_down_cup(int start) {
  for (int j = 0; j < NUM_LED_COLS; j++) {
    cup_leds[start][j] = false;
  }

  for (int i = start - 1; i >= 0; i--) { 
    for (int j = 0; j < NUM_LED_COLS; j++) { 
      if (cup_leds[i][j]) {
        cup_leds[i][j] = false; // clean point
        cup_leds[i+1][j] = true; // move point to next row
        cup_led_collors[i+1][j] = cup_led_collors[i][j]; // move color
      }
    }
  }

  FastLED.show();
}

// check lines to destroy
void check_lines(){
  bool line_full = true;
  byte line_count = FIGURE_SIZE; // count of line need to check

  int _y = figure_y - 1;

  while (line_count > 0) {
    line_full = true;

    for (int j = 0; j < NUM_LED_COLS; j++) { // check if line full
      if (!cup_leds[_y][j]) {
        line_full = false;
      }
    }

    if (line_full) {
      clean_move_down_cup(_y);
    } else {
      _y--;
    }

    line_count--;
  }
}

void game_snake_init(){
  // set start position for snake
  figure_y = (int)round(NUM_LED_ROWS / 2); 
  figure_x = (int)round(NUM_LED_COLS / 2);

  // set random snake color
  figure_color = random(collors_count);
  
  // set snake direction to up
  snake_direction = 1;

  // set snake length
  snake_length = 1;

  // TODO: array of pointers to cup_leds
}

void game_snake_tick(){

  if (DEBUG_MODE) {

  }

  cup_leds[figure_y][figure_x] = false;

  // move snake to direction
  switch (snake_direction) {
    case 1: // move up
        figure_y--;

        if (figure_y < 0)
          figure_y = NUM_LED_ROWS - 1;

      break;
    case 2: // move down
        figure_y++;
        if (figure_y >= NUM_LED_ROWS)

          figure_y = 0;
      break;
    case 3: // move left
        figure_x--;
        if (figure_x < 0)
          figure_x = NUM_LED_COLS - 1;
      break;
    case 4: // move right
        figure_x++;
        if (figure_x >= NUM_LED_COLS)
          figure_x = 0;
      break;
  }

  cup_leds[figure_y][figure_x] = true; // move snale to new point
  cup_led_collors[figure_y][figure_x] = figure_color;  // write color for snake to new point

  update_matrix_leds();
  FastLED.show();
}

void game_tetris_init() {
  init_figure(random(FIGURE_COUNT));
}

void game_tetris_tick(){
  if (move_down()) {
    update_matrix_leds();
    FastLED.show();
  }
}

// check keys on joystick
void game_check_keys() {
  // check if setup Joystick BTN was pressed
  btn_joystick_current_state = debounce(JOYSTICK_SWT_PIN, btn_joystick_last_state);

  if (btn_joystick_last_state == HIGH && btn_joystick_current_state == LOW) {
    //change brightness

    current_brightness += 30;

    if (current_brightness > 255) {
      current_brightness = 10;
    }

    // current_brightness form 0 to 255
    FastLED.setBrightness(current_brightness);  // set leds brightness

    updateEEPROM();

    if (DEBUG_MODE) {
      Serial.println("Joystick Pressed!");
    }
  }

  btn_joystick_last_state = btn_joystick_current_state;

  int x = analogRead(JOYSTICK_VRX_PIN);  // read x coordinate from joystick
  int y = analogRead(JOYSTICK_VRY_PIN);  // read y coordinate from joystick

  // x = 510 center
  if (x >= 0 && x <= JOYSTICK_X_CENTER - JOYSTICK_DEAD_ZONE) {
    // left pressed

    switch (game_num) {
      case 0:  // tetris game
        if (move_left()) {
          update_matrix_leds();
          FastLED.show();
        }
        break;
      case 1:  // snake game
        snake_direction = 3;
        break;
    }
  }

  if (x >= JOYSTICK_X_CENTER + JOYSTICK_DEAD_ZONE && x <= 1023) {
    // right pressed
    switch (game_num) {
      case 0:  // tetris game
        if (move_right()) {
          update_matrix_leds();
          FastLED.show();
        }
        break;
      case 1:  // snake game
        snake_direction = 4;
        break;
    }
  }

  // y = 511 center
  if (y >= 0 && y < JOYSTICK_Y_CENTER - JOYSTICK_DEAD_ZONE) {
    // up pressed
    switch (game_num) {
      case 0:  // tetris game
        if (move_up()) {
          update_matrix_leds();
          FastLED.show();
        }
        break;
      case 1:  // snake game
        snake_direction = 1;
        break;
    }
  }

  if (y > JOYSTICK_Y_CENTER + JOYSTICK_DEAD_ZONE && y <= 1023) {
    // down pressed
    switch (game_num) {
      case 0:  // tetris game
        if (move_down()) {
          update_matrix_leds();
          FastLED.show();
        }
        break;
      case 1:  // snake game
        snake_direction = 2;
        break;
    }
  }
}

void pull_down(bool (& fig_arr) [FIGURE_SIZE][FIGURE_SIZE]) {
  byte num_row_to_move = FIGURE_SIZE - 1;
  bool have_points;

  for (int i = FIGURE_SIZE - 1; i >= 0; i--) {
    have_points = false;
    for (int j = 0; j < FIGURE_SIZE; j++) {
      if (fig_arr[i][j]) {
        fig_arr[i][j] = false;
        fig_arr[num_row_to_move][j] = true;
        have_points = true;
      }
    }
    if (have_points) {
      num_row_to_move -= 1;
    }
  }
}

bool move_up() {
  int _y;
  int _x;

  bool figure_temp[FIGURE_SIZE][FIGURE_SIZE];  // temporary figure to store ration

  // copy figure to tem variable and rotate it
  for (int i = 0; i < FIGURE_SIZE; i++) {
    for (int j = 0; j < FIGURE_SIZE; j++) {
      figure_temp[j][FIGURE_SIZE - 1 - i] = figure[i][j];
    }
  }

  // move figure to down position on array
  pull_down(figure_temp);

  // clean coordinates from cup with current figure
  for (int j = 0; j < FIGURE_SIZE; j++) {
    for (int i = 0; i < FIGURE_SIZE; i++) {
      if (figure[i][j]) {
        _y = (figure_y - FIGURE_SIZE + (i + 1));  // calculate cup coordinate for y
        _x = figure_x + j;                        // calculate cup coordinate for x

        if (_y >= 0 && _x >= 0) {
          cup_leds[_y][_x] = false;  // remove old coordinate
        }
      }
    }
  }

  // check if new figure have space
  // TODO: Check in case no space at the left or right (but we can move figure in any direction to left or right and space will be enougph)
  for (int j = 0; j < FIGURE_SIZE; j++) {
    for (int i = 0; i < FIGURE_SIZE; i++) {
      if (figure_temp[i][j]) {
        _y = (figure_y - FIGURE_SIZE + (i + 1));  // calculate cup coordinate for y
        _x = figure_x + j;                        // calculate cup coordinate for x

        if (_y >= 0 && _y < NUM_LED_ROWS && _x >= 0 && _x < NUM_LED_COLS && cup_leds[_y][_x]) { // if coordinates inside cup check if point is empty
          return false;
        } else if (_y < 0 || _y >= NUM_LED_ROWS || _x < 0 || _x >= NUM_LED_COLS) { // TODO: think about this. Should i check this or not
          return false;
        }
      }
    }
  }

  // copy temp rotated figure to current
  for (int i = 0; i < FIGURE_SIZE; i++) {
    for (int j = 0; j < FIGURE_SIZE; j++) {
      figure[i][j] = figure_temp[i][j];
    }
  }

  for (int j = 0; j < FIGURE_SIZE; j++) {
    for (int i = 0; i < FIGURE_SIZE; i++) {
      if (figure[i][j]) {
        _y = (figure_y - FIGURE_SIZE + (i + 1));  // calculate cup coordinate for y
        _x = figure_x + j;                        // calculate cup coordinate for x

        if (_y >= 0 && _x >= 0) {
          cup_leds[_y][_x] = true;                 // write new coordinate
          cup_led_collors[_y][_x] = figure_color;  // write new color
        }
      }
    }
  }

  return true;
}

bool move_right() {
  int _y;
  int _x;

  bool check_col_fig[FIGURE_SIZE] = { false, false, false, false };

  // check if figure can move more to down
  for (int j = FIGURE_SIZE - 1; j >= 0; j--) {
    for (int i = 0; i < FIGURE_SIZE; i++) {
      if (figure[i][j]) {
        if (!check_col_fig[i] && figure[i][j]) {
          _y = figure_y - FIGURE_SIZE + (i + 1);  // new coordinate for y
          _x = figure_x + j + 1;                  // new coordinate for x + 1 (move right1)

          if (_y >= 0 && _x < NUM_LED_COLS && cup_leds[_y][_x]) {
            return false;
          } else {
            if (_x >= NUM_LED_COLS) {
              return false;
            }

            check_col_fig[i] = true;
          }
        }
      }
    }
  }

  figure_x += 1;

  // if code goes here start draw figure, space is free
  for (int j = FIGURE_SIZE - 1; j >= 0; j--) {
    for (int i = 0; i < FIGURE_SIZE; i++) {
      if (figure[i][j]) {
        _y = (figure_y - FIGURE_SIZE + (i + 1));  // calculate cup coordinate for y
        _x = figure_x + j;                        // calculate cup coordinate for x

        if (_y >= 0 && _x < NUM_LED_COLS) {
          cup_leds[_y][_x - 1] = false;  // remove old coordinate

          cup_leds[_y][_x] = true;                 // write new coordinate
          cup_led_collors[_y][_x] = figure_color;  // write new color
        }
      }
    }
  }

  return true;
}

bool move_left() {
  int _y;
  int _x;

  bool check_col_fig[FIGURE_SIZE] = { false, false, false, false };

  // check if figure can move more to down
  for (int j = 0; j < FIGURE_SIZE; j++) {
    for (int i = 0; i < FIGURE_SIZE; i++) {
      if (figure[i][j]) {
        if (!check_col_fig[i] && figure[i][j]) {
          _y = figure_y - FIGURE_SIZE + (i + 1);  // new coordinate for y
          _x = figure_x + j - 1;                  // new coordinate for x - 1 (move left)

          if (_y >= 0 && _x >= 0 && cup_leds[_y][_x]) {
            return false;
          } else {
            if (_x < 0) {
              return false;
            }

            check_col_fig[i] = true;
          }
        }
      }
    }
  }

  figure_x -= 1;

  // if code goes here start draw figure, space is free
  for (int j = 0; j < FIGURE_SIZE; j++) {
    for (int i = 0; i < FIGURE_SIZE; i++) {
      if (figure[i][j]) {
        _y = (figure_y - FIGURE_SIZE + (i + 1));  // calculate cup coordinate for y
        _x = figure_x + j;                        // calculate cup coordinate for x

        if (_y >= 0 && _x >= 0) {
          cup_leds[_y][_x + 1] = false;  // remove old coordinate

          cup_leds[_y][_x] = true;                 // write new coordinate
          cup_led_collors[_y][_x] = figure_color;  // write new color
        }
      }
    }
  }

  return true;
}

bool move_down() {
  int _y;
  int _x;
  bool can_do_action = true;

  figure_y++;

  bool check_col_fig[FIGURE_SIZE] = { false, false, false, false };  // flags for cols that figure can move down with all parts

  // check if figure can move more to down
  for (int i = FIGURE_SIZE - 1; i >= 0; i--) {
    for (int j = FIGURE_SIZE - 1; j >= 0; j--) {
      if (figure[i][j]) {
        if (!check_col_fig[j] && figure[i][j]) {
          _y = figure_y - FIGURE_SIZE + (i + 1);
          _x = j + figure_x;

          if (_y >= 0 && _y < NUM_LED_ROWS && cup_leds[_y][_x]) {
            // figure can't moving down more

            check_lines();  // check if possible to destroy full lines

            init_figure(random(FIGURE_COUNT));  // init new figure
            return false;                       // exit tick function not enougph spaces to move
          } else {
            if (_y >= NUM_LED_ROWS) {
              check_lines();  // check if possible to destroy full lines
              init_figure(random(FIGURE_COUNT));  // init new figure
              return false;
            }

            check_col_fig[j] = true;
          }
        }
      }
    }
  }

  if (can_do_action) {
    // if code goes here start draw figure, space is free
    for (int i = FIGURE_SIZE - 1; i >= 0; i--) {
      for (int j = FIGURE_SIZE - 1; j >= 0; j--) {
        if (figure[i][j]) {
          _y = (figure_y - FIGURE_SIZE + (i + 1));
          _x = figure_x + j; 

          if (_y >= 0) {
            if (_y > 0) {
              cup_leds[_y - 1][_x] = false;
            }

            cup_leds[_y][_x] = true;
            cup_led_collors[_y][_x] = figure_color;
          }
        }
      }
    }
  }

  return can_do_action;
}

// convert i, j coordinates to led num for ws2812b 
int get_led_num(int i = 0, int j = 0){
  switch (LED_START_DIRECTION) {
    case START_TOP_RIGHT:
      return NUM_LED_COLS * i + ((i % 2) ? j : (NUM_LED_COLS - j - 1));
      break;
    case START_TOP_LEFT:
    default:
      return 0;
      break;
  }
}

void print_matrix(){
  for (int i = 0 ; i < NUM_LED_ROWS; i++ ) {
    for (int j = 0 ; j < NUM_LED_COLS; j++ ) {
      Serial.print(cup_leds[i][j]);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void print_color_matrix(){
  for (int i = 0 ; i < NUM_LED_ROWS; i++ ) {
    for (int j = 0 ; j < NUM_LED_COLS; j++ ) {
      Serial.print(cup_led_collors[i][j]);
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
  for (int i = 0; i < NUM_LED_ROWS; i++) {
    for (int j = 0; j < NUM_LED_COLS; j++) {
      if (cup_leds[i][j]) {
        leds[get_led_num(i, j)].setRGB(collors[cup_led_collors[i][j]][0], collors[cup_led_collors[i][j]][1], collors[cup_led_collors[i][j]][2]);
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

// update device settings
void updateEEPROM() {
  EEPROM.write(1, current_brightness);
  
  if (game_num == 1) {
    EEPROM.write(2, 0);
  } else {
    EEPROM.write(2, 1);
  }
}

// read device settings
void readEEPROM() {
  current_brightness = EEPROM.read(1);
  game_num = EEPROM.read(2);

  if (current_brightness == 0) {
    current_brightness = 10;
    updateEEPROM();
  }

  if (game_num > 1) {
    game_num = 0;
    updateEEPROM();
  }
}

void setup() {
  if (DEBUG_MODE) {
    Serial.begin(9600);
  }

  // Read variables from memory
  readEEPROM();

  // Update variables from memory
  updateEEPROM();

  // setup led
  FastLED.setBrightness(current_brightness);  // set leds brightness
  FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, NUM_LEDS);
  //controllers[0] = &FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, NUM_LEDS);

  // make lead black before start
  FastLED.clear();
  //FastLED.show();
  
  // generate seed start value for random random. Get value from empty analog PIN
  randomSeed(analogRead(A3));

  // joystick
  pinMode(JOYSTICK_SWT_PIN, INPUT_PULLUP);

  switch (game_num) {
    case 0:
      game_tetris_init(); //tetris
    break;
    case 1:
      game_snake_init(); //snake
    break;
  }
}

void loop() {
  if (millis() - global_timer > GLOBAL_LOOP) {
    global_timer = millis();  // reset main timer
   
    if (!game_over) {
      if (millis() - controls_timer > (CONTROLS_LOOP)) {
        controls_timer = millis();  // reset controls timer

        game_check_keys();
      }
    }
        
    if (!game_over) {
      if (millis() - game_timer > (GLOBAL_LOOP * game_speed)) {
        game_timer = millis();  // reset game timer
        
        switch (game_num) {
          case 0:
            game_tetris_tick();
          break;
          case 1:
            game_snake_tick();
          break;
        }
        
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
    }
  }
}