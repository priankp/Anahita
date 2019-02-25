#include <ros/ros.h>
#include <iostream>
#include <std_msgs/Float32.h>
#include <dynamic_reconfigure/server.h>
#include <pid_calibration/pidConfig.h>
#include <errorToPWM.h>

double x;
double y;
double z;
double roll;
double pitch;
double yaw;

bool send_goal = false;
double p = 0;
double i = 0;
double d = 0;
double target = 0;
double band = 0;

ErrorDescriptor yaw_ ("YAW");
ErrorDescriptor roll_ ("ROLL");
ErrorDescriptor pitch_ ("PITCH");
ErrorDescriptor x_coord ("X_COORD");
ErrorDescriptor y_coord ("Y_COORD");
ErrorDescriptor z_coord ("Z_COORD");

std::string action = "yaw";

void xCB (const std_msgs::Float32 msg) {
    x = msg.data;
    x_coord.errorToPWM (x);
}

void yCB (const std_msgs::Float32 msg) {
    y = msg.data;
    y_coord.errorToPWM (y);
}

void zCB (const std_msgs::Float32 msg) {
    z = msg.data;
    z_coord.errorToPWM (z);
}

void rollCB (const std_msgs::Float32 msg) {
    roll = msg.data;
    roll_.errorToPWM (roll);
}

void pitchCB (const std_msgs::Float32 msg) {
    pitch = msg.data;
    pitch_.errorToPWM (pitch);
}

void yawCB (const std_msgs::Float32 msg) {
    yaw = msg.data;
    yaw_.errorToPWM (yaw);
}

void setTarget (double target) {
    
    if (action == "yaw") { yaw_.setReference (target); }
    if (action == "roll") { roll_.setReference (target); }
    if (action == "pitch") { pitch_.setReference (target); }
    if (action == "surge") { x_coord.setReference (target); }
    if (action == "heave") { z_coord.setReference (target); }
    if (action == "sway") { y_coord.setReference (target); }
}

void callback (pid_calibration::pidConfig &config, uint32_t level) {
    target = config.target;
    if (config.send_goal) {
        setTarget (target);
        config.send_goal = false;
    }
    p = config.p;
    i = config.i;
    d = config.d;
    band = config.band;

    if (action == "yaw") { yaw_.setPID (p, i, d, band); }
    if (action == "roll") { roll_.setPID (p, i, d, band); }
    if (action == "pitch") { pitch_.setPID (p, i, d, band); }
    if (action == "surge") { x_coord.setPID (p, i, d, band); }
    if (action == "heave") { y_coord.setPID (p, i, d, band); }
    if (action == "sway") { z_coord.setPID (p, i, d, band); }
}

int main (int argc, char** argv) {
    ros::init (argc, argv, "pid_calibrate");
    ros::NodeHandle nh;

    if (action == "surge") { ros::Subscriber x_sub = nh.subscribe<std_msgs::Float32>("/anahita/x_coordinate", 1, &xCB); }
    if (action == "heave") { ros::Subscriber z_sub = nh.subscribe<std_msgs::Float32>("/anahita/z_coordinate", 1, &zCB); }
    if (action == "sway") { ros::Subscriber y_sub = nh.subscribe<std_msgs::Float32>("/anahita/y_coordinate", 1, &yCB); }
    if (action == "roll") { ros::Subscriber roll_sub = nh.subscribe<std_msgs::Float32>("/anahita/imu/pitch", 1, &rollCB); }
    if (action == "pitch") { ros::Subscriber pitch_sub = nh.subscribe<std_msgs::Float32>("/anahita/imu/roll", 1, &pitchCB); }
    if (action == "yaw") { ros::Subscriber yaw_sub = nh.subscribe<std_msgs::Float32>("/anahita/imu/yaw", 1, &yawCB); }
    
    // register dynamic reconfig server.
    dynamic_reconfigure::Server<pid_calibration::pidConfig> server;
    dynamic_reconfigure::Server<pid_calibration::pidConfig>::CallbackType f;
    f = boost::bind(&callback, _1, _2);
    server.setCallback(f);

    ros::Rate loop_rate (20);

    int pwm_yaw = 0;
    int pwm_roll = 0;
    int pwm_pitch = 0;
    int pwm_surge = 0;
    int pwm_heave = 0;
    int pwm_sway = 0;

    while (ros::ok()) {

        if (action == "sway") { pwm_sway = y_coord.getPWM (); }
        if (action == "surge") { pwm_surge = x_coord.getPWM (); }
        if (action == "heave") { pwm_heave = z_coord.getPWM (); }
        if (action == "roll") { pwm_roll = roll_.getPWM (); }
        if (action == "pitch") { pwm_pitch = pitch_.getPWM (); }
        if (action == "yaw") { pwm_yaw = yaw_.getPWM (); }

        nh.setParam ("/pwm_sway", pwm_sway);
        nh.setParam ("/pwm_surge", pwm_surge);
        nh.setParam ("/pwm_heave", pwm_heave);
        nh.setParam ("/pwm_roll", pwm_roll);
        nh.setParam ("/pwm_yaw", pwm_yaw);
        nh.setParam ("/pwm_pitch", pwm_pitch);

        loop_rate.sleep ();
        ros::spinOnce ();
    }

    ros::spin();

    return 0;
}