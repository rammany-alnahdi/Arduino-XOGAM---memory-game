//OUTPUT PINS - Control the external hardware
const int A=13;        // Bit 0 of 4-bit output (LSB)
const int B=12;        // Bit 1 of 4-bit output
const int C=11;        // Bit 2 of 4-bit output
const int D=10;        // Bit 3 of 4-bit output (MSB)
const int E=9;         // Player indicator: LOW=X, HIGH=O
const int clk=8;       // Clock signal to latch the output
const int reset_ff = 7;// Reset pin to clear all flip-flops

//INPUT PINS - Read player input
const int BUTTONS[] = {2, 3, 4, 5}; // 4-bit input buttons A,B,C,D
const int NUM_BUTTONS = 4;

// GAME STATE VARIABLES
char XOMAP[9] = {      // Tic Tac Toe board (3x3 grid)
  '-','-','-',         // '-'=empty, 'x'=player X, 'o'=player O
  '-','-','-', 
  '-','-','-'
};
char player = 'x';     // Current player: 'x' or 'o'
bool gamedone = false; // Game over flag
bool singlePlayer = false; // Single player mode flag

// INSTRUCTION SYSTEM - For win animations and special effects
int Instruction[50] = {-1}; // Array to store cell patterns for animations
int instructionCount = 0;   // Number of active instructions
char winPlayer = ' ';       // Stores which player won ('x' or 'o')

// YAZAN LETTER PATTERNS - Display name on 3x3 grid
// Each letter defined by lit cells, 0=separator between letters
int yazanSequence[] = {
    // Y: Cells 1,3,5,8 form letter Y
    1, 3, 5, 8, 0,
    // A: Cells 2,4,5,6,7,9 form letter A
    2, 4, 5, 6, 7, 9, 0,
    // Z: Cells 1,2,3,5,7,8,9 form letter Z
    1, 2, 3, 5, 7, 8, 9, 0,
    // A: Cells 2,4,5,6,7,9 form letter A
    2, 4, 5, 6, 7, 9, 0,
    // N: Cells 1,3,4,5,6,7,9 form letter N
    1, 3, 4,5, 6, 7, 9, 0
};

// MEMORY GAME VARIABLES
bool memoryGameMode = false;
int memorySequence[20] = {-1};  // Store the sequence of cells
int memoryLevel = 1;            // Current level (sequence length)
int playerInputIndex = 0;       // Track player's current input position
unsigned long lastFlashTime = 0; // Timing for sequence display
int currentSequenceIndex = 0;   // Track which sequence element to show
bool showingSequence = true;    // Whether we're showing or receiving input

//======================== ARDUINO SETUP ========================
void setup() {
  // Configure OUTPUT pins
  pinMode(A,OUTPUT);       // Data bit 0
  pinMode(B,OUTPUT);       // Data bit 1
  pinMode(C,OUTPUT);       // Data bit 2
  pinMode(D,OUTPUT);       // Data bit 3
  pinMode(E,OUTPUT);       // Player indicator
  pinMode(clk,OUTPUT);     // Clock signal
  pinMode(reset_ff, OUTPUT); // Flip-flop reset

  // Configure INPUT pins with pull-up resistors
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTONS[i], INPUT);
  }
}

// CLOCK EXECUTE - Send clock pulse and reset data lines
void clkexute() {
    digitalWrite(clk, HIGH);  // Rising edge latches data
    delay(10);                // Pulse width
    digitalWrite(clk, LOW);   // Falling edge
    
    // Reset data lines after clock (prepare for next output)
    digitalWrite(A, LOW);
    digitalWrite(B, LOW);
    digitalWrite(C, LOW);
    digitalWrite(D, LOW);
}

// DISPLAY NAME - Show "YAZAN" letter by letter with delays
void displayName() {
    for(int i = 0; i < 36; i++) { // Loop through sequence
        if(yazanSequence[i] == 0) {
            delay(2000);          // Pause between letters
            // Reset display between letters
            digitalWrite(reset_ff, HIGH);
            delay(10);
            digitalWrite(reset_ff, LOW);
        } else {
            // Display cell with alternating E pin (visual effect)
            digitalWrite(E, LOW);
            encodeToPins(yazanSequence[i]);
            clkexute();
            digitalWrite(E, HIGH);
            encodeToPins(yazanSequence[i]);
            clkexute();
        }
    }
    // Final reset after name display
    digitalWrite(reset_ff, HIGH);
    delay(10);
    digitalWrite(reset_ff, LOW);
}

