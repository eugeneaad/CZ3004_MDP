#include <PinChangeInt.h>
#include <DualVNH5019MotorShield.h>
#include <RunningMedian.h>
#include <SharpIR.h>

//Sharp IR sensor define
SharpIR sensorFR(GP2Y0A21YK0F, A0);
SharpIR sensorFL(GP2Y0A21YK0F, A1);
SharpIR sensorFC(GP2Y0A21YK0F, A3);
//SharpIR sensorR(GP2Y0A02YK0F, A2);
#define sensorR A2
SharpIR sensorLF(GP2Y0A21YK0F, A5);
SharpIR sensorLB(GP2Y0A21YK0F, A4);

//motor pins
DualVNH5019MotorShield md(4, 2, 6, A0, 7, 8, 12, A1);

//Median variables
static RunningMedian FrontR = RunningMedian(50);
static RunningMedian FrontL = RunningMedian(50);
static RunningMedian FrontC = RunningMedian(50);
static RunningMedian Right = RunningMedian(100);
static RunningMedian LeftF = RunningMedian(50);
static RunningMedian LeftB = RunningMedian(50);


volatile int mLTicks = 0;
volatile int mRTicks = 0;

//Global var declaration
boolean flag = false;         //for starting alligment
String inString = "";         //for cmd
double previousLF;               //to record the previous value of Left sensor for calibration
double previousR;                //to record the previous value of Right sensor for calibration
int adjustFrontFailCount = 0; //for check fail to adjust front
int adjustFailCount = 0;      //for check fail to adjust
double disFL, disFC, disFR, disLF, disLB, disR;   //for sensor calibration
int FL, FC, FR, LF, LB, R;    //for sensor feedback to pc

void setup() {
  // put your setup code here, to run once:
  pinMode(4, INPUT);  //Interrupt Pin 4
  pinMode(13, INPUT); //Interrupt Pin 13

  md.init();    // init robot

  PCintPort::attachInterrupt(11, &compute_mL_ticks, RISING);  //Attached to Pin 11
  PCintPort::attachInterrupt(3, &compute_mR_ticks, RISING); //Attached to Pin 3

  Serial.begin(115200);
  Serial.setTimeout(20);        // for reading string time out intervel
  //  Serial.println("Waiting for data: ");
}

int moveCount = 0;    //counter

void loop() {
  // put your main code here, to run repeatedly:

//  getSensorsDataActual();

  // Read string from serial
  while (Serial.available() > 0)
  {
    inString = Serial.readString();
  }

  // Only print when it is not empty string
  if (inString != "")
  {
    String decodeString;
    //    Serial.println(inString);
    char cmd = inString.charAt(0);
    //    Serial.println(cmd);
    for (int i = 1; i < inString.length(); i++)
    {
      decodeString += inString.charAt(i);
    }
    int target = decodeString.toInt();
    //    Serial.println(target);

    runCMD(cmd, target);

  }

  // Clear the string
  inString = "";
  delay(40);
}

void fastExplore(String str)
{
  char cmd;
  String cdis;
  int dis = 0;

  for (int i = 1; i < str.length(); i++)
  {
    if (str[i] == 'M')
    {
      cmd = 'M';
      i++;
      cdis = str[i];
      i++;
      cdis += str[i];
    }
    else
    {
      cmd = str[i];
    }
    dis = cdis.toInt();
    //    Serial.println(cmd);
    //    Serial.println(dis);

    switch (cmd)
    {
      case 'M':
        fastForward(dis);
        //        Serial.println(dis);
        break;
      case 'R':
        turnRightFast(90);
        //        Serial.println("Fast R");
        break;
      case 'L':
        turnLeftFast(90);
        //        Serial.println("Fast L");
        break;
      case 'U':
        uTurn();
        //        Serial.println("Fast U");
        break;
    }
    cdis = "";
    dis = 0;
    checkForCalibration(cmd);
    delay(40);
  }
  getSensorsData(cmd);
}

void fastPath(String str)
{
  char cmd;
  String cdis;
  int dis = 0;

  for (int i = 1; i < str.length(); i++)
  {
    if (str[i] == 'M')
    {
      cmd = 'M';
      i++;
      cdis = str[i];
      i++;
      cdis += str[i];
    }
    else
    {
      cmd = str[i];
    }
    dis = cdis.toInt();
    //    Serial.println(cmd);
    //    Serial.println(dis);

    switch (cmd)
    {
      case 'M':
        fastForward(dis);
        //        Serial.println(dis);
        break;
      case 'R':
        turnRightFast(90);
        //        Serial.println("Fast R");
        break;
      case 'L':
        turnLeftFast(90);
        //        Serial.println("Fast L");
        break;
      case 'U':
        uTurn();
        //        Serial.println("Fast U");
        break;
    }
    cdis = "";
    dis = 0;
    delay(40);
  }

  Serial.println("BS");
}

