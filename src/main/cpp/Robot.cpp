// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.
#include "frc/XboxController.h"
#include <frc/TimedRobot.h>
#include <frc/drive/DifferentialDrive.h>
#include <frc/motorcontrol/PWMSparkMax.h>
#include <frc/Timer.h>
#include <frc/Notifier.h>
#include <frc/motorcontrol/MotorControllerGroup.h>
#include <frc/smartdashboard/SmartDashboard.h>
#include <rev/CANSparkMax.h>
//---
#include <frc/motorcontrol/Victor.h>
#include <frc/motorcontrol/PWMVictorSPX.h>
#include <frc/motorcontrol/PWMTalonSRX.h>
#include <frc/motorcontrol/Talon.h> //NEW MOTOR-CONTROLLER TYPES
//---
#include <frc/smartdashboard/SendableChooser.h>
#include <frc/RobotState.h>
#include <units/time.h>
#define WPILIB_NO_UNITS

  //auto options
   frc::SendableChooser<std::string> m_chooser;
   std::string kAutoDefault = "NONE";
   std::string kAutoFowrard1 = "Red_Mid";
   std::string kAutoFowrard2 = "Red_Right";
   std::string kAutoFowrard3 = "Red_Left";

/**
 * This is a demo program showing the use of the DifferentialDrive class.
 * Runs the motors with tank steering.
 */
class Robot : public frc::TimedRobot {

  //insersted from EVERYBOT EXAMPLE CODE-----------------------------
  frc::PWMTalonSRX driveLeftTalon{1};
  frc::PWMVictorSPX driveRightVictor{2}; //check which one exactly + IDs

  /**
   * How many amps the arm motor can use.
   */
  static const int ARM_CURRENT_LIMIT_A = 20;

  /**
   * Percent output to run the arm up/down at
   */
  static const double ARM_OUTPUT_POWER = 0.4;

  /**
   * How many amps the intake can use while picking up
   */
  static const int INTAKE_CURRENT_LIMIT_A = 25;

  /**
   * How many amps the intake can use while holding
   */
  static const int INTAKE_HOLD_CURRENT_LIMIT_A = 5;

  /**
   * Percent output for intaking
   */
  static const double INTAKE_OUTPUT_POWER = 1.0;

  /**
   * Percent output for holding
   */
  static const double INTAKE_HOLD_POWER = 0.07;

  /**
   * Time to extend or retract arm in auto
   */
  static const double ARM_EXTEND_TIME_S = 2.0;

  /**
   * Time to throw game piece in auto
   */
  static const double AUTO_THROW_TIME_S = 0.375;

  /**
   * Time to drive back in auto
   */
  static const double AUTO_DRIVE_TIME = 6.0;

  /**
   * Speed to drive backwards in auto
   */
  static const double AUTO_DRIVE_SPEED = -0.25;
  //--------------------------------------------------------------------



  //creating 4 motors as it is in the robot
  rev::CANSparkMax m_leftMotor1{1, rev::CANSparkMax::MotorType::kBrushed};
  rev::CANSparkMax m_leftMotor2{2, rev::CANSparkMax::MotorType::kBrushed};
  rev::CANSparkMax m_rightMotor1{3, rev::CANSparkMax::MotorType::kBrushed};
  rev::CANSparkMax m_rightMotor2{4, rev::CANSparkMax::MotorType::kBrushed};

  //2 motors for the intake
  rev::CANSparkMax m_leftIntake{12, rev::CANSparkMax::MotorType::kBrushless}; //need to change IDs
  rev::CANSparkMax m_rightIntake{5, rev::CANSparkMax::MotorType::kBrushless};

  //create a MotorControllerGroup to combine all left and right motors
  frc::MotorControllerGroup m_leftMotors{m_leftMotor1, m_leftMotor2};
  frc::MotorControllerGroup m_rightMotors{m_rightMotor1, m_rightMotor2};

  //Combine both motor groups
  frc::DifferentialDrive m_robotDrive{m_leftMotors, m_rightMotors};
  frc::XboxController controller{0}; 

  //AUTO
  frc::Timer m_timer;