// ENCODE TO PINS - Convert number to 4-bit output and toggle player
void encodeToPins(int number) {
    if (number < 0 || number > 15) return; // Validate input

    // Toggle player only if game is active
    if(gamedone==false)
    if(player == 'x') {
        digitalWrite(E, LOW);   // E=LOW indicates player X
        player = 'o';           // Switch to next player
    } else {
        digitalWrite(E, HIGH);  // E=HIGH indicates player O
        player = 'x';           // Switch to next player
    }
    
    // Output number as 4-bit binary on pins A,B,C,D
    digitalWrite(A, (number & 1) ? HIGH : LOW);   // Bit 0 (1)
    digitalWrite(B, (number & 2) ? HIGH : LOW);   // Bit 1 (2)
    digitalWrite(C, (number & 4) ? HIGH : LOW);   // Bit 2 (4)
    digitalWrite(D, (number & 8) ? HIGH : LOW);   // Bit 3 (8)
}

// FULL RESET - Completely reset game state and hardware
void fullReset() {
    // Clear game board
    for (int i = 0; i < 9; i++) {
        XOMAP[i] = '-';
    }
    player = 'x';           // Reset to player X
    gamedone = false;       // Game can start
    
    // Clear instruction system
    instructionCount = 0;
    for(int i = 0; i < 50; i++) Instruction[i] = -1;
    winPlayer = ' ';
    
    // Reset hardware flip-flops
    digitalWrite(reset_ff, HIGH);
    delay(10);
    digitalWrite(reset_ff, LOW);
}

// CHECK GAME STATE - Detect wins, draws, or continue playing
char checkGameState() {
    // Reset instruction system for new check
    instructionCount = 0;
    for(int i = 0; i < 50; i++) Instruction[i] = -1;
    winPlayer = ' ';
     
    // Check all 3 rows for win
    for (int i = 0; i < 3; i++) {
        if (XOMAP[i*3] != '-' && XOMAP[i*3] == XOMAP[i*3+1] && XOMAP[i*3] == XOMAP[i*3+2]) {
            // Store winning cells for animation
            Instruction[0] = i*3 + 1;     // First cell in winning row
            Instruction[1] = i*3 + 2;     // Second cell in winning row
            Instruction[2] = i*3 + 3;     // Third cell in winning row
            instructionCount = 3;
            winPlayer = XOMAP[i*3];       // Store winner
            return winPlayer;
        }
    }
    
    // Check all 3 columns for win
    for (int i = 0; i < 3; i++) {
        if (XOMAP[i] != '-' && XOMAP[i] == XOMAP[i+3] && XOMAP[i] == XOMAP[i+6]) {
            // Store winning cells for animation
            Instruction[0] = i + 1;       // Top cell in column
            Instruction[1] = i + 4;       // Middle cell in column
            Instruction[2] = i + 7;       // Bottom cell in column
            instructionCount = 3;
            winPlayer = XOMAP[i];
            return winPlayer;
        }
    }
    
    // Check both diagonals for win
    if (XOMAP[0] != '-' && XOMAP[0] == XOMAP[4] && XOMAP[0] == XOMAP[8]) {
        // Top-left to bottom-right diagonal
        Instruction[0] = 1; // Top-left
        Instruction[1] = 5; // Center
        Instruction[2] = 9; // Bottom-right
        instructionCount = 3;
        winPlayer = XOMAP[0];
        return winPlayer;
    }
    if (XOMAP[2] != '-' && XOMAP[2] == XOMAP[4] && XOMAP[2] == XOMAP[6]) {
        // Top-right to bottom-left diagonal
        Instruction[0] = 3; // Top-right
        Instruction[1] = 5; // Center
        Instruction[2] = 7; // Bottom-left
        instructionCount = 3;
        winPlayer = XOMAP[2];
        return winPlayer;
    }
    
    // Check for draw (board full with no winner)
    bool boardFull = true;
    for (int i = 0; i < 9; i++) {
        if (XOMAP[i] == '-') {
            boardFull = false;
            break;
        }
    }
    
    if (boardFull) {
        return 'd'; // Draw game
    }
    
    return 'c'; // Continue playing
}

