#pragma config(Hubs,  S1, HTMotor,  HTMotor,  HTMotor,  HTServo)
#pragma config(Sensor, S1,     ,               sensorI2CMuxController)
#pragma config(Motor,  motorA,          outrigger,     tmotorNXT, PIDControl, encoder)
#pragma config(Motor,  motorB,          outrigger,     tmotorNXT, PIDControl, encoder)
#pragma config(Motor,  mtr_S1_C1_1,     right,         tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C1_2,     left,          tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C2_1,     back,          tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C2_2,     front,         tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C3_1,     lift,          tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C3_2,     arm,           tmotorTetrix, PIDControl, encoder)
#pragma config(Servo,  srvo_S1_C4_1,    wrist,                tServoStandard)
#pragma config(Servo,  srvo_S1_C4_2,    tineHook1,            tServoStandard)
#pragma config(Servo,  srvo_S1_C4_3,    tineHook2,            tServoStandard)
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

#define ARMSPEED 50
#define DRIVESPEED 100
#define LIFTSPEED 100
#define FORWARD 0		                   //move left motor cw, right motor ccw
#define BACKWARD 4		                 //move left motor ccw, right motor cw
#define	LEFT 6 	                       //move front motor ccw, back motor cw
#define	RIGHT 2 	                     //move front motor cw, back motor ccw
#define	DIAGONALFOWARDLEFT 7           //move front motor ccw, back motor cw, left motor cw, right motor ccw
#define	DIAGONALFOWARDRIGHT 1	         //move front motor cw, back motor ccw, left motor cw, right motor ccw
#define	DIAGONALBACKWARDLEFT 5         //move front motor ccw, back motor cw, left motor ccw, right motor cw
#define	DIAGONALBACKWARDRIGHT 3        //move front motor cw, back motor ccw, left motor ccw, right motor cw
#define	SPINLEFT 8                     //spin left	move all motors ccw
#define	SPINRIGHT 9                    //spin right move all motors cw


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

	// The desired direction is the directory that the user intends the robot
	// to move based on joystick input.
	int desiredDriveDirection;

	// The desired drive speed
	int desiredDriveSpeed;

	// The indicator for whether or not the wrist should automatically move with the arm
	bool autoMode;

	// The following track the current motor speeds and positions
	// of arms.
	int motorLeftSpeed;
	int motorRightSpeed;
	int motorFrontSpeed;
	int motorBackSpeed;
	int armSpeed;
	int armPosition;
	int liftSpeed;
	float wristPosition;
	int tineLockPosition;
	int outriggerSpeed;
} State;

