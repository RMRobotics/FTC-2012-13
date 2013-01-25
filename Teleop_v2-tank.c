#pragma config(Hubs,  S1, HTMotor,  HTMotor,  HTMotor,  HTServo)
#pragma config(Sensor, S1,     ,               sensorI2CMuxController)
#pragma config(Motor,  mtr_S1_C1_1,     leftTread,     tmotorTetrix, openLoop, reversed)
#pragma config(Motor,  mtr_S1_C1_2,     rightTread,    tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C2_1,     lift,          tmotorTetrix, openLoop, reversed)
#pragma config(Motor,  mtr_S1_C2_2,     lift2,         tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C3_1,     lift3,         tmotorTetrix, openLoop, reversed)
#pragma config(Motor,  mtr_S1_C3_2,     motorI,        tmotorTetrix, openLoop)
#pragma config(Servo,  srvo_S1_C4_1,    wristHoriz,           tServoStandard)
#pragma config(Servo,  srvo_S1_C4_2,    wristVert1,           tServoStandard)
#pragma config(Servo,  srvo_S1_C4_3,    wristVert2,           tServoStandard)
#pragma config(Servo,  srvo_S1_C4_4,    servo4,               tServoNone)
#pragma config(Servo,  srvo_S1_C4_5,    servo5,               tServoNone)
#pragma config(Servo,  srvo_S1_C4_6,    servo6,               tServoNone)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                           RMRobotics Tele-Operation Mode Code
//
// Program for teleop (remote control) portion of 2012-2013 FTC game: Ring It Up
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "JoystickDriver.c"  //Include file to "handle" the Bluetooth messages.

#define WRISTSPEED .3

typedef struct {
	TJoystick joy;

	short joy1_Buttons_Changed;  // Bit map for the 12 buttons. 1 means that the button changed state
	short joy2_Buttons_Changed;
} UserInput;

typedef struct {
	// Keep track what buttons were previously pressed so that
	// we can figure out whether their state changed.
	short old_joy1_Buttons;
	short old_joy2_Buttons;

	// Keep track of left and right tread speeds
	int leftTreadSpeed;
	int rightTreadSpeed;

	// Keep track of wrist servos' positions
	float wristHorizPos; // (0 - 243)
	float wristVert1Pos; // (0 - 247)
	float wristVert2Pos; // (0 - 227)

	// Keep track of lift speed
	int liftSpeed;
	int returnSpeed;

} State;

void getLatestInput(State *state, UserInput *input);
void handleDriveInputs(State *state, UserInput *input);
void handleLiftInputs(State *state, UserInput *input);
void handleWristInputs(State *state, UserInput *input);
void verifyCommands(State *state);
void updateAllMotors(State *state);
void showDiagnostics(State *state);

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                    initializeRobot
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