//*cmd
void runCMD(char cmd, int target)
{
  switch (cmd)
  {
    case 'M':
      moveSpeedup(target);
      getSensorsData(cmd);
      break;
    case 'R':
      turnRightFast(target);
      getSensorsData(cmd);
      break;
    case 'L':
      turnLeftFast(target);
      getSensorsData(cmd);
      break;
    case 'U':
      uTurn();
      getSensorsData(cmd);
      break;
    case 'S':
      moveBack(target);
      getSensorsData(cmd);
      break;
    case 'A':
      turnLeftFast(90);
      autoCalibrate();
      delay(20);
      turnRightFast(90);
      autoCalibrate();
      delay(20);
      //      getSensorsData();
      Serial.println("POK");
      break;
    case 'Q':
      turnLeftFast(90);
      autoCalibrate();
      delay(20);
      turnRightFast(90);
      //      getSensorsData();
      Serial.println("POK");
      break;
    case 'E':
      turnRightFast(90);
      autoCalibrate();
      delay(20);
      turnLeftFast(90);
      //      getSensorsData();
      Serial.println("POK");
      break;
    case 'C':
      adjustDistance();
      adjustAngleLeft();
      adjustAngleFront();
      //      getSensorsData();
      Serial.println("POK");
      break;
    case 'Z':
      fastExplore(inString);
      break;
    case 'X':
      fastPath(inString);
      break;
    case 'D':
      getSensorsData(cmd);
      break;
    case 'P':
      //for exploration completed calibration
      autoCalibrate();
      turnRightFast(90);
      autoCalibrate();
      turnRightFast(90);
      for (int i = 0; i < 10; i++)
      {
        autoCalibrate();
        delay(100);
      }
      Serial.println("POK");
      break;

    //debug function
    case '/':
      stairCaseTest();
      break;
    case 'F':
      getSensorsDataFront();
      break;
    case 'G':
      getSensorsDataLeft();
      break;
    case 'H':
      getSensorsDataDistanceAdjust();
      break;
    case 'J':
      getSensorsDataActual();
      break;
    case 'K':
      getSensorsDataStairs();
      break;
    default:
      break;
  }

}


//*pid
int pidControl(int LeftPosition, int RightPosition) {

  int error;
  int prev_error;
  double integral, derivative, output;
  double Kp = 3;
  double Kd = 0;
  double Ki = 0;

  error = LeftPosition - RightPosition;
  integral += error;
  derivative = (error - prev_error);

  output = Kp * error + Ki * integral + Kd * derivative;
  prev_error = error;
  return output;
}

//*msu
void moveSpeedup(int dis)
{

  mLTicks = 0;
  mRTicks = 0;
  double dTotalTicks = 0;
  double output;
  //      int count = 0;
  //      double avg, total = 0;

  //  int pwm1 = 282, pwm2 = 330;   Week 8
  int pwm1 = 375, pwm2 = 363;   //Battery 1
  //  int pwm1 = 360, pwm2 = 380; //Battery 2

  dTotalTicks = 251;

  while (mLTicks < dTotalTicks)
  {

    output = pidControl(mLTicks, mRTicks);
    md.setSpeeds(pwm1 - output * 5, pwm2 + output * 5);    //check coiffient for debug
    //            count++;
    //            total += abs(mLTicks - mRTicks);
    //            Serial.println(mLTicks - mRTicks);
    //            Serial.print(mLTicks);
    //            Serial.print("/");
    //            Serial.println(mRTicks);
  }
  //      avg = total / count;
  //      Serial.print("Avg:");
  //      Serial.println(avg);

  fbrake();

  //  movementCount++;
  //  if ( movementCount >= 5)
  //  {
  //    while (mLTicks < 12)
  //      md.setSpeeds(200, 0);
  //    movementCount = 0;
  //  }
  //  fbrake();
}

void fastForward(int dis)
{

  mLTicks = 0;
  mRTicks = 0;

  double dTotalTicks = 0;
  double output;
  //  int count = 0;
  //  double avg, total = 0;

  int pwm1 = 375, pwm2 = 380;

  if (dis <= 1)
  {
    pwm1 = 375, pwm2 = 358;   //Battery 1
    dTotalTicks = 251;
  }
  else if (dis == 2 )
  {
    pwm1 = 375, pwm2 = 358;   //Battery 1
    dTotalTicks = 264 * dis;
  }
  else if (dis == 3)
  {
    dTotalTicks = 277 * dis;
  }
  else if (dis == 4)
  {
    dTotalTicks = 285 * dis;
  }
  else if (dis == 5)
  {
    dTotalTicks = 286 * dis;
  }
  else if (dis == 6)
  {
    dTotalTicks = 287 * dis;
  }
  else if (dis == 7)
  {
    dTotalTicks = 288 * dis;
  }
  else if (dis == 8)
  {
    dTotalTicks = 289 * dis;
  }
  else if (dis == 9)
  {
    dTotalTicks = 290 * dis;
  }
  else if (dis == 10)
  {
    dTotalTicks = 290.5 * dis;
  }
  else if (dis == 11)
  {
    dTotalTicks = 291 * dis;
  }
  else if (dis == 12)
  {
    dTotalTicks = 292 * dis;
  }
  else if (dis == 13)
  {
    dTotalTicks = 292 * dis;
  }
  else if (dis == 14)
  {
    dTotalTicks = 292.5 * dis;
  }
  else if (dis == 15)
  {
    dTotalTicks = 293 * dis;
  }
  else if (dis == 16)
  {
    dTotalTicks = 296.5 * dis;
  }
  else if (dis == 17)
  {
    dTotalTicks = 294 * dis;
  }


  while (mLTicks < dTotalTicks)
  {
    output = pidControl(mLTicks, mRTicks);
    md.setSpeeds(pwm1 - output * 5, pwm2 + output * 5);    //check coiffient for debug
    //    count++;
    //    total += abs(mLTicks - mRTicks);
    //    Serial.println(mLTicks - mRTicks);
    //    Serial.print(mLTicks);
    //    Serial.print("/");
    //    Serial.println(mRTicks);
  }
  //  avg = total / count;
  //  Serial.print("Avg:");
  //  Serial.println(avg);
  fbrake();
}