// X WIN - Animation for player X victory
void xWin() {
    // Reset hardware and set player indicator
    digitalWrite(reset_ff, HIGH);
    delay(10);
    digitalWrite(reset_ff, LOW);
    delay(100);
    digitalWrite(E, LOW); // X player indicator
    
    // Blink winning cells 10 times
    for(int blinkCount = 0; blinkCount < 10; blinkCount++) {
        // Turn ON winning cells
        for(int i = 0; i < instructionCount; i++) {
            if(Instruction[i] != -1) {
                encodeToPins(Instruction[i]);
                clkexute();
            }
        }
        delay(200); // ON time
        
        // Turn OFF all cells
        digitalWrite(reset_ff, HIGH);
        delay(10);
        digitalWrite(reset_ff, LOW);
        delay(200); // OFF time
    }
    
    fullReset(); // Reset game after animation
}

// O WIN - Animation for player O victory
void oWin() {
    // Reset hardware and set player indicator
    digitalWrite(reset_ff, HIGH);
    delay(10);
    digitalWrite(reset_ff, LOW);
    delay(100);
    digitalWrite(E, HIGH); // O player indicator
    
    // Blink winning cells 10 times
    for(int blinkCount = 0; blinkCount < 10; blinkCount++) {
        // Turn ON winning cells
        for(int i = 0; i < instructionCount; i++) {
            if(Instruction[i] != -1) {
                encodeToPins(Instruction[i]);
                
                clkexute();
            }
        }
        delay(200); // ON time
        
        // Turn OFF all cells
        digitalWrite(reset_ff, HIGH);
        delay(10);
        digitalWrite(reset_ff, LOW);
        delay(200); // OFF time
    }
    
    fullReset(); // Reset game after animation
}

// GAME DRAW - Animation for tied game
void gameDraw() {
    // Set instructions to blink all 9 cells
    for(int i = 0; i < 9; i++) Instruction[i] = i+1;
    instructionCount=9;
    
    // Reset hardware
    digitalWrite(reset_ff, HIGH);
    delay(10);
    digitalWrite(reset_ff, LOW);
    delay(100);
    
    // Blink all cells 10 times
    for(int blinkCount = 0; blinkCount < 10; blinkCount++) {
        // Turn ON all cells with correct player indicators
        for(int i = 0; i < instructionCount; i++) {
            // Set E pin based on which player occupied each cell
            if (XOMAP[i]== 'x')
                digitalWrite(E, LOW);  // X's cells
            else
                digitalWrite(E, HIGH); // O's cells
                
            if(Instruction[i] != -1) {
                encodeToPins(Instruction[i]);
                clkexute();
            }
        }
        delay(200); // ON time
        
        // Turn OFF all cells
        digitalWrite(reset_ff, HIGH);
        delay(10);
        digitalWrite(reset_ff, LOW);
        delay(200); // OFF time
    }
    
    fullReset(); // Reset game after animation
}

// GET INPUT - Convert 4 button states to decimal number (0-15)
int getinput(bool input_A, bool input_B, bool input_C, bool input_D) {
    // Binary to decimal conversion for 4-bit input
    if(!input_A && !input_B && !input_C && !input_D) return 0;  // 0000
    else if( input_A && !input_B && !input_C && !input_D) return 1;  // 0001
    else if(!input_A &&  input_B && !input_C && !input_D) return 2;  // 0010
    else if( input_A &&  input_B && !input_C && !input_D) return 3;  // 0011
    else if(!input_A && !input_B &&  input_C && !input_D) return 4;  // 0100
    else if( input_A && !input_B &&  input_C && !input_D) return 5;  // 0101
    else if(!input_A &&  input_B &&  input_C && !input_D) return 6;  // 0110
    else if( input_A &&  input_B &&  input_C && !input_D) return 7;  // 0111
    else if(!input_A && !input_B && !input_C &&  input_D) return 8;  // 1000
    else if( input_A && !input_B && !input_C &&  input_D) return 9;  // 1001
    else if(!input_A &&  input_B && !input_C &&  input_D) return 10; // 1010
    else if( input_A &&  input_B && !input_C &&  input_D) return 11; // 1011
    else if(!input_A && !input_B &&  input_C &&  input_D) return 12; // 1100
    else if( input_A && !input_B &&  input_C &&  input_D) return 13; // 1101
    else if(!input_A &&  input_B &&  input_C &&  input_D) return 14; // 1110
    else if( input_A &&  input_B &&  input_C &&  input_D) return 15; // 1111
    else return -1; // Error case
}