 public:
  void RobotInit() override {
    //make motors reset to 0
    m_leftMotor1.RestoreFactoryDefaults();
    m_leftMotor2.RestoreFactoryDefaults();
    m_rightMotor1.RestoreFactoryDefaults();
    m_rightMotor2.RestoreFactoryDefaults();

    m_leftIntake.RestoreFactoryDefaults();
    m_rightIntake.RestoreFactoryDefaults();

    //right motor must be inverted for it to go forward
    m_rightMotors.SetInverted(true);
    
    //list of Auto Options
    m_chooser.SetDefaultOption("NONE", kAutoDefault);
    m_chooser.AddOption("Red_Mid", kAutoFowrard1);
    m_chooser.AddOption("Red_Right", kAutoFowrard2);
    m_chooser.AddOption("Red_Left", kAutoFowrard3);
    frc::SmartDashboard::PutData("Auto Modes", &m_chooser);

    //give a 0 as initial number for intake speed
    frc::SmartDashboard::PutNumber("Intake Speed", 0.4);
    frc::SmartDashboard::PutNumber("Delay (Sec)", 0);
  }

  void TeleopPeriodic() override {
    //motor variables
    double speed = controller.GetLeftY();
    double rotation = controller.GetRightX();
    m_robotDrive.ArcadeDrive(speed, rotation);

    //initial speed of the intake will be 40%
    double intakeSpeed = frc::SmartDashboard::GetNumber("Intake Speed", 0.4);

    //check for left and right trigger
    if (controller.GetLeftTriggerAxis() > 0) {
        //intake in
        m_leftIntake.Set(-intakeSpeed);
        m_rightIntake.Set(intakeSpeed);
    }
    else if (controller.GetRightTriggerAxis() > 0) {
        //intake out
        m_leftIntake.Set(intakeSpeed);
        m_rightIntake.Set(-intakeSpeed);
    }
    else {
        //if none, set to 0
        m_leftIntake.Set(0);
        m_rightIntake.Set(0);
    }
    
  }

    //*************************************************AUTONOMUS PART*************************************************************
  void AutonomousInit() override {
    m_timer.Reset();
    m_timer.Start();
  }