void fbrake() {

  for (int i = 0; i < 10; i++)
  {
    //    md.setBrakes(390, 400);
    md.setBrakes(368, 400);

  }

  delay(100);

}

//*b
void brake() {

  for (int i = 0; i < 3; i++)
  {
    md.setBrakes(400, 400);
  }

  delay(100);

}

void brakeabit() {
  for (int i = 0; i < 3; i++)
  {
    md.setBrakes(400, 400);
  }

  delay(10);

}

//*tr
//void turnRight(int degree) {
//
//  double dTotalTicks = 0;
//  double output;
//  //  int count = 0;
//  //  double avg, total = 0;
//  //  int pwm1 = 355, pwm2 = -330;        //Battery 1
//  int pwm1 = 344, pwm2 = -316;
//
//  dTotalTicks = 359;     //Battery 1
//  //    dTotalTicks = 357;        //Battery 2
//  while (mLTicks < dTotalTicks)
//  {
//
//    //    output = pidControl(mLTicks, mRTicks);
//
//    //    md.setSpeeds(pwm1 + output * 3, pwm2 - output * 3);
//    md.setSpeeds(pwm1 , pwm2);
//    //    count++;
//    //    total += abs(mLTicks - mRTicks);
//    //    Serial.println(mLTicks - mRTicks);
//    //    Serial.print(mLTicks);
//    //    Serial.print("/");
//    //    Serial.print(mRTicks);
//    //    Serial.print("/");
//    //    Serial.println(dTotalTicks);
//  }
//  //  avg = total / count;
//  //  Serial.print("Avg:");
//  //  Serial.println(avg);
//
//  brake();
//}
//
//
//void turnLeft(int degree) {
//  double dTotalTicks = 0;
//  double output;
//  //  int count = 0;
//  //  double avg, total = 0;
//  //  int pwm1 = -355, pwm2 = 316;    //Battery 1
//  int pwm1 = -344, pwm2 = 316;
//  //    int pwm1 = -355, pwm2 = 326;        //Battery 2
//
//  dTotalTicks = 359;    //Battery 1
//  //    dTotalTicks = 360;      //Battery 2
//
//  while (mLTicks < dTotalTicks)
//  {
//    //    output = pidControl(mLTicks, mRTicks);
//    //    md.setSpeeds(pwm1 - output * 3, pwm2 + output * 3);
//    md.setSpeeds(pwm1, pwm2);
//    //    count++;
//    //    total += abs(mLTicks - mRTicks);
//    //    Serial.println(mLTicks - mRTicks);
//    //    Serial.print(mLTicks);
//    //    Serial.print("/");
//    //    Serial.print(mRTicks);
//    //    Serial.print("/");
//    //    Serial.println(dTotalTicks);
//  }
//  //  avg = total / count;
//  //  Serial.print("Avg:");
//  //  Serial.println(avg);
//
//  brake();
//}


//*tr
void turnRightFast(int degree) {

  mLTicks = 0;
  mRTicks = 0;
  double dTotalTicks = 0;
  double output;
  //    int count = 0;
  //    double avg, total = 0;

  int pwm1 = 344, pwm2 = -316;
  //  int pwm1 = 380, pwm2 = -360;
  //  dTotalTicks = 349;
  dTotalTicks = 350;

  while (mLTicks < dTotalTicks || mRTicks < dTotalTicks)
  {
    output = pidControl(mLTicks, mRTicks);
    md.setSpeeds(pwm1 + output * 3, pwm2 - output * 3);
    //        count++;
    //        total += abs(mLTicks - mRTicks);
    //        Serial.println(mLTicks - mRTicks);
    //        Serial.print(mLTicks);
    //        Serial.print("/");
    //        Serial.print(mRTicks);
    //        Serial.print("/");
    //        Serial.println(dTotalTicks);
  }
  //    avg = total / count;
  //    Serial.print("Avg:");
  //    Serial.println(avg);

  brake();
}


void turnLeftFast(int degree) {
  mLTicks = 0;
  mRTicks = 0;

  double dTotalTicks = 0;
  double output;
  //    int count = 0;
  //    double avg, total = 0;

  int pwm1 = -344, pwm2 = 316;
  //  int pwm1 = -380, pwm2 = 360;

  //  dTotalTicks =XM02 348;      //Battery 2
  dTotalTicks = 345;
  while (mLTicks < dTotalTicks || mRTicks < dTotalTicks)
  {
    output = pidControl(mLTicks, mRTicks);
    md.setSpeeds(pwm1 - output * 3, pwm2 + output * 3);

    //        count++;
    //        total += abs(mLTicks - mRTicks);
    //        Serial.println(mLTicks - mRTicks);
    //        Serial.print(mLTicks);
    //        Serial.print("/");
    //        Serial.print(mRTicks);
    //        Serial.print("/");
    //        Serial.println(dTotalTicks);
  }
  //    avg = total / count;
  //    Serial.print("Avg:");
  //    Serial.println(avg);

  brake();
}

