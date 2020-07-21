/** Code for Side Project's adaptation of Samy Kamkar's Combo Breaker
 *  Notes:
 *    - Stepper has 200 steps total
 *    - Microstep modes (MS1, MS2): full (0,0), half (1,0), 1/4 (0,1), 1/8 (1,1)
 *    - Each step = 1.8 degrees, each digit on lock = 9 degrees, so each number = 5 steps
 *    
 *  Hardware Notes:
 *    - 20 grooves around head, 12.8 mm diameter  
 *    
**/

// these pins are connected to the various pins on the EasyDriver, and can be any digital pin you want
int stepPin = 9;
int dirPin = 10;
int ms1Pin = 7;
int ms2Pin = 6;

bool CW = true; //relative to dial
bool CCW = false; //relative to dial

//stores the position of the dial
int dialPosition = 0;
bool turnDir = CW;

void setup() {               
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(ms1Pin, OUTPUT);
  pinMode(ms2Pin, OUTPUT);

  digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);

  //sets microstep mode to full step
  digitalWrite(ms1Pin, LOW);
  digitalWrite(ms2Pin, LOW);
  
  Serial.begin(9600);

}

String dataEntered = "";
void loop() {
  //im aware that this is messy, but to sum up the functionality: entering one of the commands below 
  //in the serial monitor/bluetooth serial will control the position of the lock dial
  /**
  CW or CCW - sets direction to turn when a TURN command or integer is entered (automatically CW)
  integer - turns to that integer on the lock dial in the direction specified by turnDir
  TURN - will turn a specified number of turns in the specified direction
  TRY - enters a specified combination
  CRACK - attempts to open a lock with an unknown combo based on the 3 "resistance points" (refer to Samy Kamkar's video for these)
  
  **/
  
  if (Serial.available() > 0) {
    dataEntered = Serial.readString();
    Serial.println(dataEntered);
    if (dataEntered.startsWith("CW")) {
      turnDir = CW;
      Serial.println("Turning CW");
    } 
    
      else if (dataEntered.startsWith("CCW")) {
      turnDir = CCW;  
      Serial.println("Turning CCW");
    } 
    
      else if (dataEntered.startsWith("TURN")) {
      Serial.println("Enter number of turns:");
      while(Serial.available() == 0){}
      int turnNum = Serial.readString().toInt();
      Serial.println("Turning");
      Serial.println(turnNum);
      fullTurn(turnNum, turnDir);
    } 

      else if (dataEntered.startsWith("TRY")) {
        Serial.println("Enter 1st Digit:");
        while(Serial.available() == 0){}
        int first = Serial.readString().toInt();
        Serial.println("Enter 2nd Digit:");
        while(Serial.available() == 0){}
        int second = Serial.readString().toInt();
        Serial.println("Enter 3rd Digit:");
        while(Serial.available() == 0){}
        int third = Serial.readString().toInt();
        enterCombo(first,second,third);
    }

      else if (dataEntered.startsWith("CRACK")) {
        Serial.println("Enter Resistance:");
        while(Serial.available() == 0){}
        int first = Serial.readString().toInt();
        Serial.println("Enter 1st Notch:");
        while(Serial.available() == 0){}
        int second = Serial.readString().toInt();
        Serial.println("Enter 2nd Notch:");
        while(Serial.available() == 0){}
        int third = Serial.readString().toInt();
        crackCombo(first,second,third);
    }
    
      else {
      int stepNum = dataEntered.toInt();
      turnTo(stepNum, turnDir);
    }
  }
}

//takes a specified number of steps in either cw or ccw (ccw is positive, cw is negative in relation to dial. reverse for face up testing)
void step(int num, bool dir) {
  if (dir == CCW) {
    //ccw relative to lock
    digitalWrite(dirPin, HIGH);
  } else {
    //cw relative to lock
    digitalWrite(dirPin, LOW);  
  }

  for (int i = 0; i < abs(num); i++) {
    digitalWrite(stepPin, HIGH);
    delay(1);         
    digitalWrite(stepPin, LOW);
    delay(1); 
    
  }
  //Serial.println(num);
}


//takes a specified number of digits to turn in either cw or ccw
void turnDigits(int num, bool dir) {
  step(num*5, dir);
}

//turns to a specified number, uses bool as direction. reverse for face up testing
void turnTo(int dest, bool dir) {
  int turnDistance;
  Serial.print("Current Position:");
  Serial.println(dialPosition);

  //puts destination within 0 - 40 range
  dest = limitInDial(dest);


  if (dir == CW) {
    turnDistance = (40 - dest) + dialPosition;
    turnDistance = limitInDial(turnDistance);
  } else if (dir == CCW) {
    turnDistance = dest - dialPosition;
    turnDistance = limitInDial(turnDistance);
  }

  turnDigits(turnDistance, dir);
  dialPosition = dest;
  
  Serial.print("New Position:");
  Serial.println(dialPosition);
}


//puts an int within 0 <= x <40
int limitInDial(int x) {
  while (x >= 40) {
    x-=40;
  }
  while (x < 0) {
    x+=40;  
  }
  return x;  
}

//turns a set # of full rotations in a specified direction
void fullTurn(int num, bool dir) {
    turnDigits(40*num, dir);
}

//enters a combo on the lock
void enterCombo(int num1, int num2, int num3) {
  //step 1
  fullTurn(3,CW);
  turnTo(num1,CW);
  //step 2
  fullTurn(1,CCW);
  turnTo(num2,CCW);
  //step 3
  turnTo(num3,CW);
}

//tries possible combinations from first and third number resistance points
void crackCombo(int num1, int num2, int num3) {
  //gets first digit
  int digit1 = num1 + 6;
  
  //finds possible 3rd numbers
  int possible3Set1[4] = {num2, num2+10, num2+20, num2+30};
  int possible3Set2[4] = {num3, num3+10, num3+20, num3+30};
  int digit3[2];
  //position in digit3 array
  int pos = 0;
  
  for (int i = 0; i < 4; i++) {
    if (possible3Set1[i]%4 == digit1%4) {
      digit3[pos] = possible3Set1[i];
      pos++;
    }
  }
  for (int i = 0; i < 4; i++) {
    if (possible3Set2[i]%4 == digit1%4) {
      digit3[pos] = possible3Set2[i];
      pos++;
    }
  }

  //gives possible 2nd numbers
  int tenPossible2[10];
  int shiftedMod = (digit1%4)-2;
  if(shiftedMod < 0) {
    shiftedMod = 4 + shiftedMod;
  }
  
  int digit2Pos = 0;
  for (int i = 1; i <= 40; i++) {
    if (i%4 == shiftedMod) {
      tenPossible2[digit2Pos] = i;
      digit2Pos++;
    }
  }
  Serial.println(shiftedMod);
//  Serial.println(digit2Pos);
  Serial.println("DIGIT 1:");
  Serial.println(digit1);
  Serial.println("DIGIT 2:");
  for (int i = 0; i < 10; i++) {
    Serial.println(tenPossible2[i]);
  }
  Serial.println("DIGIT 3:");
  for (int i = 0; i < 2; i++) {
    Serial.println(digit3[i]);
  }
  //start moving
  //step 1
  turnTo(0, CW);
  fullTurn(3, CW);
  turnTo(digit1, CW);
  
  //begin the testing process
  fullTurn(1, CCW);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 10; j++) {
      if (!(digit3[i] + 2 == tenPossible2[j]) && !(digit3[i] - 2 == tenPossible2[j])) {
        turnTo(tenPossible2[j], CCW);
        turnTo(digit3[i], CW);
        delay(3000);
      }
    }
  }
  
}