  void AutonomousPeriodic() override {
    //initial values for the auto
    int x = frc::SmartDashboard::GetNumber("Delay (Sec)", 0);
    double intakeSpeed = frc::SmartDashboard::GetNumber("Intake Speed", 0.8);
    std::string selectedOption = m_chooser.GetSelected();
    //convertion of double into seconds
    units::unit_t<units::time::second, double, units::linear_scale> secondsX(x);


    if (selectedOption == "Red_Left") {
      //ROBOT MUST FACE THE DRIVER

      //What this Auto is doing:
      //1. facing driver it goes a little forward with object and release it
      //2. turn 90 degrees to the left + forward for a few meters
      //3. turn left again facing the bridge
      //4. getting up to the bridge with lower speed 
      if(m_timer.Get() < secondsX){
        m_robotDrive.ArcadeDrive(0.0, 0.0, false);
      }
      else if(m_timer.Get() < 0.2_s + secondsX){ //+0.75s is perfect time to turn 90 degrees
        m_robotDrive.ArcadeDrive(-0.6, 0.0, false); 
        //INTAKE ON
      }
      else if (m_timer.Get() < 0.85_s + secondsX){
        m_robotDrive.TankDrive(0.7, -0.7, false);
        //INAKE OFF
      }
      else if (m_timer.Get() < 1.85_s + secondsX){
        m_robotDrive.ArcadeDrive(-0.5, 0, false);
      }
      else if (m_timer.Get() < 2.3_s + secondsX){
        m_robotDrive.TankDrive(0.72, -0.72, false);
      }
      else if (m_timer.Get() < 3.7_s + secondsX){
        m_robotDrive.ArcadeDrive(-0.3, 0, false);
      }
      else if(m_timer.Get() < 15_s + secondsX){
        m_robotDrive.ArcadeDrive(0, 0, false); 
      }
    }


    else if(selectedOption == "Red_Mid"){
      //ROBOT MUST FACE THE DRIVER

      //What this Auto is doing:
      //1. facing driver it goes a little forward with object and release it
      //2. turn 180 degrees to the left + forward - slow to get through te bridge
      //3. get the object in the middle of the field
      //4. may chose to go both to the left or right.
      if(m_timer.Get() < secondsX){
        m_robotDrive.ArcadeDrive(0.0, 0.0, false);
      }
      else if(m_timer.Get() < 0.75_s + secondsX){ //0.48 if nothing else is wokring is perfect to turn right 90 degrees
        m_robotDrive.TankDrive(-0.55, 0.55, false); 
      }
      else if(m_timer.Get() < 2_s + secondsX){
          m_robotDrive.ArcadeDrive(-0.55, 0.0, false); 
      }
      else if(m_timer.Get() < 2.5_s + secondsX){
          m_robotDrive.ArcadeDrive(0, 0.0, false); 
      }
      else if(m_timer.Get() < 3.2_s + secondsX){ 
        m_robotDrive.TankDrive(0.65, -0.65, false); 
      }
      else if(m_timer.Get() < 3.6_s + secondsX){ 
        m_robotDrive.ArcadeDrive(0, 0, false);
      }
      else if(m_timer.Get() < 4.4_s + secondsX){ 
        m_robotDrive.ArcadeDrive(-.9, 0, false);
      }
      else if(m_timer.Get() < 5.65_s + secondsX){ 
        m_robotDrive.ArcadeDrive(0, 0, false);
      }
      else if(m_timer.Get() < 6.7_s + secondsX){ 
        m_robotDrive.TankDrive(-0.6, 0.6, false); 
      }
      else if(m_timer.Get() < 7_s + secondsX){ 
        m_robotDrive.TankDrive(0, 0, false); 
      }
      else if(m_timer.Get() < 9_s + secondsX){ 
        m_robotDrive.ArcadeDrive(-.5, 0, false); 
      }
      else if(m_timer.Get() < 10_s + secondsX){ 
        m_robotDrive.ArcadeDrive(0, 0, false); 
      }
      else if(m_timer.Get() < 10.75_s + secondsX){ 
        m_robotDrive.TankDrive(-0.6, 0.6, false); 
      }
      else if(m_timer.Get() < 15_s + secondsX){
        m_robotDrive.ArcadeDrive(0, 0, false); 
      }
    }


    else if(selectedOption == "Red_Right"){
      //ROBOT MUST FACE THE DRIVER

      //What this Auto is doing:
      //1. facing driver it goes a little forward with object and release it
      //2. turn 180 degrees to the left + forward until obejct is hit
      //3. capture the object and turn 180 degrees
      //4. go back and put the object in different place
      if(m_timer.Get() < secondsX){
        m_robotDrive.ArcadeDrive(0.0, 0.0, false);
      }
      else if(m_timer.Get() < 0.2_s + secondsX){
        m_robotDrive.ArcadeDrive(-0.6, 0.0, false); 
      }
      else if (m_timer.Get() < 0.5_s + secondsX){
        m_robotDrive.ArcadeDrive(0.0, 0.0, false); 
          m_leftIntake.Set(intakeSpeed);
          m_rightIntake.Set(-intakeSpeed);
      }
      else if(m_timer.Get() < 1.3_s + secondsX){
          m_robotDrive.TankDrive(-.9, .9, false); //worked on 0.76 and 0.79
          m_leftIntake.Set(0);
          m_rightIntake.Set(0);
      }
      else if(m_timer.Get() < 1.5_s + secondsX){
          m_robotDrive.TankDrive(0, 0.0, false);
      }
      else if(m_timer.Get() < 3_s + secondsX){
          m_robotDrive.ArcadeDrive(-0.65, 0.0, false); 
          m_leftIntake.Set(intakeSpeed);
          m_rightIntake.Set(-intakeSpeed);
      }
      else if(m_timer.Get() < 5_s + secondsX){
          m_robotDrive.ArcadeDrive(0.0, 0.0, false); 
      }
      else if(m_timer.Get() < 6_s + secondsX){
          m_robotDrive.TankDrive(-.8, .8, false); 
      }
      else if(m_timer.Get() < 6.1_s + secondsX){
          m_robotDrive.TankDrive(0, 0, false); 
          m_leftIntake.Set(0);
          m_rightIntake.Set(0);
      }
      else if(m_timer.Get() < 7.35_s + secondsX){
          m_robotDrive.ArcadeDrive(-0.75, 0.0, false); 
      }
      else if(m_timer.Get() < 8.1_s + secondsX){
          m_robotDrive.ArcadeDrive(0, 0.0, false); 
          m_leftIntake.Set(intakeSpeed);
          m_rightIntake.Set(-intakeSpeed);
      }
      else if(m_timer.Get() < 9.1_s + secondsX){
          m_leftIntake.Set(0);
          m_rightIntake.Set(0);
      }
      else if(m_timer.Get() < 15_s + secondsX){
          m_robotDrive.TankDrive(0, 0.0, false);
      }
    }
    
  }
  //**************************************************************************************************************************
};

#ifndef RUNNING_FRC_TESTS
int main() {
  return frc::StartRobot<Robot>();
}
#endif