void uTurn() {
  mLTicks = 0;
  mRTicks = 0;

  double dTotalTicks = 0;
  double output;
  //  int count = 0;
  //  double avg, total = 0;

    int pwm1 = -344, pwm2 = 316;
//  int pwm1 = 380, pwm2 = -380;
  dTotalTicks = 740;

  while (mLTicks < dTotalTicks || mRTicks < dTotalTicks)
  {
    output = pidControl(mLTicks, mRTicks);
    md.setSpeeds(pwm1 - output * 3, pwm2 + output * 3);
    //    md.setSpeeds(pwm1, pwm2);
    //    count++;
    //    total += abs(mLTicks - mRTicks);
    //    Serial.println(mLTicks - mRTicks);
    //    Serial.print(mLTicks);
    //    Serial.print("/");
    //    Serial.print(mRTicks);
    //    Serial.print("/");
    //    Serial.println(dTotalTicks);
  }
  //  avg = total / count;
  //  Serial.print("Avg:");
  //  Serial.println(avg);

  brake();
}

void turnRightabit(int degree) {
  mLTicks = 0;
  mRTicks = 0;

  double dTotalTicks = 0;
  double output;

  int pwm1 = 355, pwm2 = -330;        //Battery 1

  dTotalTicks = 1; // 45 degree
  while (mLTicks < dTotalTicks)
  {
    //    output = pidControl(mLTicks, mRTicks);
    //    md.setSpeeds(pwm1 + output * 3, pwm2 - output * 3);
    md.setSpeeds(pwm1 , pwm2 );
  }
  brakeabit();
}


void turnLeftabit(int degree) {
  mLTicks = 0;
  mRTicks = 0;
  double dTotalTicks = 0;
  double output;

  int pwm1 = -355, pwm2 = 316;    //Battery 1
  dTotalTicks = 1; // 45 degree

  while (mLTicks < dTotalTicks)
  {
    //    output = pidControl(mLTicks, mRTicks);
    //    md.setSpeeds(pwm1 - output * 3, pwm2 + output * 3);
    md.setSpeeds(pwm1 , pwm2 );
  }
  brakeabit();
}


void moveBack(int dis)
{
  mLTicks = 0;
  mRTicks = 0;
  double dTotalTicks = 0;
  double output;
  //  int count = 0;
  //  double avg, total = 0;
  //  int pwm1 = -355, pwm2 = -315;
  int pwm1 = -375, pwm2 = -354;
  if (dis <= 1)
  {
    dTotalTicks = 285;  // 1 box
  }
  else if (dis > 1 && dis <= 3 )
  {
    dTotalTicks = 285 * dis;  // 1 to 3 box
  }
  else if (dis > 3 && dis <= 5)
  {
    dTotalTicks = 285 * dis;  // 3 to 5 box
  }
  else if (dis > 5 && dis <= 7)
  {
    dTotalTicks = 285 * dis;  //5 to 7 box
  }
  else if (dis > 7 && dis <= 10)
  {
    dTotalTicks = 285 * dis;  //7 to 10 box
  }
  else if (dis > 10 && dis <= 13)
  {
    dTotalTicks = 285 * dis;  //10 to 13 box
  }

  while (mLTicks < dTotalTicks)
  {

    output = pidControl(mLTicks, mRTicks);
    md.setSpeeds(pwm1 - output * 5, pwm2 - output * 5);

    //    count++;
    //    total += abs(mLTicks-mRTicks);
    //    Serial.println(mLTicks-mRTicks);
    //    Serial.print(mLTicks);
    //    Serial.print("/");
    //    Serial.println(mRTicks);
  }
  //  avg =total/count;
  //  Serial.print("Avg:");
  //  Serial.println(avg);

  brake();

}

void moveAdjustF()
{
  mLTicks = 0;
  mRTicks = 0;
  double dTotalTicks = 0;
  double output;
  //  int pwm1 = 333, pwm2 = 353;
  int pwm1 = 375, pwm2 = 354;
  dTotalTicks = 1;

  while (mLTicks < dTotalTicks)
  {
    //    output = pidControl(mLTicks, mRTicks);+
    //    md.setSpeeds(pwm1 - output * 5, pwm2 + output * 5);
    md.setSpeeds(pwm1 , pwm2 );
  }
  brakeabit();
}

void moveAdjustB()
{
  mLTicks = 0;
  mRTicks = 0;
  double dTotalTicks = 0;
  double output;
  int pwm1 = -375, pwm2 = -354;
  //  int pwm1 = -100, pwm2 = -80;

  dTotalTicks = 1;

  while (mLTicks < dTotalTicks)
  {
    //    output = pidControl(mLTicks, mRTicks);
    md.setSpeeds(pwm1 , pwm2 );

  }
  brakeabit();
}

//read long range sensor data
double readLongRange(uint8_t sensor, double offset)
{
  double data;
  int distance;
  //Reading analog voltage of sensor
  data = analogRead(sensor);
  if (data > 520 )
  {
    distance = 10;
  }
  else if (data > 400 && data <= 520)
  {
    distance = 20;
  }
  else if (data > 310 && data <= 400)
  {
    distance = 30;
  }
  else if (data > 242 && data <= 310)
  {
    distance = 40;
  }
  else if (data > 177 && data <= 242)
  {
    distance = 50;
  }
  else if (data <= 177)
  {
    distance = 60;
  }
  else
  {
    distance = 60;
  }
  //  distance = (-0.00000082448212984193 * pow(data, 3) + 0.001131576262231 * pow(data, 2) - 0.601043546879649 * data + 139.718362171335) - offset;

  //  if (distance < 18)
  //    distance = 10;
  //  else if (distance < 25)
  //    distance = 20;
  //  else if (distance >= 25 &&  distance < 35)
  //    distance = 30;
  //  else if (distance >= 35 && distance < 45)
  //    distance = 40;
  //  else if (distance >= 45 && distance < 55)
  //    distance = 50;
  //  else if (distance >= 55 && distance < 65)
  //    distance = 60;
  return distance;
}

