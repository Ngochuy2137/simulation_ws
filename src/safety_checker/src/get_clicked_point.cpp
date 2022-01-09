#include <nav_msgs/Path.h>
#include <nav_msgs/OccupancyGrid.h>
#include "std_msgs/String.h"
#include <typeinfo>
#include <ros/ros.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/PointStamped.h>
#include <thread>

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

class checkedPose
{
public:
  worldPosition w_position;
  mapPosition m_position;
  unsigned int cost;
};

const int num_poses = 100;
nav_msgs::Path checked_path;
checkedPose checked_poses[num_poses];
checkedPose checked_pose;

void get_checked_pose_cb(const geometry_msgs::PointStamped::ConstPtr& msg)
{
    checked_pose.w_position.x = msg->point.x;
    checked_pose.w_position.y = msg->point.y;

    cout << "[ " << checked_pose.w_position.x << " " << checked_pose.w_position.y << " ]"<< endl;
}
void get_path_cb(const nav_msgs::Path::ConstPtr& msg)
{
  checked_path = *msg;
  
  for(int i = 0; i<num_poses; i++)
  {
    checked_poses[i].w_position.x = checked_path.poses[i].pose.position.x;
    checked_poses[i].w_position.y = checked_path.poses[i].pose.position.y;
  }  
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "safety_checker");
  ros::NodeHandle n;
  ros::Subscriber sub_pose = n.subscribe("/clicked_point", 1, get_checked_pose_cb);

  ros::spin();
  return 0;
}
