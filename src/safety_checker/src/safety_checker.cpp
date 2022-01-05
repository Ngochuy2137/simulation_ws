#include <nav_msgs/Path.h>
#include <nav_msgs/OccupancyGrid.h>
// #include<geometry_msgs/PoseStamped.h>
// #include "ros/ros.h"
#include "std_msgs/String.h"
// #include <iostream>
#include <typeinfo>
// #include <costmap_2d/costmap_2d.h>
#include <ros/ros.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <tf2_ros/transform_listener.h>
// #include <tf2_ros/buffer.h>
// #include <tf2_ros/buffer_client.h>
// #include <tf2_ros/buffer_interface.h>
// #include <tf2_ros/buffer_server.h>
#include<thread>

// undefined reference to `vtable for tf2_ros::Buffer'
#include <ros/duration.h>


using namespace std;
struct Point
{
  float x;
  float y;
};
struct localPoint
{
  unsigned int x;
  unsigned int y;
  unsigned char cost;
};

class costPose
{
public:
  Point point;
  localPoint local_point;
};

const int num_poses = 100;

nav_msgs::Path checked_path;
// geometry_msgs::PoseStamped checked_pose;
costPose cost_pose[num_poses];

float CHECK = 10;
void get_path_cb(const nav_msgs::Path::ConstPtr& msg)
{
  // ROS_INFO("Checking path=================================");

  //  TO-DO: 
    // ĐẾM SỐ CELLS TRONG PATH
    // cout << "size *msg:" << sizeof(*msg) << endl;
    // cout << "size msg->poses[0]:" << sizeof(msg->poses[0]) << endl;
    // cout << msg->poses[0].pose << endl;

  checked_path = *msg;
  
  for(int i = 0; i<num_poses; i++)
  {
    // cout << "=========" << i << "=========" << endl;
    // cout << checked_path.poses[i].pose.position.x << endl;
    cost_pose[i].point.x = checked_path.poses[i].pose.position.x;
    cost_pose[i].point.y = checked_path.poses[i].pose.position.y;
  }
  cout << " pose" << cost_pose[1].point.x << " " << cost_pose[1].point.y << endl;
  
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "safety_checker");
  ros::NodeHandle n;
  ros::Subscriber sub_path = n.subscribe("/move_base/NavfnROS/plan", 1, get_path_cb);
  // ros::Subscriber sub_costmap = n.subscribe("/move_base/local_costmap/costmap", 1, safety_checker_cb);
  tf2_ros::Buffer buffer(ros::Duration(10));
  tf2_ros::TransformListener tf(buffer);

  costmap_2d::Costmap2DROS safety_costmap("safety_costmap", buffer);
  costmap_2d::Costmap2D* costmap = safety_costmap.getCostmap();
  int count = 0;
  while(ros::ok())
  {
    
    //get a copy of the costmap contained by our ros wrapper
    for(int i = 0; i<num_poses; i++)
    {
      costmap->worldToMap(cost_pose[i].point.x, cost_pose[i].point.y, cost_pose[i].local_point.x,cost_pose[i].local_point.y);
      cost_pose[i].local_point.cost = costmap->getCost(cost_pose[i].local_point.x, cost_pose[i].local_point.y);
    }
    cout << "Pose " << cost_pose[10].point.x << " " << cost_pose[10].point.y << endl;
    cout << "local Pose " << cost_pose[10].local_point.x << " " << cost_pose[10].local_point.y << endl;
    // cout << "COST " << cost_pose[10].local_point.cost << endl;
    // for(int i=0; i< num_poses; i++)
    // {
    //   cout << "OUT " << cost_pose[1].point.x << " " << cost_pose[1].point.y << endl;
    // }
    count ++;
    ros::spinOnce();
    ros::Duration(1.0).sleep();
  }

  return 0;
}