//read short range sensors data
double readSensor(SharpIR sensor, double offset)
{
  double dis;

  dis = sensor.getDistance() + offset;

  return dis;
}

//*grm
//get all sensor data
void getRMedian()
{
  //sample 50 times for short range sensors
  for (int sCount = 0; sCount < 50 ; sCount++)
  {
    //Calculate the distance in centimeters and store the value in a variable
    disFL = readSensor(sensorFL, -5);
    disFC = readSensor(sensorFC, -4);
    disFR = readSensor(sensorFR, -5);

    disLF = readSensor(sensorLF, -7);
    disLB = readSensor(sensorLB, -7);

    //add the variables into arrays as samples
    FrontR.add(disFR);
    FrontL.add(disFL);
    FrontC.add(disFC);
    LeftF.add(disLF);
    LeftB.add(disLB);
  }
  //sample 100 times for short range sensors
  for (int sCount = 0; sCount < 100 ; sCount++)
  {
    disR = readLongRange(sensorR, 0);

    //add the variables into arrays as samples
    Right.add(disR);
  }

}

void getRMedianAutoCalibrate()
{
  for (int sCount = 0; sCount < 20 ; sCount++)
  {
    disFL = readSensor(sensorFL, 0);
    disFC = readSensor(sensorFC, 1);
    disFR = readSensor(sensorFR, 0);
    disLF = readSensor(sensorLF, -1);
    disLB = readSensor(sensorLB, -1);

    //add the variables into arrays as samples
    FrontR.add(disFR);
    FrontL.add(disFL);
    FrontC.add(disFC);
    LeftF.add(disLF);
    LeftB.add(disLB);
  }
}

//seperate from the main function to save time
//get front sensor data only
//offset to increase the acurracy for short distance
void getRMedianFront()
{
  for (int sCount = 0; sCount < 20 ; sCount++)
  {
    disFL = readSensor(sensorFL, 0);
    disFC = readSensor(sensorFC, 1);
    disFR = readSensor(sensorFR, 0);

    //add the variables into arrays as samples
    FrontR.add(disFR);
    FrontL.add(disFL);
    FrontC.add(disFC);
  }
}

//seperate from the main function to save time
//get left sensor data only
//offset to increase the acurracy for short distance
void getRMedianLeft()
{
  for (int sCount = 0; sCount < 20 ; sCount++)
  {

    disLF = readSensor(sensorLF, 0);
    disLB = readSensor(sensorLB, 0);

    //add the variables into arrays as samples
    LeftF.add(disLF);
    LeftB.add(disLB);
  }
}

//For distance adjustment
void getRMedianDistanceAdjust()
{
  for (int sCount = 0; sCount < 20 ; sCount++)
  {
    disFL = readSensor(sensorFL, 0);
    //add the variables into arrays as samples
    FrontL.add(disFL);

  }
}
void getRMedianDistanceAdjustFR()
{
  for (int sCount = 0; sCount < 20 ; sCount++)
  {
    disFR = readSensor(sensorFR, 0);
    //add the variables into arrays as samples
    FrontR.add(disFR);
  }
}

void getRMedianStairs()
{
  for (int sCount = 0; sCount < 20 ; sCount++)
  {
    disFL = readSensor(sensorFL, 0);
    disFC = readSensor(sensorFC, 0);
    disLF = readSensor(sensorLF, 0);

    //add the variables into arrays as samples
    FrontL.add(disFL);
    FrontC.add(disFC);
    LeftF.add(disLF);
  }
}

//*crm
void clearRMedian() {
  FrontR.clear();
  FrontL.clear();
  FrontC.clear();
  Right.clear();
  LeftF.clear();
  LeftB.clear();
}

void getSensorsDataActual() {

  getRMedian();
  disFL = FrontL.getMedian() ;
  disFC = FrontC.getMedian() ;
  disFR = FrontR.getMedian() ;
  disLF = LeftF.getMedian() ;
  disLB = LeftB.getMedian() ;
  disR = Right.getMedian();
  clearRMedian();

  // Message to PC
  Serial.print('P');
  Serial.print(disFL);
  Serial.print(",");
  Serial.print(disFC);
  Serial.print(",");
  Serial.print(disFR);
  Serial.print(",");
  Serial.print(disLF);
  Serial.print(",");
  Serial.print(disLB);
  Serial.print(",");
  Serial.println(disR);
}

//*gsd
void getSensorsData(char cmd) {

  checkForCalibration(cmd);
  //  delay(100);
  // Message to PC

  getRMedian();
  FL = FrontL.getMedian() / 10 + 1;
  FC = FrontC.getMedian() / 10 + 1;
  FR = FrontR.getMedian() / 10 + 1;
  R = Right.getMedian() / 10 + 1;

  LF = LeftF.getMedian() / 10 + 1;;
  LB = LeftB.getMedian() / 10 + 1;;
  clearRMedian();
  Serial.print('P');
  Serial.print(FL);
  Serial.print(",");
  Serial.print(FC);
  Serial.print(",");
  Serial.print(FR);
  Serial.print(",");
  Serial.print(LF);
  Serial.print(",");
  Serial.print(LB);
  Serial.print(",");
  Serial.println(R);
}