// GET COORDINATES - Process player input and manage game flow
void getCoordinates(int cell_num) {
    // Special command: Reset game (all buttons pressed = 15)
    if (cell_num == 15) {
        fullReset(); // Reset game hardware read
        return;
    }
    // Special command: Display name (button combination = 14)
    else if(cell_num==14){
      displayName();
      return;
    }
    // Special command: Toggle single player mode (button combination = 13)
    else if(cell_num==13){
        if(singlePlayer ==true)
          singlePlayer = false; // Switch to two-player
        else
          singlePlayer = true;  // Switch to single-player
        return;
    }
    else if(cell_num == 12){
        startMemoryGame();
        return;
    }
    // Game move: Cells 1-9 for Tic Tac Toe
    else if(cell_num > 0 && cell_num < 10) {
        if(XOMAP[cell_num-1] == '-') { // Check if cell is empty
            XOMAP[cell_num-1] = player; // Place player's mark
            
            // Check game state after move
            char gameState = checkGameState();
            
            // Handle X win
            if (gameState == 'x') {
                if(gamedone==false){
                  encodeToPins(cell_num); // Show winning move
                  clkexute();
                  gamedone=true;
                }
                xWin(); // Start win animation
            }
            // Handle O win
            else if (gameState == 'o') {
                if(gamedone==false){
                  encodeToPins(cell_num); // Show winning move
                  clkexute();
                  gamedone=true;
                }
                oWin(); // Start win animation
            }
            // Handle draw
            else if (gameState == 'd') {
                if(gamedone==false){
                  encodeToPins(cell_num); // Show final move
                  clkexute();
                  gamedone=true;
                }
                gameDraw(); // Start draw animation
            }
            // Game continues
            else {
                // Normal move - update display
                encodeToPins(cell_num);
                clkexute();
                
                // SINGLE PLAYER MODE: Computer makes move
                if(singlePlayer && !gamedone) {
                    delay(500); // Pause so player can see their move
                    computerMove(); // AI makes its move
                    
                    // Check if computer won
                    gameState = checkGameState();
                    if(gameState == 'o') {
                        gamedone=true;
                        oWin(); // Computer wins
                    }
                    else if(gameState == 'd') {
                        gamedone=true;
                        gameDraw(); // Game tied
                    }
                }
            }
        }
    }
}

// READ ALL INPUTS - Read current state of all 4 buttons
void readAllInputs(bool inputs[]) {
    for (int j = 0; j < NUM_BUTTONS; j++) {
        inputs[j] = (digitalRead(BUTTONS[j]) == HIGH); // true if pressed
    }
}
//======================== GenerateMemorySequence ========================
void generateMemorySequence() {
    unsigned long base = millis();
    
    for(int i = 0; i < memoryLevel; i++) {
        // Use different parts of millis() combined with index
        memorySequence[i] = ((base + i * 137) % 9) + 1;
    }
}
void startMemoryGame() {
    memoryGameMode = true;
    memoryLevel = 1;
    playerInputIndex = 0;
    currentSequenceIndex = 0;
    showingSequence = true;
    lastFlashTime = 0;
    
    // Clear sequence array
    for(int i = 0; i < 20; i++) {
        memorySequence[i] = -1;
    }
    
    // Generate initial sequence
    generateMemorySequence();
    
    // Reset display
    digitalWrite(reset_ff, HIGH);
    delay(10);
    digitalWrite(reset_ff, LOW);
    
    for(int i = 1; i < 11; i++) {
      if(i%2==0)
        displayCell(i, 'x');  // Use 'x' for display
      else
        displayCell(i, 'o');  // Use 'x' for display
        clkexute();
        delay(200);
    }
    delay(500);
    digitalWrite(reset_ff, HIGH);
    delay(10);
    digitalWrite(reset_ff, LOW);
    delay(1000);
}

