#include <nav_msgs/Path.h>
#include <nav_msgs/OccupancyGrid.h>
#include "std_msgs/String.h"
// #include "std_msgs/Int8.h"
#include <typeinfo>
#include <ros/ros.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/PointStamped.h>
#include <thread>
#include <ros/duration.h>

using namespace std;

class mapInfo
{
public:
  int origin_x;
  int origin_y;
  float resolution;
  int width;
  int height;

  mapInfo();
  mapInfo(int origin_x_, int origin_y_, float resolution_, int width_, int height_);
  ~mapInfo(){};

  void set_info(int origin_x_, int origin_y_, float resolution_, int width_, int height_);
};

mapInfo::mapInfo()
{
  this->origin_x = 0;
  this->origin_y = 0;
  this->resolution = 0.0;
  this->width = 0;
  this->height = 0;
}
mapInfo::mapInfo(int origin_x_, int origin_y_, float resolution_, int width_, int height_)
{
  this->origin_x = origin_x_;
  this->origin_y = origin_y_;
  this->resolution = resolution_;
  this->width = width_;
  this->height = height_;
}
void mapInfo::set_info(int origin_x_, int origin_y_, float resolution_, int width_, int height_)
{
  this->origin_x = origin_x_;
  this->origin_y = origin_y_;
  this->resolution = resolution_;
  this->width = width_;
  this->height = height_;
}


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
  // std_msgs::Int8 cost;
  unsigned int cost;

};

const int num_poses = 100;
nav_msgs::Path checked_path;
// geometry_msgs::PoseStamped checkedPose;
checkedPose checked_poses[num_poses];
checkedPose checked_pose;
mapInfo map_info;


bool my_worldToMap(double wx, double wy, unsigned int& mx, unsigned int& my)
{
    
  if (wx < map_info.origin_x || wy < map_info.origin_y)
  {
    // cout << "ERROR 1" << endl;
    return false;
  }
//   cout << "HELLO ================================" << endl;
//   cout << "mx my:" << int((wx - origin_x_) / resolution_) << " " << int((wy - origin_y_) / resolution_) << endl;
  mx = int((wx - map_info.origin_x) / map_info.resolution);
  my = int((wy - map_info.origin_y) / map_info.resolution);

  if (mx < map_info.width && my < map_info.height)
  {
    // cout << "ERROR 2" << endl;
    return true;
  }

//   cout << "mx my:" << mx << " " << my << endl;
//   cout << "ERROR 3" << endl;
  return false;
}

void get_checked_pose_cb(const geometry_msgs::PointStamped::ConstPtr& msg)
{
  checked_pose.w_position.x = msg->point.x;
  checked_pose.w_position.y = msg->point.y;
  bool a = my_worldToMap(checked_pose.w_position.x, checked_pose.w_position.y, checked_pose.m_position.x, checked_pose.m_position.y);
  cout << "WORLD [ " << checked_pose.w_position.x << " " << checked_pose.w_position.y << " ]"<< endl;
  cout << "   map [ " << checked_pose.m_position.x << " " << checked_pose.m_position.y << " ]"<< endl;
}
// void get_path_cb(const nav_msgs::Path::ConstPtr& msg)
// {
//   checked_path = *msg;
  
//   for(int i = 0; i<num_poses; i++)
//   {
//     checked_poses[i].w_position.x = checked_path.poses[i].pose.position.x;
//     checked_poses[i].w_position.y = checked_path.poses[i].pose.position.y;
//   }  
// }

void check_costmap_cb(const nav_msgs::OccupancyGrid::ConstPtr& msg)
{
  nav_msgs::OccupancyGrid costmap = *msg;

  map_info.set_info( int(costmap.info.origin.position.x),
            int(costmap.info.origin.position.y),
            float(costmap.info.resolution),
            // float(0.05),
            int(costmap.info.width),
            int(costmap.info.height)
  );

  // map_info( int(1),
  //           int(1),
  //           // float(costmap.info.resolution),
  //           float(0.05),
  //           int(1),
  //           int(1);
  cout << "DES: " << map_info.origin_x << " " << map_info.origin_y << " "<< map_info.resolution << " "<< map_info.width << " "<< map_info.height << endl;
  cout << "SRC: " << costmap.info.origin.position.x << " " << costmap.info.origin.position.y << " "<< costmap.info.resolution << " "<< costmap.info.width << " "<< costmap.info.height << endl;
  // int index = checked_pose.m_position.x + checked_pose.m_position.y * costmap.info.width;
  // checked_pose.cost = costmap.data[index];
  
  // cout << "WORLD [ " << checked_pose.w_position.x << " " << checked_pose.w_position.y << " ]"<< endl;
  // cout << "   2 map [ " << checked_pose.m_position.x << " " << checked_pose.m_position.y << " ]"<< endl;
  // cout << "   2 cost:" << checked_pose.cost << endl;

}


int main(int argc, char **argv)
{
  ros::init(argc, argv, "safety_checker");
  ros::NodeHandle n;
  ros::Subscriber sub_costmap = n.subscribe("/move_base/global_costmap/costmap", 1, check_costmap_cb);
  ros::Subscriber sub_pose = n.subscribe("/clicked_point", 1, get_checked_pose_cb);
//   ros::Subscriber sub_path = n.subscribe("/move_base/NavfnROS/plan", 1, get_path_cb);
//   tf2_ros::Buffer buffer(ros::Duration(10));
//   tf2_ros::TransformListener tf(buffer);

//   costmap_2d::Costmap2DROS safety_costmap("safety_costmap", buffer);
//   costmap_2d::Costmap2D* costmap = safety_costmap.getCostmap();

  ros::spin();
  return 0;
}