void initializeRobot()
{
  servo[wristHoriz] = 0;
  servo[wristVert1] = 0;
  servo[wristVert2] = 227;

  // Disable joystick driver's diagnostics display to free up nxt screen for our own diagnostics diplay
	disableDiagnosticsDisplay();

  return;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                         Main Task
//
// This is the main loop for the Tele-op program.
//
// The current controls to the robot are the following:
//
//               _==7==_                               _==8==_
//              / __5__ \                             / __6__ \
//            +.-'_____'-.---------------------------.-'_____'-.+
//           /   |     |  '.                       .'  |     |   \
//          / ___| /|\ |___ \                     / ___|(Y/4)|___ \
//         / |      |      | ;   __        __    ; |             | ;
//         | | <---   ---> | |  |__|      |__|   | |(X/1)   (B/3)| |
//         | |___   |   ___| ; BACK/9   START/10 ; |___       ___| ;
//         |\    | \|/ |    /  _     ___      _   \    |(A/2)|    /|
//         | \   |_____|  .','" "', |___|  ,'" "', '.  |_____|  .' |
//         |  '-.______.-' /       \ MODE /       \  '-._____.-'   |
//         |               |       |------|       |                |
//         |              /\       /      \       /\               |
//         |             /  '.___.'        '.___.'  \              |
//         |            /                            \             |
//          \          /                              \           /
//           \________/                                \_________/
//
// Controls
//   Controller 1
//     Left Joystick:................Left tread speed (analog control)
//     Right Joystick:...............Right tread speed (analog control)
//   Controller 2
//     A/2:........................Hand left
//     B/3:........................Hand right
//     X/1:........................Hand down
//     Y/4:........................Hand up
//     D-pad (Top hat)
//       Up:.........................Increase hand angle relative to the ground
//       Down:.......................Decrease hand angle relative to the ground
//
// NOTE: ASCII Art adapted from: http://chris.com/ascii/index.php?art=video%20games/other
/////////////////////////////////////////////////////////////////////////////////////////////////////

task main()
{
	initializeRobot();

	// Initialize state values
  State currentState;
  memset(&currentState, 0, sizeof(currentState));
  currentState.wristVert2Pos = 227;

  //waitForStart();   // wait for start of tele-op phase

  while (true)
  {
  	// Get latest user input
  	UserInput input;
  	getLatestInput(&currentState, &input);

  	// Process user inputs
  	handleDriveInputs(&currentState, &input);
  	handleLiftInputs(&currentState, &input);
  	handleWristInputs(&currentState, &input);

  	// Verify validity/possibility of commands
  	verifyCommands(&currentState);

  	// Execute user inputs
  	updateAllMotors(&currentState);
  	showDiagnostics(&currentState);
  }
}

void getLatestInput(State *state, UserInput *input)
{
	// Get the current joystick position
	getJoystickSettings(joystick);

	// Fill out our input structure
	memcpy(input->joy, joystick, sizeof(TJoystick));

	// Calculate which buttons changed.
	input->joy1_Buttons_Changed = input->joy.joy1_Buttons ^ state->old_joy1_Buttons;
	input->joy2_Buttons_Changed = input->joy.joy2_Buttons ^ state->old_joy2_Buttons;

	// Update state for next time.
	state->old_joy1_Buttons = input->joy.joy1_Buttons;
	state->old_joy2_Buttons = input->joy.joy2_Buttons;
}

int joyButton(short bitmask, int button)
{
	//return 1 or 0 based on whether button in bitmask is pressed or not
	return bitmask & (1 << (button - 1));
}

void handleDriveInputs(State *state, UserInput *input)
{
	// If left joystick is outside dead zone, move left tread, otherwise stop.
	if (abs(input->joy.joy1_y1) > 20) {
		state->leftTreadSpeed = input->joy.joy1_y1 * (100.0 / 128.0) + 0.5;
	} else {
		state->leftTreadSpeed = 0;
	}

	// If right joystick is outside dead zone, move right tread, otherwise stop.
	if (abs(input->joy.joy1_y2) > 20) {
		state->rightTreadSpeed = input->joy.joy1_y2 * (100.0 / 128.0) + 0.5;
	} else {
		state->rightTreadSpeed = 0;
	}
}

void handleLiftInputs(State *state, UserInput *input)
{
	if (joyButton(input->joy.joy2_Buttons, 6)) {
		state->liftSpeed = 100;
		state->returnSpeed = 10;
	} else if (joyButton(input->joy.joy2_Buttons, 8)) {
		state->liftSpeed = -10;
		state->returnSpeed = -100;
	} else {
		state->liftSpeed = 0;
		state->returnSpeed = 0;
	}
}

void handleWristInputs(State *state, UserInput *input)
{
	// Controls for horizontal servo
	if (joyButton(input->joy.joy2_Buttons, 3)) {
		state->wristHorizPos += WRISTSPEED;
	} else if (joyButton(input->joy.joy2_Buttons, 2)) {
		state->wristHorizPos -= WRISTSPEED;
	}

	// Controls for synchronized movement of vertical servos
	//   (keeps the angle of the hand relative to the ground constant)
	if (joyButton(input->joy.joy2_Buttons, 4)) {
		state->wristVert1Pos += WRISTSPEED;
		state->wristVert2Pos -= WRISTSPEED;
	} else if (joyButton(input->joy.joy2_Buttons, 1)) {
		state->wristVert1Pos -= WRISTSPEED;
		state->wristVert2Pos += WRISTSPEED;
	}

	// Controls for individual movement of 2nd vertical servo
	//   (allows the angle of the hand relative to the ground to be changed)
	if (input->joy.joy2_TopHat == 0) {
		state->wristVert2Pos += WRISTSPEED;
	} else if (input ->joy.joy2_TopHat == 4) {
		state->wristVert2Pos -= WRISTSPEED;
	}
}

void verifyCommands(State *state)
{
	// If the projected servo values aren't within the servos' ranges,
	// stay at the servos' current positions.

	// Servo Ranges
	// Horiz Servo: 0 - 243
	// 1st Vert Servo: 0 - 247
	// 2nd Vert Servo: 0 - 227

	if (state->wristHorizPos > 243 || state->wristHorizPos < 0) {
		state->wristHorizPos = ServoValue[wristHoriz];
	}

	if (state->wristVert1Pos > 247 || state->wristVert2Pos < 0
	 || state->wristVert2Pos > 227 || state->wristVert2Pos < 0) {
		state->wristVert1Pos = ServoValue[wristVert1];
		state->wristVert2Pos = ServoValue[wristVert2];
	}
}

void updateAllMotors(State *state)
{
	motor[leftTread] = state->leftTreadSpeed;
	motor[rightTread] = state->rightTreadSpeed;
	motor[lift] = state->liftSpeed;
	motor[lift2] = state->liftSpeed;
	motor[lift3] = state->returnSpeed;
	servo[wristHoriz] = state->wristHorizPos;
	servo[wristVert1] = state->wristVert1Pos;
	servo[wristVert2] = state->wristVert2Pos;
}

void showDiagnostics(State *state)
{
	//create label
	string sWristHorizPos = "wristHoriz = ";
	string sWristVert1Pos = "wristVert1 = ";
	string sWristVert2Pos = "wristVert2 = ";

	//store variable in a string
	string string1 = state->wristHorizPos;
	string string2 = state->wristVert1Pos;
	string string3 = state->wristVert2Pos;

	//concat variable with label
	strcat(sWristHorizPos, string1);
	strcat(sWristVert1Pos, string2);
	strcat(sWristVert2Pos, string3);

	eraseDisplay();

	//display label and value
	nxtDisplayTextLine(1, sWristHorizPos);
	nxtDisplayTextLine(3, sWristVert1Pos);
	nxtDisplayTextLine(5, sWristVert2Pos);
}