// SHOW MEMORY SEQUENCE - Display the sequence to memorize
void showMemorySequence() {
    unsigned long currentTime = millis();
    
    if(currentTime - lastFlashTime > 1000) { // 1 second between flashes
        if(currentSequenceIndex < memoryLevel) {
            // Flash the current sequence cell
            int cell = memorySequence[currentSequenceIndex];
            displayCell(cell, 'x');  // Show with 'x' indicator
            clkexute();
            delay(500);  // ON time
            
            // Turn off
            digitalWrite(reset_ff, HIGH);
            delay(10);
            digitalWrite(reset_ff, LOW);
            delay(300);  // OFF time
            
            currentSequenceIndex++;
            lastFlashTime = currentTime;
        } else {
            // Finished showing sequence
            showingSequence = false;
            playerInputIndex = 0;
            currentSequenceIndex = 0;
            
            // Brief pause before player input
            delay(500);
            
            // Show ready indication (quick flash)
            for(int i = 1; i <= 3; i++) {
                displayCell(5, 'o');  // Flash center cell
                clkexute();
                delay(100);
                digitalWrite(reset_ff, HIGH);
                delay(10);
                digitalWrite(reset_ff, LOW);
                delay(100);
            }
        }
    }
}


void displayCell(int number, char displayPlayer) {
    if (number < 0 || number > 15) return;

    // Set E based on specified player, don't toggle
    digitalWrite(E, (displayPlayer == 'x') ? LOW : HIGH);
    
    // Output number as 4-bit binary
    digitalWrite(A, (number & 1) ? HIGH : LOW);
    digitalWrite(B, (number & 2) ? HIGH : LOW);
    digitalWrite(C, (number & 4) ? HIGH : LOW);
    digitalWrite(D, (number & 8) ? HIGH : LOW);
}
void memoryGameOver() {
    // Show wrong cell flash
    for(int i = 0; i < 5; i++) {
        displayCell(memorySequence[playerInputIndex], 'x');
        clkexute();
        delay(500);
        digitalWrite(reset_ff, HIGH);
        delay(10);
        digitalWrite(reset_ff, LOW);
        delay(500);
    }
     delay(1000);
    showMemoryScore();
}
void showMemoryScore() {
    // Flash score (level-1 because they failed the current level)
    int score = memoryLevel - 1;
    for(int i = 0; i < 3; i++) {
        // Display score as binary on first 4 cells
        for(int bit = 0; bit < 4; bit++) {
            if(score & (1 << bit)) {
                displayCell(bit + 1, 'x'); // Cells 1-4 represent bits
                clkexute();
            }
        }
        delay(500);
        digitalWrite(reset_ff, HIGH);
        delay(10);
        digitalWrite(reset_ff, LOW);
        delay(500);
    }
    
    exitMemoryGame();
}
void exitMemoryGame() {
    for(int i = 1; i < 10; i++) {
      if(i%2==0)
        displayCell(i, 'x');  // Use 'x' for display
      else
        displayCell(i, 'o');  // Use 'x' for display
      clkexute();
      delay(200);
    }
    delay(500);
    memoryGameMode = false;
    fullReset(); // Reset to Tic Tac Toe
}
// LEVEL COMPLETE MEMORY - Player completed current level
void levelCompleteMemory() {
    // Celebration flash
    for(int i = 0; i < 3; i++) {
        for(int cell = 1; cell <= 9; cell++) {
            displayCell(cell, 'x');
            clkexute();
        }
        delay(200);
        digitalWrite(reset_ff, HIGH);
        delay(10);
        digitalWrite(reset_ff, LOW);
        delay(200);
    }
    
    // Increase level
    memoryLevel++;
    if(memoryLevel > 20) memoryLevel = 6; // Max level
    
    // Reset for next level
    playerInputIndex = 0;
    currentSequenceIndex = 0;
    showingSequence = true;
    
    // Generate new sequence
    generateMemorySequence();
    
    delay(1000); // Pause before next level
}