//Debug function for calibration
void getSensorsDataFront() {

  getRMedianFront();
  Serial.print('P');
  Serial.print(FrontL.getAverage());
  Serial.print(",");
  Serial.print(FrontC.getAverage());
  Serial.print(",");
  Serial.println(FrontR.getAverage());
  clearRMedian();
}


//Debug function for calibration
void getSensorsDataLeft() {

  getRMedianLeft();
  Serial.print('P');
  Serial.print(LeftF.getAverage());
  Serial.print(",");
  Serial.println(LeftB.getAverage());
  clearRMedian();
}


//Debug function for calibration
void getSensorsDataDistanceAdjust() {

  getRMedianDistanceAdjust();

  Serial.print('P');
  Serial.println(FrontL.getAverage());
  getRMedianDistanceAdjustFR();
  Serial.println(FrontR.getAverage());
  clearRMedian();
}

//Debug function for calibration
void getSensorsDataStairs() {

  getRMedianStairs();
  Serial.print('P');
  Serial.print(FrontL.getAverage());
  Serial.print(",");
  Serial.print(FrontC.getAverage());
  Serial.print(",");
  Serial.println(LeftF.getAverage());
  clearRMedian();
}


int countFAndLWall = 0;
void checkForCalibration(char cmd)
{
  getRMedianAutoCalibrate();
  disFL = FrontL.getAverage();
  disFC = FrontC.getAverage();
  disFR = FrontR.getAverage();
  disLF = LeftF.getAverage();
  disLB = LeftB.getAverage();
  clearRMedian();

  if ((disFL  <= 13 && disFC  <= 13) || (disFL  <= 13 && disFR  <= 13) || (disFC  <= 13 && disFR  <= 13) )
  {
    if (disLF  <= 15.5 && disLB  <= 15.5 && countFAndLWall >= 15 )
    {
      FrontAndLeftWall();
      adjustFailCount = 0;
      adjustFrontFailCount = 0;
      countFAndLWall = 0;
      //      Serial.println("Left and Front Wall");
    }
    else
    {
      FrontWall();
      //      adjustFrontFailCount = 0;
      adjustFailCount = 0;
      countFAndLWall++;
      //      Serial.println("Front Wall");
    }
  }
  else if (((disLF  <= 15.5 && disLB  <= 15.5)) && adjustFailCount >= 0 )
  {
    LeftWall();
    //    adjustFrontFailCount++;
    countFAndLWall++;
    //    Serial.println("Left Wall");
  }
  else
  {
    //    adjustDistance();
    //    adjustFrontFailCount++;
    adjustFailCount++;
    countFAndLWall++;
  }



  if ((disLF >= 13.5 && disLF <= 15) && (previousLF >= 8 && previousLF <= 15))
  {
    leftFrontLimit();
  }
  else if ((disLB >= 13.5 && disLB <= 15) && (previousLF >= 8 && previousLF <= 15))
  {
    leftBackLimit();
  }
  else if (disLF <= 9 && (previousLF >= 8 && previousLF <= 15))
  {
    leftFrontLimit();
  }
  else if (disLB <= 9 && (previousLF >= 8 && previousLF <= 15))
  {
    leftBackLimit();
  }
  else if ((disLF >= 13.5 && disLF <= 15) && (disLB >= 13.5 && disLB <= 15))
  {
    leftBackLimit();
  }
  else if (disLF <= 9 && disLB <= 9)
  {
    leftBackLimit();
  }

  //  if ((adjustFrontFailCount >= 12 || adjustFailCount >= 10) && LF  <= 1 && LB <= 1 )
  //  {
  //    turnLeft(90);
  //    adjustDistance();
  //    adjustAngleFront();
  //    //    delay(100);
  //    turnRight(90);
  //    //    delay(100);
  //    adjustFailCount = 0;
  //    adjustFrontFailCount = 0;
  //    countFAndLWall++;
  //    //    Serial.println("Turn Left Calibrate");
  //  }
  //  else if ((adjustFailCount >= 10 || adjustFrontFailCount >= 12) )
  //  {
  //    if (previousR == 2 && R == 2)
  //    {
  //      turnRight(90);
  //      adjustDistance();
  //      adjustAngleFront();
  //      //      delay(100);
  //      turnLeft(90);
  //      //      delay(100);
  //      adjustFailCount = 0;
  //      countFAndLWall++;
  //      adjustFrontFailCount = 0;
  //      //      Serial.println("Turn Right Calibrate");
  //    }
  //    staircase angle calibration
  //    else if ((FL == 1 && FC == 2 && LF == 1) && adjustFailCount >= 15)
  //    {
  //        adjustAngleStaircase();
  //        countFAndLWall++;
  //    }
  //  }

  previousR = R;
  previousLF = disLF;

}

void leftFrontLimit()
{
  //  Serial.println("LF limit");
  turnLeftFast(90);
  delay(50);
  getRMedianFront();
  disFL = FrontL.getAverage();
  disFC = FrontC.getAverage();
  disFR = FrontR.getAverage();
  clearRMedian();
  if ((disFC > 3 && disFC < 13) || (disFL > 3 && disFL < 13) || (disFR > 3 && disFR < 13))
  {
    FrontWallFR();
  }
  else
    adjustDistanceFR();
  delay(20);
  turnRightFast(90);
}

