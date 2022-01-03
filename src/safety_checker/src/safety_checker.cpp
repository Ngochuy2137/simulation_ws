#include <nav_msgs/Path.h>
#include<geometry_msgs/PoseStamped.h>
#include "ros/ros.h"
#include "std_msgs/String.h"
#include <iostream>
#include <typeinfo>

using namespace std;

// class path_poses
// {
// public:
//   int num_poses = 3000;
//   // geometry_msgs::PoseStamped poses[num_poses];
//   // bool check_obstacles(geometry_msgs::PoseStamped checked_poses);
//   // path_poses();
//   // ~path_poses();
// };
int num_poses = 3000;
// geometry_msgs::PoseStamped path_pose[num_poses];

// void path_poses::check_obstacles(geometry_msgs::PoseStamped checked_pose)
// {
  
// }
nav_msgs::Path checked_path;
geometry_msgs::PoseStamped checked_pose;


void get_path_cb(const nav_msgs::Path::ConstPtr& msg)
{
  // ROS_INFO("Checking path=================================");

  //  ĐẾM SỐ CELLS TRONG PATH
    // cout << "size *msg:" << sizeof(*msg) << endl;
    // cout << "size msg->poses[0]:" << sizeof(msg->poses[0]) << endl;

    // cout << msg->poses[0].pose << endl;
  checked_path = *msg;
  
  cout << checked_path << endl;
  for(int i = 0; i<100; i++)
  {
    cout << "=========" << i << "=========" << endl;
    cout << checked_path.poses[i] << endl;
    // checked_pose
  }


  // nav_msgs::Path msg2;

    // ROS_INFO("I heard: [%s]", msg->header);

    // SAMPLE
    // cout << typeid(msg->data).name() << endl;
  
    // cout << typeid(msg->data).name() << endl;
    
    // ROS_INFO("I heard: [%s]", msg->data.c_str());

    // cout << "size *msg:" << sizeof(*msg) << endl;
}

// void safety_checker_cb()
// {

// }

// bool check obs_on_path();

int main(int argc, char **argv)
{
  ros::init(argc, argv, "safety_checker");
  ros::NodeHandle n;
  ros::Subscriber sub_path = n.subscribe("/move_base/NavfnROS/plan", 1, get_path_cb);

  // ros::Subscriber sub_costmap = n.subscribe("/move_base/local_costmap/costmap", 1, safety_checker_cb);
  ros::spin();

  return 0;
}