// GET COORDINATES MEMORY - Process player input during memory game
void getCoordinatesMemory(int cell_num) {
    
    if(cell_num == 15) {
        // Reset button - exit memory game
        exitMemoryGame();
        return;
    }
    
    if(cell_num > 0 && cell_num < 10) {
        // Player pressed a valid cell
        // Show the pressed cell briefly
        displayCell(cell_num,'o');
        clkexute();
        delay(300);
        digitalWrite(reset_ff, HIGH);
        delay(10);
        digitalWrite(reset_ff, LOW);
        
        // Check if correct
        if(cell_num == memorySequence[playerInputIndex]) {  //[3,5,7,2] cell_num ==7 && playerInputIndex ==2]=> true
            playerInputIndex++;
            
            // Check if level complete
            if(playerInputIndex >= memoryLevel) {
                // Level completed!
                levelCompleteMemory();
            }
        } else {
            // Wrong input - game over
            memoryGameOver();
        }
    }
}
void runMemoryGame() {
    if(showingSequence) {
        showMemorySequence();
    } else {
        // Waiting for player input - handled in getCoordinatesMemory
    }
}

//======================== ARTIFICIAL INTELLIGENCE ========================

// COMPUTER MOVE - AI decides where to play
void computerMove() {
    int move = -1;
    
    // Count moves to determine game phase
    int moveCount = 0;
    for(int i = 0; i < 9; i++) {
        if(XOMAP[i] != '-') moveCount++;
    }
    
    // Early game: random moves for variety
    if(moveCount < 2) {
        do {
            move = millis() % 9; // Random position
        } while(XOMAP[move] != '-');
    }
    // Mid/late game: strategic moves
    else {
        // Strategy priority: 1. Win, 2. Block, 3. Center, 4. Corner, 5. Random
        move = findWinningMove('o'); // Try to win
        if(move == -1) move = findWinningMove('x'); // Block player win
        if(move == -1) move = takeCenter(); // Take center if available
        if(move == -1) move = takeCorner(); // Take corner if available
        if(move == -1) move = randomEmptyCell(); // Random empty cell
    }
    
    // Execute the chosen move
    if(move != -1 && XOMAP[move] == '-') {
        XOMAP[move] = 'o';
        encodeToPins(move + 1);
        clkexute();
    }
}

// FIND WINNING MOVE - Check if player can win with one move
int findWinningMove(char player) {
    // Test each empty cell to see if it creates a win
    for(int i = 0; i < 9; i++) {
        if(XOMAP[i] == '-') {
            // Simulate move
            XOMAP[i] = player;
            char result = checkGameState();
            XOMAP[i] = '-'; // Undo simulation
            
            if(result == player) {
                return i; // This move wins
            }
        }
    }
    return -1; // No winning move found
}

// TAKE CENTER - Prefer center position
int takeCenter() {
    if(XOMAP[4] == '-') return 4; // Center cell
    return -1;
}

// TAKE CORNER - Prefer corner positions
int takeCorner() {
    int corners[] = {0, 2, 6, 8}; // Four corner positions
    // Find first available corner
    for(int i = 0; i < 4; i++) {
        if(XOMAP[corners[i]] == '-') {
            return corners[i];
        }
    }
    return -1; // No corners available
}

// RANDOM EMPTY CELL - Choose any available cell randomly
int randomEmptyCell() {
    // Collect all empty cells
    int emptyCells[9];
    int count = 0;
    
    for(int i = 0; i < 9; i++) {
        if(XOMAP[i] == '-') {
            emptyCells[count] = i;
            count++;
        }
    }
    
    // Return random empty cell if available
    if(count > 0) {
        return emptyCells[millis() % count];
    }
    return -1; // No empty cells (shouldn't happen)
}

//======================== MAIN LOOP ========================
void loop() {
    bool currentInputs[4] = {false, false, false, false};

    
    // Read current button states
    readAllInputs(currentInputs);
    // If in memory game mode, run memory game logic
    if(memoryGameMode) {
        runMemoryGame();
    }
    // Check each button for press events
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (currentInputs[i] == true) {
            // Button pressed - debounce delay
            delay(50);
            
            // Wait for button release
            while (digitalRead(BUTTONS[i]) == LOW) {
                delay(10);
            }
            
            // Read final input state after release
            readAllInputs(currentInputs);
            // Convert 4-bit input to decimal cell number
            int cell = getinput(currentInputs[0], currentInputs[1], currentInputs[2], currentInputs[3]);
             if(memoryGameMode) {
                getCoordinatesMemory(cell);
            } else {
                getCoordinates(cell);
            }
            // Prevent multiple reads from same press
            delay(200);
            break; // Process only one button per loop
        }
    }
}
