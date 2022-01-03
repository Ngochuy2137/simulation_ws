#include <nav_msgs/Path.h>
#include "ros/ros.h"
#include "std_msgs/String.h"
#include <iostream>
using namespace std;
void safety_checker_cb(const nav_msgs::Path::ConstPtr& msg)
{
//   ROS_INFO("I heard: [%s]", msg->data.c_str());
    cout << "READING MESSAGE FROM PATH: \n" << endl;
    cout << msg[0] << endl;
}

// bool check obs_on_path();

int main(int argc, char **argv)
{
  ros::init(argc, argv, "safety_checker");
  ros::NodeHandle n;
  ros::Subscriber sub = n.subscribe("/move_base/NavfnROS/plan", 1000, safety_checker_cb);
  ros::spin();

  return 0;
}