void leftBackLimit()
{
  //  Serial.println("LB limit");
  turnLeftFast(90);
  delay(50);
  getRMedianFront();
  disFL = FrontL.getAverage();
  disFC = FrontC.getAverage();
  disFR = FrontR.getAverage();
  clearRMedian();
  if ((disFC > 3 && disFC < 13) || (disFL > 3 && disFL < 13) || (disFR > 3 && disFR < 13))
  {
    FrontWall();
  }
  else
    adjustDistance();
  delay(20);
  turnRightFast(90);
}

void FrontWall()
{

  adjustDistance();
  adjustAngleFront();

}

void FrontWallFR()
{
  adjustDistanceFR();
  adjustAngleFront();

}

void LeftWall()
{
  adjustAngleLeft();
}

void FrontAndLeftWall()
{
  turnLeftFast(90);
  autoCalibrate();
  turnRightFast(90);
  autoCalibrate();
}

//auto calibration
void autoCalibrate()
{
  for (int i = 0; i < 10; i++)
  {
    adjustDistance();
    //    adjustAngleLeft();
    adjustAngleFront();
  }
}

//*aa
// Front One Grid Angle Alignment
void adjustAngleFront()
{
  //error variables
  double calibrateFront = 9;
  double calibrateFrontLeft = 9;
  double calibrateFrontRight = 9;

  double dErrorFront = 0;
  double dErrorFrontLeft = 0;
  double dErrorFrontRight = 0;

  double dErrorDiff_1 = 0, dErrorDiff_2 = 0, dErrorDiff_3 = 0;

  //timer for break the loop in case stuck in the loop
  unsigned long preTime = millis();
  unsigned long curTime = 0;

  while (1)
  {
    curTime = millis();

    if (curTime - preTime > 3000)
    {
      break;
    }

    getRMedianFront();
    disFL = FrontL.getAverage();
    disFC = FrontC.getAverage();
    disFR = FrontR.getAverage();
    clearRMedian();

    dErrorFront =  disFC - calibrateFront;
    dErrorFrontLeft =  disFL - calibrateFrontLeft;
    dErrorFrontRight =  disFR - calibrateFrontRight;

    dErrorDiff_1 = dErrorFrontLeft - dErrorFrontRight;
    dErrorDiff_2 = dErrorFront - dErrorFrontLeft;
    dErrorDiff_3 = dErrorFront - dErrorFrontRight;

    //    if ((disFC > 3 && disFC < 13) || (disFL > 3 && disFL < 13) || (disFR > 3 && disFR < 13))
    //    {

    if ((disFC > 3 && disFC < 13) && (disFL > 3 && disFL < 13))
    {
      if (abs(dErrorDiff_2) < 0.3)
      {
        break;
      }
      if (dErrorFrontLeft > dErrorFront)
      {
        turnRightabit(1);
        //          delay(50);
      }
      else
      {
        turnLeftabit(1);
        //          delay(50);
      }
    }
    else if ((disFC > 3 && disFC < 13) && (disFR > 3 && disFR < 13))
    {
      if (abs(dErrorDiff_3) < 0.3)
      {
        break;
      }
      if (dErrorFrontRight > dErrorFront)
      {
        turnLeftabit(1);
        //          delay(50);
      }
      else
      {
        turnRightabit(1);
        //          delay(50);
      }
    }
    else if ((disFL > 3 && disFL < 13) && (disFR > 3 && disFR < 13))
    {
      if (abs(dErrorDiff_1) < 0.3)
      {
        break;
      }
      if (dErrorFrontLeft < dErrorFrontRight)
      {
        turnLeftabit(1);
        //          delay(50);
      }
      else
      {
        turnRightabit(1);
        //          delay(50);
      }
    }
    //    }
    //break the while loop instead of wait for 5s
    else
    {
      break;
    }
  }
  //May need a delay here

}

void adjustAngleLeft()
{
  //error variables
  double calibrateLeftFront = 9;
  double calibrateLeftBack = 9;

  double dErrorLeftFront = 0;
  double dErrorLeftBack = 0;

  double dErrorDiff = 0;

  //timer for break the loop in case stuck in the loop
  unsigned long preTime = millis();
  unsigned long curTime = 0;

  while (1)
  {
    curTime = millis();

    if (curTime - preTime > 3000)
    {
      break;
    }
    getRMedianLeft();
    disLF = LeftF.getAverage();
    disLB = LeftB.getAverage();
    clearRMedian();

    dErrorLeftFront =  disLF - calibrateLeftFront;
    dErrorLeftBack =  disLB - calibrateLeftBack;
    dErrorDiff = dErrorLeftFront - dErrorLeftBack;

    if ((disLF > 1 && disLF < 15.5) && (disLB > 1 && disLB < 15.5))
    {

      if (abs(dErrorDiff) < 0.3)
      {
        break;
      }
      if (dErrorLeftFront < dErrorLeftBack)
      {
        turnRightabit(1);
        //        delay(50);
      }
      else if (dErrorLeftFront > dErrorLeftBack)
      {
        turnLeftabit(1);
        //        delay(50);
      }
    }
    //break the while loop instead of wait for 5s
    else
    {
      break;
    }
  }
  //May need a delay here

}

