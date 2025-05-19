/**
 * Simon Says Game for Arduino
 * Uses PWM for LED brightness control and timers for sequence timing
 * Two-player game where each player must repeat a sequence of LED patterns
 */

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

// change this to make the song slower or faster
int tempo = 105;

// change this to whichever pin you want to use
int buzzer = 24;

int melody[] = {

  // Pacman
  // Score available at https://musescore.com/user/85429/scores/107109
  NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16, //1
  NOTE_B5, 32, NOTE_FS5, -16, NOTE_DS5, 8, NOTE_C5, 16,
  NOTE_C6, 16, NOTE_G6, 16, NOTE_E6, 16, NOTE_C6, 32, NOTE_G6, -16, NOTE_E6, 8,

  NOTE_B4, 16,  NOTE_B5, 16,  NOTE_FS5, 16,   NOTE_DS5, 16,  NOTE_B5, 32,  //2
  NOTE_FS5, -16, NOTE_DS5, 8,  NOTE_DS5, 32, NOTE_E5, 32,  NOTE_F5, 32,
  NOTE_F5, 32,  NOTE_FS5, 32,  NOTE_G5, 32,  NOTE_G5, 32, NOTE_GS5, 32,  NOTE_A5, 16, NOTE_B5, 8
};

// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
// there are two values per note (pitch and duration), so for each note there are four bytes
int notes = sizeof(melody) / sizeof(melody[0]) / 2;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / tempo;

int divider = 0, noteDuration = 0;

const int numLeds = 4;
const int maxSequenceLength = 100;

// Player 1 pins
const int ledsPlayer1[] = {2, 3, 4, 5};
const int buttonsPlayer1[] = {31, 33, 30, 26};

// Player 2 pins
const int ledsPlayer2[] = {6, 7, 8, 9};
const int buttonsPlayer2[] = {38, 36, 28, 53};

// PWM settings
const int pwmFrequency = 1000;  // 1 kHz
const int pwmResolution = 8;    // 8-bit resolution (0-255)
const int ledBrightness = 180;  // Default brightness

// Game variables
int sequence[maxSequenceLength];
int sequenceLength = 1;
int playerInputs[maxSequenceLength];

// Game state
int currentPlayer = 1;
bool gameOver = false;
int score1 = 0;
int score2 = 0;

// Timing variables
unsigned long displayDelay = 500;    // Time each LED stays on
unsigned long pauseDelay = 200;      // Time between LEDs in sequence
unsigned long betweenPlayersDelay = 1000; // Time between player turns

void setup() {
  Serial.begin(9600);
  Serial.println("Simon Says Game Starting!");

  // Set up LED pins as outputs and button pins as inputs with pull-up resistors
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledsPlayer1[i], OUTPUT);
    pinMode(buttonsPlayer1[i], INPUT_PULLUP);

    pinMode(ledsPlayer2[i], OUTPUT);
    pinMode(buttonsPlayer2[i], INPUT_PULLUP);
  }

  // Initialize PWM for all LED pins
  for (int i = 0; i < numLeds; i++) {
    // Set initial PWM to 0 (off)
    analogWrite(ledsPlayer1[i], 0);
    analogWrite(ledsPlayer2[i], 0);
  }

  // Set up Timer1 for sequence timing
  setupTimer();

  // Initialize random seed from an unconnected analog pin
  randomSeed(analogRead(A0));
}

