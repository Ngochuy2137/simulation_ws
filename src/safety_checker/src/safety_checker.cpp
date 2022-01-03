#include <nav_msgs/Path.h>
#include<geometry_msgs/PoseStamped.h>
#include "ros/ros.h"
#include "std_msgs/String.h"
#include <iostream>
#include <typeinfo>

using namespace std;

class path_poses
{
public:
  int num_poses = 3000;
  // geometry_msgs::PoseStamped poses[num_poses];
  // bool check_obstacles(geometry_msgs::PoseStamped checked_poses);
  // path_poses();
  // ~path_poses();
};

// void path_poses::check_obstacles(geometry_msgs::PoseStamped checked_pose)
// {
  
// }



void safety_checker_cb(const nav_msgs::Path::ConstPtr& msg)
{
  ROS_INFO("Checking path");
    // nav_msgs::Path msg2;
    // cout << "READING MESSAGE FROM PATH: \n" << endl;
  // cout << msg->poses[0] << endl;
  cout << (*msg) << endl;

    // ROS_INFO("I heard: [%s]", msg->header);

    // SAMPLE
    // cout << typeid(msg->data).name() << endl;
  
    // cout << typeid(msg->data).name() << endl;
    
    // ROS_INFO("I heard: [%s]", msg->data.c_str());
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