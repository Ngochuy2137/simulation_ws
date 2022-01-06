#include <nav_msgs/Path.h>
#include <nav_msgs/OccupancyGrid.h>
// #include<geometry_msgs/PoseStamped.h>
#include "std_msgs/String.h"
#include <typeinfo>
#include <ros/ros.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <tf2_ros/transform_listener.h>
#include<thread>

#include <ros/duration.h>


using namespace std;
struct worldPosition
{
  float x;
  float y;
};
struct mapPosition
{
  unsigned int x;
  unsigned int y;
};

class costPose
{
public:
  worldPosition w_position;
  mapPosition m_position;
  unsigned char cost;
};

const int num_poses = 100;

nav_msgs::Path checked_path;
// geometry_msgs::PoseStamped checked_pose;

costPose cost_pose[num_poses];

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
    // cout << checked_path.poses[i].pose.w_position.x << endl;
    cost_pose[i].w_position.x = checked_path.poses[i].pose.position.x;
    cost_pose[i].w_position.y = checked_path.poses[i].pose.position.y;
  }
  // cout << " pose" << cost_pose[1].w_position.x << " " << cost_pose[1].w_position.y << endl;
  
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
    cout << "====================   CHECK   ====================" << endl;
    for(int i = 0; i<num_poses; i++)
    {
      cout << "-----------" << i << "-----------" << endl;
      costmap->worldToMap(cost_pose[i].w_position.x, cost_pose[i].w_position.y, cost_pose[i].m_position.x, cost_pose[i].m_position.y);
      cost_pose[i].cost = costmap->getCost(cost_pose[i].m_position.x, cost_pose[i].m_position.y);

      cout << "World Pose " << cost_pose[i].w_position.x << " " << cost_pose[i].w_position.y << endl;
      cout << " Map Pose " << cost_pose[i].m_position.x << " " << cost_pose[i].m_position.y << endl;
      cout << " Cost: " << cost_pose[i].cost << endl;
    }
    // cout << "---------------------" << endl;
    // cout << "World Pose " << cost_pose[10].w_position.x << " " << cost_pose[10].w_position.y << endl;
    // cout << " Map Pose " << cost_pose[10].m_position.x << " " << cost_pose[10].m_position.y << endl;
    // cout << " Cost: " << cost_pose[10].cost << endl;
   
    ros::spinOnce();
    ros::Duration(1.0).sleep();
  }

  return 0;
}