//*ad
void adjustDistance()
{
  unsigned long preTime = millis();
  unsigned long curTime = 0;

  while (1)
  {
    curTime = millis();
    if (curTime - preTime > 3000)
    {
      break;
    }
    getRMedianDistanceAdjust();
    disFL = FrontL.getAverage();
    clearRMedian();

    //Use Front Left Sensor adjust distance
    if (disFL >= 7.2 && disFL < 9.5)
    {
      moveAdjustB();
      //      delay(50);      //original is delay(100) reduce to speedup
    }
    else if (disFL > 10.5 && disFL <= 15)
    {
      moveAdjustF();
      //      delay(50);
    }
    else
    {
      break;
    }
  }
}

void adjustDistanceFR()
{
  unsigned long preTime = millis();
  unsigned long curTime = 0;

  while (1)
  {
    curTime = millis();
    if (curTime - preTime > 3000)
    {
      break;
    }
    getRMedianDistanceAdjustFR();
    disFR = FrontR.getAverage();
    clearRMedian();

    //Use Front Left Sensor adjust distance
    if (disFR >= 7.2 && disFR < 9.5)
    {
      moveAdjustB();
      //      delay(50);      //original is delay(100) reduce to speedup
    }
    else if (disFR > 10.5 && disFR <= 16)
    {
      moveAdjustF();
      //      delay(50);
    }
    else
    {
      break;
    }
  }
}

void adjustAngleStaircase()
{
  //error variables
  double calibrateFL = 11;
  double calibrateFC = 19;
  double calibrateLF = 10;

  double dErrorFL = 0;
  double dErrorFC = 0;
  double dErrorLF = 0;

  double dErrorDiffFLFC = 0, dErrorDiffFLLF = 0;

  //timer for break the loop in case stuck in the loop
  unsigned long preTime = millis();
  unsigned long curTime = 0;
  bool disFlag = true;
  while (1)
  {
    curTime = millis();

    if (curTime - preTime > 3000)
    {
      break;
    }

    getRMedianStairs();
    disFL = FrontL.getAverage();
    disFC = FrontC.getAverage();
    disLF = LeftF.getAverage();
    clearRMedian();

    dErrorFL =  disFL - calibrateFL;
    dErrorFC =  disFC - calibrateFC;
    dErrorLF =  disLF - calibrateLF;

    dErrorDiffFLFC = dErrorFL - dErrorFC;
    dErrorDiffFLLF = dErrorFL - dErrorLF;

    if ((disFL > 3 && disFL < 15) && (disFC > 3 && disFC < 25) && (disLF > 3 && disLF < 15) )
    {
      if (disFlag)
      {
        adjustDistance();
        turnLeftFast(90);
        adjustDistanceFR();
        turnRightFast(90);
        disFlag = false;
      }
      if (abs(dErrorDiffFLFC) < 0.3 && abs(dErrorDiffFLLF) < 0.3)
      {
        break;
      }
      if ((dErrorFL < dErrorFC)  && (dErrorFL < dErrorLF))
      {
        turnLeftabit(1);
        //        delay(50);
      }
      else
      {
        turnRightabit(1);
        //        delay(50);
      }
    }
    //    else if ((disFL > 3 && disFL < 15) && (disLF > 3 && disLF < 15))
    //    {
    //      if (abs(dErrorDiffFLLF) < 0.3)
    //      {
    //        break;
    //      }
    //      if (dErrorFL < dErrorLF)
    //      {
    //        turnLeftabit(1);
    //        delay(50);
    //      }
    //      else
    //      {
    //        turnRightabit(1);
    //        delay(50);
    //      }
    //    }
    //break the while loop instead of wait for 5s
    else
    {
      break;
    }
  }
  //May need a delay here
}


void stairCaseTest()
{
  adjustAngleStaircase();
}


void compute_mL_ticks()
{
  mLTicks++;
}

void compute_mR_ticks()
{
  mRTicks++;
}


//*debug
//void debug(String str)
//{
//  //Sample string        QC10L355R317T295
//  String outputString;
//  for (int i = 1; i < str.length(); i++)
//  {
//    outputString += str[i];
//  }
//  //  Serial.print("outputString:");
//  //  Serial.println(outputString);
//  int coefficient, leftpwm, righpwm, totaltick;
//
//  String value = "";
//
//  for (int i = 1; i < 3; i++)
//  {
//    value += outputString.charAt(i);
//  }
//  coefficient = value.toInt();
//  //  Serial.print("coefficient:");
//  //  Serial.println(coefficient);
//
//
//  value = "";
//  for (int i = 4; i < 7; i++)
//  {
//    value += outputString.charAt(i);
//  }
//  leftpwm = value.toInt();
//  //  Serial.print("leftpwm:");
//  //  Serial.println(leftpwm);
//
//
//  value = "";
//  for (int i = 8; i < 11; i++)
//  {
//    value += outputString.charAt(i);
//  }
//  righpwm = value.toInt();
//  //  Serial.print("righpwm:");
//  //  Serial.println(righpwm);
//
//
//  value = "";
//  int index = outputString.indexOf('T');
//  for (int i = index + 1; i < outputString.length(); i++)
//  {
//    value += outputString.charAt(i);
//  }
//  totaltick = value.toInt();
//  //  Serial.print("totaltick:");
//  //  Serial.println(totaltick);
//
//  double output;
//  double dTotalTicks = totaltick;
//
//  int pwm1 = -leftpwm, pwm2 = righpwm;
//
//  while (mLTicks < dTotalTicks)
//  {
//
//    output = pidControl(mLTicks, mRTicks);
//    md.setSpeeds(pwm1 - output * coefficient, pwm2 + output * coefficient);     //check coiffient for debug
//
//  }
//
//  brake();
//
//}