void getLatestInput(State *state, UserInput *input);
void handleDriveInputs(State *state, UserInput *input);
void handleArmInputs(State *state, UserInput *input);
void handleWristInputs(State *state, UserInput *input);
void handleLiftInputs(State *state, UserInput *input);
void handleTineInputs(State *state, UserInput *input);
void handleOutriggerInputs(State *state, UserInput *input);
void computeDriveMotorSpeeds(State *state);
void computeActualState(State *desiredState, State *actualState);
void updateAllMotors(State *state);
void showDiagnostics(State *desiredState, State *actualState, UserInput *input);

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                    initializeRobot
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
void initializeRobot()
{
	// Set motors to lock when unpowered
	bFloatDuringInactiveMotorPWM = false;

	// Initialize the arm to a known position
	nMotorEncoder[arm] = 0;

	// Set distance outriggers have to extend to in endgame
	nMotorEncoderTarget[outrigger] = 100;

	// Disable joystick driver's diagnostics display to free up nxt screen for our own diagnostics diplay
	disableDiagnosticsDisplay();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                         Main Task
//
// Controls:
//   Tophat (D-pad)
//     Left:.......................Spin left
//     Right:......................Spin right
//   Left Joystick:................Lift up/down
//   Right Joystick:...............Movement
//   Button 1:.....................Release tines
//   Button 2:.....................Deploy outrigger
//   Button 5 (Left Bumper):.......Arm up
//   Button 7 (Left Trigger):......Arm down
//   Button 6 (Right Bumper):......Wrist up
//   Button 8 (Right Trigger):.....Wrist down
//   Button 9:.....................Toggle movement speed (slow/fast)
//   Button 10:....................Toggle autoMode
//                                 ON: wrist automatically moves with arm
//                                 OFF: manual control of wrist
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

task main()
{
	initializeRobot();

	State actualState;
	State desiredState;

	// Initialize everything in the desired state to 0.
	memset(&desiredState, 0, sizeof(desiredState));
	desiredState.wristPosition = 100;
	desiredState.desiredDriveSpeed = DRIVESPEED;

	//waitForStart();   // wait for start of tele-op phase

	while (true)
	{
		// Wait for the next update from the joystick.
		UserInput input;
		getLatestInput(&desiredState, &input);

		// Process the joystick input
		handleDriveInputs(&desiredState, &input);
		handleArmInputs(&desiredState, &input);
		handleWristInputs(&desiredState, &input);
		handleLiftInputs(&desiredState, &input);
		handleTineInputs(&desiredState, &input);
		handleOutriggerInputs(&desiredState, &input);

		computeDriveMotorSpeeds(&desiredState);
		computeActualState(&desiredState, &actualState);

		updateAllMotors(&actualState);

		// Display variable values for debugging purposes
		showDiagnostics(&desiredState, &actualState, &input);
	}
}

void getLatestInput(State *state, UserInput *input)
{
	// Get the current joystick position
	getJoystickSettings(joystick);

	// Fill out our input structure
	//input->joy = joystick; // It doesn't work, doesn't copy the values in joystick to input->joy
		//for now, I'm using joystick to access the joystick controller inputs

	// Calculate which buttons changed.
	input->joy1_Buttons_Changed = joystick.joy1_Buttons ^ state->old_joy1_Buttons;
	input->joy2_Buttons_Changed = joystick.joy2_Buttons ^ state->old_joy2_Buttons;

	// Update state for next time.
	state->old_joy1_Buttons = joystick.joy1_Buttons;
	state->old_joy2_Buttons = joystick.joy2_Buttons;
}

int joyButton(short bitmask, int button)
{
	//return 1 or 0 based on whether button in bitmask is pressed or not
	return bitmask & (1 << (button - 1));
}

void handleDriveInputs(State *state, UserInput *input)
{
	// Default direction of null  = stop
	int dir = -1;

	// Toggle drive speed if button 9 was clicked (i.e. changed and button down)
	if(joyButton(input->joy1_Buttons_Changed, 9) && joyButton(joystick.joy1_Buttons, 9)) {
		state->desiredDriveSpeed = (DRIVESPEED*DRIVESPEED)/(state->desiredDriveSpeed*4);
	}

	// movement controlled by right joystick
	short x = joystick.joy1_x2;
	short y = joystick.joy1_y2;
	if (x == 0) x = 1; //to avoid division by 0
	float angle = atan2(y, x); //find angle using tangent

	if (abs(x) > 20 || abs(y) > 20) { //If robot isn't in deadzone...
		                                //Value of angle, in radians and degrees
	                                  //  Notice that degrees don't correspond to radians.
	                                  //  This is because atan2(Y,X) returns angles in radians from -PI to PI
	                                  //  while using radiansToDegrees(Radians) to convert atan2(Y,X) to degrees
	                                  //  returns values from 0-360.
		if (angle > 7*PI/8) {           //+2.75 rad, 157.5 deg
			dir = LEFT;
			} else if (angle > 5*PI/8) {  //+1.96 rad, 112.5 deg
			dir = DIAGONALFOWARDLEFT;
			} else if (angle > 3*PI/8) {  //+1.18 rad, 067.5 deg
			dir = FORWARD;
			} else if (angle > PI/8) {    //+0.39 rad, 022.5 deg
			dir = DIAGONALFOWARDRIGHT;
			} else if (angle > -PI/8) {   //-0.39 rad, 337.5 deg
			dir = RIGHT;
			} else if (angle > -3*PI/8) { //-1.18 rad, 292.5 deg
			dir = DIAGONALBACKWARDRIGHT;
			} else if (angle > -5*PI/8) { //-1.96 rad, 247.5 deg
			dir = BACKWARD;
			} else if (angle > -7*PI/8) { //-2.75 rad, 202.5 deg
			dir = DIAGONALBACKWARDLEFT;
			} else {
			dir = LEFT;
		}
	}

	// Spinning controlled by left and right on the tophat (d-pad)
	switch (joystick.joy1_TopHat) {
	case 2: dir = SPINRIGHT; break;
	case 6: dir = SPINLEFT;
	}

	state->desiredDriveDirection = dir;
}

void handleArmInputs(State *state, UserInput *input)
{
	// Button 5 to raise arm, button 7 to lower arm
	if (joyButton(joystick.joy1_Buttons, 5))	{
		state->armSpeed = ARMSPEED;
	}
	else if(joyButton(joystick.joy1_Buttons, 7)) {
		state->armSpeed = -ARMSPEED;
	}
	else {
		state->armSpeed = 0;
	}
}

void handleWristInputs(State *state, UserInput *input)
{
	// Toggle automode if button 10 was clicked (i.e. changed and button down)
	if (joyButton(input->joy1_Buttons_Changed, 10) && joyButton(joystick.joy1_Buttons, 10)) {
		state->autoMode = !state->autoMode;
		state->armPosition = nMotorEncoder[arm];
	}

	// If auto mode on, sync arm and wrist. Else, allow manual control of wrist position.
	//     *synced meaning 1 deg up on arm makes wrist go down 1deg so that wrist always
	//      meaintains the same orientation to the ground
	if (state->autoMode) {
		state->wristPosition -= (nMotorEncoder[arm]-state->armPosition)/4;
			// servo values correspond to degrees,
			// encoder values correspond to 1/4 degrees
		state->armPosition = nMotorEncoder[arm];
	}
	else {
		// Button 6 to raise wrist, button 8 to lower wrist (if wrist isn't being synced to arm)
		if (joyButton(joystick.joy1_Buttons, 6) && state->wristPosition < 220) {
			++state->wristPosition;
		}
		else if (joyButton(joystick.joy1_Buttons, 8) && state->wristPosition > 20) {
			--state->wristPosition;
		}
	}
}

void handleLiftInputs(State *state, UserInput *input)
{
	// left joystick forward to raise lift, backward to lower lift
	if(joystick.joy1_y1 >= 20) {
		state->liftSpeed = LIFTSPEED;
	}
	else if(joystick.joy1_y1 <= -20) {
		state->liftSpeed = -LIFTSPEED;
	}
	else {
		state->liftSpeed = 0;
	}
}

void handleTineInputs(State *state, UserInput *input) {
	// If button 1 is pressed, release tines;
	if (joyButton(joystick.joy1_Buttons, 1))
		state->tineLockPosition=100;
}

void handleOutriggerInputs(State *state, UserInput *input) {
	// If button 2 is clicked (ie. changed and pressed down), deploy outriggers
	if (joyButton(input->joy1_Buttons_Changed, 2) && joyButton(joystick.joy1_Buttons, 2))
		state->outriggerSpeed=100;

	// Turn off outrigger motors once they're fully deployed
	if (nMotorRunState[outrigger] == runStateIdle)
		state->outriggerSpeed = 0;

}

void computeDriveMotorSpeeds(State *state)
{
	/*
	DIRECTION PARAMETER: (cw/ccw are determined from perspective of robot's center)
	(cw is positive rotation)
	forward								move left motor cw, right motor ccw
	backward							move left motor ccw, right motor cw
	left									move front motor ccw, back motor cw
	right									move front motor cw, back motor ccw
	diagonal fwd left			move front motor ccw, back motor cw, left motor cw, right motor ccw
	diagonal fwd right		move front motor cw, back motor ccw, left motor cw, right motor ccw
	diagonal bwd left			move front motor ccw, back motor cw, left motor ccw, right motor cw
	diagonal bwd right		move front motor cw, back motor ccw, left motor ccw, right motor cw
	spin left							move all motors ccw
	spin right						move all motors cw
	*/

	switch(state->desiredDriveDirection) {
	case FORWARD: state->motorFrontSpeed = 0;
		state->motorBackSpeed = 0;
		state->motorLeftSpeed = state->desiredDriveSpeed;
		state->motorRightSpeed = -state->desiredDriveSpeed;
		break;
	case BACKWARD: state->motorFrontSpeed = 0;
		state->motorBackSpeed = 0;
		state->motorLeftSpeed = -state->desiredDriveSpeed;
		state->motorRightSpeed = state->desiredDriveSpeed;
		break;
	case LEFT: state->motorFrontSpeed = -state->desiredDriveSpeed;
		state->motorBackSpeed = state->desiredDriveSpeed;
		state->motorLeftSpeed = 0;
		state->motorRightSpeed = 0;
		break;
	case RIGHT: state->motorFrontSpeed = state->desiredDriveSpeed;
		state->motorBackSpeed = -state->desiredDriveSpeed;
		state->motorLeftSpeed = 0;
		state->motorRightSpeed = 0;
		break;
	case DIAGONALFOWARDLEFT: state->motorFrontSpeed = -state->desiredDriveSpeed;
		state->motorBackSpeed = state->desiredDriveSpeed;
		state->motorLeftSpeed = state->desiredDriveSpeed;
		state->motorRightSpeed = -state->desiredDriveSpeed;
		break;
	case DIAGONALFOWARDRIGHT: state->motorFrontSpeed = state->desiredDriveSpeed;
		state->motorBackSpeed = -state->desiredDriveSpeed;
		state->motorLeftSpeed = state->desiredDriveSpeed;
		state->motorRightSpeed = -state->desiredDriveSpeed;
		break;
	case DIAGONALBACKWARDLEFT: state->motorFrontSpeed = -state->desiredDriveSpeed;
		state->motorBackSpeed = state->desiredDriveSpeed;
		state->motorLeftSpeed = -state->desiredDriveSpeed;
		state->motorRightSpeed = state->desiredDriveSpeed;
		break;
	case DIAGONALBACKWARDRIGHT: state->motorFrontSpeed = state->desiredDriveSpeed;
		state->motorBackSpeed = -state->desiredDriveSpeed;
		state->motorLeftSpeed = -state->desiredDriveSpeed;
		state->motorRightSpeed = state->desiredDriveSpeed;
		break;
	case SPINLEFT: state->motorFrontSpeed = -state->desiredDriveSpeed;
		state->motorBackSpeed = -state->desiredDriveSpeed;
		state->motorLeftSpeed = -state->desiredDriveSpeed;
		state->motorRightSpeed = -state->desiredDriveSpeed;
		break;
	case SPINRIGHT:	state->motorFrontSpeed = state->desiredDriveSpeed;
		state->motorBackSpeed = state->desiredDriveSpeed;
		state->motorLeftSpeed = state->desiredDriveSpeed;
		state->motorRightSpeed = state->desiredDriveSpeed;
		break;
	default:	state->motorFrontSpeed = 0;
		state->motorBackSpeed = 0;
		state->motorLeftSpeed = 0;
		state->motorRightSpeed = 0;
	}
}

void computeActualState(State *desiredState, State *actualState)
{
	//actualState = desiredState; // It doesn't work. It doesn't copy the values in desiredState to actualState
		//for now, I'm individually copying the values needed for updateAllMotors to work.
	actualState->motorLeftSpeed = desiredState->motorLeftSpeed;
	actualState->motorRightSpeed = desiredState->motorRightSpeed;
	actualState->motorFrontSpeed = desiredState->motorFrontSpeed;
	actualState->motorBackSpeed = desiredState->motorBackSpeed;
	actualState->armSpeed = desiredState->armSpeed;
	actualState->liftSpeed = desiredState->liftSpeed;
	actualState->wristPosition = desiredState->wristPosition;
	actualState->tineLockPosition = desiredState->tineLockPosition;
	actualState->outriggerSpeed = desiredState->outriggerSpeed;

	// If theoretical wrist position is outside of the servo's range, change it to the
	// closest value within the servo's range. The theoretial value is kept though so
	// that if the wrist is synced with the arm, it'll return to being synced once
	// servo values are back in range.
	//     *synced meaning 1deg up on arm makes wrist go down 1deg so that wrist always
	//      maintains the same orientation to the ground
	if(desiredState->wristPosition < 20) {
		actualState->wristPosition = 21;
		}	else if(desiredState->wristPosition > 220) {
		actualState->wristPosition = 219;
	}
}

void updateAllMotors(State *state)
{
	motor[front] = state->motorFrontSpeed;
	motor[back] = state->motorBackSpeed;
	motor[left] = state->motorLeftSpeed;
	motor[right] = state->motorRightSpeed;
	motor[arm] = state->armSpeed;
	motor[lift] = state->liftSpeed;
	motor[outrigger] = state->outriggerSpeed;
	servo[wrist] = state->wristPosition;
	servo[tineHook1] = state->tineLockPosition;
	servo[tineHook2] = state->tineLockPosition;
}

void showDiagnostics(State *desiredState, State *actualState, UserInput *input)
{
	//create label
	string sDriveDirection = "dir = ";
	string sDriveSpeed = "driveSpeed = ";
	string sArmSpeed = "armSpeed = ";
	string sLiftSpeed = "liftSpeed = ";
	string sAutoMode = "autoMode = ";
	string sArmPosition = "armPos = ";
	string sWristPositionD = "dWristPos = ";
	string sWristPositionA = "aWristPos = ";

	//store variable in a string
	string string1 = desiredState->desiredDriveDirection;
	string string2 = desiredState->desiredDriveSpeed;
	string string3 = actualState->armSpeed;
	string string4 = actualState->liftSpeed;
	string string5 = (int)desiredState->autoMode;
	string string6 = desiredState->armPosition;
	string string7 = (int)desiredState->wristPosition;
	string string8 = (int)actualState->wristPosition;

	//concat variable with label
	strcat(sDriveDirection, string1);
	strcat(sDriveSpeed, string2);
	strcat(sArmSpeed, string3);
	strcat(sLiftSpeed, string4);
	strcat(sAutoMode, string5);
	strcat(sArmPosition, string6);
	strcat(sWristPositionD, string7);
	strcat(sWristPositionA, string8);

	eraseDisplay();

	//display label and value
	nxtDisplayTextLine(0, sDriveDirection);
	nxtDisplayTextLine(1, sDriveSpeed);
	nxtDisplayTextLine(2, sArmSpeed);
	nxtDisplayTextLine(3, sLiftSpeed);
	nxtDisplayTextLine(4, sAutoMode);
	nxtDisplayTextLine(5, sArmPosition);
	nxtDisplayTextLine(6, sWristPositionD);
	nxtDisplayTextLine(7, sWristPositionA);
}