void setupTimer() {
  // Set up Timer1 for timing the game sequence
  // Clear registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  
  // Set compare match register for 1Hz increments
  OCR1A = 15624;  // = 16MHz / (1024 * 1Hz) - 1
  
  // Turn on CTC mode
  TCCR1B |= (1 << WGM12);
  
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  
  // Enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}

// Timer1 interrupt service routine
ISR(TIMER1_COMPA_vect) {
  // This function will be called when Timer1 reaches the set value
  // Can be used for sequence timing if needed
}

void loop() {
  if (!gameOver) {
    Serial.print("Round ");
    Serial.print(sequenceLength);
    Serial.println(" starting!");
    
    // Player 1's turn
    currentPlayer = 1;
    playGame();
    sequenceLength++;
    
    if (!gameOver) {
      delay(betweenPlayersDelay);
      
      // Player 2's turn
      currentPlayer = 2;
      playGame();
      sequenceLength++;
      
      if (!gameOver) {
        delay(betweenPlayersDelay);
      }
    }
  } else {
    // Game is over, show the winner
    displayWinner();
    
    // Wait for any button press to restart
    if (checkAnyButtonPress()) {
      resetGame();
    }
  }
}

void playGame() {
  Serial.print("Player ");
  Serial.print(currentPlayer);
  Serial.println("'s turn!");
  
  // Generate a new sequence
  generateSequence();
  
  // Display the sequence to the player
  displaySequence();
  
  // Wait for player input
  if (!getPlayerInput()) {
    // Player failed
    gameOver = true;
    return;
  }
  
  // Player succeeded this round
  if (currentPlayer == 1) {
    score1++;
  } else {
    score2++;
  }
}

void generateSequence() {
  // Add a new random LED at the end
  sequence[sequenceLength - 1] = random(0, numLeds);
}

void displaySequence() {
  Serial.println("Displaying sequence:");
  
  // Turn off all LEDs first
  turnOffAllLeds();
  delay(pauseDelay * 2);
  
  // Display each step in the sequence
  for (int i = 0; i < sequenceLength; i++) {
    int ledIndex = sequence[i];
    
    // Print which LED is lighting up
    Serial.print("LED ");
    Serial.println(ledIndex);
    
    // Light up the LED using PWM for the current player
    if (currentPlayer == 1) {
      analogWrite(ledsPlayer1[ledIndex], ledBrightness);
    } else {
      analogWrite(ledsPlayer2[ledIndex], ledBrightness);
    }
    
    delay(displayDelay);
    
    // Turn off the LED
    if (currentPlayer == 1) {
      analogWrite(ledsPlayer1[ledIndex], 0);
    } else {
      analogWrite(ledsPlayer2[ledIndex], 0);
    }
    
    delay(pauseDelay);
  }
}

bool getPlayerInput() {
  Serial.println("Waiting for player input...");
  
  // Clear previous inputs
  for (int i = 0; i < sequenceLength; i++) {
    playerInputs[i] = -1;
  }
  
  // Get player's input for each step in the sequence
  for (int step = 0; step < sequenceLength; step++) {
    // Wait for a button press
    int buttonPressed = waitForChoice();
    
    if (buttonPressed == -1) {
      Serial.println("No button pressed (timeout)");
      return false;
    }
    
    playerInputs[step] = buttonPressed;
    
    // Flash the LED that was pressed using PWM
    const int* leds = (currentPlayer == 1) ? ledsPlayer1 : ledsPlayer2;
    analogWrite(leds[buttonPressed], ledBrightness);
    delay(displayDelay / 2);
    analogWrite(leds[buttonPressed], 0);
    
    // Check if this input was correct
    if (playerInputs[step] != sequence[step]) {
      Serial.println("Wrong button pressed!");
      indicateFailure();
      return false;
    }
    
    delay(pauseDelay);
  }
  
  // All inputs were correct
  indicateSuccess();
  return true;
}

int waitForChoice() {
  const int* buttons = (currentPlayer == 1) ? buttonsPlayer1 : buttonsPlayer2;
  
  // Set up a timeout (10 seconds to make a choice)
  unsigned long startTime = millis();
  unsigned long timeout = 10000;  // 10 seconds
  
  while (millis() - startTime < timeout) {
    // Check each button
    for (int i = 0; i < numLeds; i++) {
      if (digitalRead(buttons[i]) == LOW) {  // Button is pressed (LOW because of pull-up)
        Serial.print("Button ");
        Serial.print(i);
        Serial.println(" pressed");
        
        // Wait for button release to prevent multiple reads
        while (digitalRead(buttons[i]) == LOW) {
          delay(10);
        }
        
        return i;
      }
    }
    delay(10);  // Small delay to prevent CPU hogging
  }
  
  // If we get here, there was a timeout
  Serial.println("Input timeout!");
  return -1;
}

bool checkAnyButtonPress() {
  // Check if any button is pressed on either player's controls
  for (int i = 0; i < numLeds; i++) {
    if (digitalRead(buttonsPlayer1[i]) == LOW || digitalRead(buttonsPlayer2[i]) == LOW) {
      // Wait for release
      delay(100);
      return true;
    }
  }
  return false;
}

void turnOffAllLeds() {
  // Turn off all LEDs for both players
  for (int i = 0; i < numLeds; i++) {
    analogWrite(ledsPlayer1[i], 0);
    analogWrite(ledsPlayer2[i], 0);
  }
}

void indicateSuccess() {
  Serial.println("Sequence correct!");
  
  // Flash all LEDs in sequence for the current player
  const int* leds = (currentPlayer == 1) ? ledsPlayer1 : ledsPlayer2;
  
  for (int i = 0; i < numLeds; i++) {
    analogWrite(leds[i], ledBrightness);
    delay(100);
  }
  
  delay(200);
  
  for (int i = 0; i < numLeds; i++) {
    analogWrite(leds[i], 0);
    delay(100);
  }
}

void indicateFailure() {
  Serial.println("Sequence incorrect!");
  
  // Flash all LEDs simultaneously 3 times for the current player
  const int* leds = (currentPlayer == 1) ? ledsPlayer1 : ledsPlayer2;
  
  for (int flash = 0; flash < 3; flash++) {
    // Turn all LEDs on
    for (int i = 0; i < numLeds; i++) {
      analogWrite(leds[i], ledBrightness);
    }
    delay(200);
    
    // Turn all LEDs off
    for (int i = 0; i < numLeds; i++) {
      analogWrite(leds[i], 0);
    }
    delay(200);
  }
}

void displayWinner() {
  Serial.println("Game Over!");
  playBuzzer();
  
  if (score1 > score2) {
    Serial.println("Player 1 wins!");
    // Flash Player 1's LEDs
    for (int flash = 0; flash < 5; flash++) {
      for (int i = 0; i < numLeds; i++) {
        analogWrite(ledsPlayer1[i], ledBrightness);
      }
      delay(200);
      for (int i = 0; i < numLeds; i++) {
        analogWrite(ledsPlayer1[i], 0);
      }
      delay(200);
    }
  } else if (score2 > score1) {
    Serial.println("Player 2 wins!");
    // Flash Player 2's LEDs
    for (int flash = 0; flash < 5; flash++) {
      for (int i = 0; i < numLeds; i++) {
        analogWrite(ledsPlayer2[i], ledBrightness);
      }
      delay(200);
      for (int i = 0; i < numLeds; i++) {
        analogWrite(ledsPlayer2[i], 0);
      }
      delay(200);
    }
  } else {
    Serial.println("It's a tie!");
    // Flash both players' LEDs
    for (int flash = 0; flash < 5; flash++) {
      for (int i = 0; i < numLeds; i++) {
        analogWrite(ledsPlayer1[i], ledBrightness);
        analogWrite(ledsPlayer2[i], ledBrightness);
      }
      delay(200);
      for (int i = 0; i < numLeds; i++) {
        analogWrite(ledsPlayer1[i], 0);
        analogWrite(ledsPlayer2[i], 0);
      }
      delay(200);
    }
  }
  
  Serial.print("Final score - Player 1: ");
  Serial.print(score1);
  Serial.print(", Player 2: ");
  Serial.println(score2);
  Serial.println("Press any button to restart");
}

void resetGame() {
  Serial.println("Resetting game...");
  sequenceLength = 1;
  gameOver = false;
  score1 = 0;
  score2 = 0;
  turnOffAllLeds();
  delay(1000);
  Serial.println("New game starting!");
}

void playBuzzer() {
  // iterate over the notes of the melody.
  // Remember, the array is twice the number of notes (notes + durations)
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzer);
  }
}