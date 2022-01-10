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
#define COSTMAP_TOPIC "/move_base/global_costmap/costmap"

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
  unsigned int cost;

};

//------------------    Variable declarations   ------------------//
const int num_poses = 100;
nav_msgs::Path checked_path;
checkedPose checked_poses[num_poses];
checkedPose checked_pose;
mapInfo map_info;
nav_msgs::OccupancyGrid costmap;
//------------------    Funtion declarations   ------------------//
bool my_worldToMap(double wx, double wy, unsigned int& mx, unsigned int& my)
{
  if (wx < map_info.origin_x || wy < map_info.origin_y)
  {
    cout << "converting ERROR 1" << endl;
    cout << "   wx wy:  " << wx << " " << wy << endl;
    cout << "   origin: " << map_info.origin_x << " " << map_info.origin_y << endl;
    cout << "   ---" << endl;
    return false;
  }
  mx = int((wx - map_info.origin_x) / map_info.resolution);
  my = int((wy - map_info.origin_y) / map_info.resolution);

  if (mx < map_info.width && my < map_info.height)
  {
    return true;
  }
  cout << "converting ERROR 2" << endl;
  cout << "   mx my:  " << mx << " " << my << endl;
  cout << "   size:   " << map_info.width << " " << map_info.height << endl;
  cout << "   ---" << endl;
  return false;
}

void get_checked_pose_cb(const geometry_msgs::PointStamped::ConstPtr& msg)
{
  checked_pose.w_position.x = msg->point.x;
  checked_pose.w_position.y = msg->point.y;
  bool a = my_worldToMap(checked_pose.w_position.x, checked_pose.w_position.y, checked_pose.m_position.x, checked_pose.m_position.y);
  if(a)
  {
    int index = checked_pose.m_position.x + checked_pose.m_position.y * map_info.width;
    checked_pose.cost = costmap.data[index];
    cout << "WORLD [ " << checked_pose.w_position.x << " " << checked_pose.w_position.y << " ]"<< endl;
    cout << "   map [ " << checked_pose.m_position.x << " " << checked_pose.m_position.y << " ]"<< endl;
    cout << "   cost: " << checked_pose.cost << endl;
  }
}

void check_costmap_cb(const nav_msgs::OccupancyGrid::ConstPtr& msg)
{
  costmap = *msg;
  map_info.set_info( int(costmap.info.origin.position.x),
            int(costmap.info.origin.position.y),
            float(costmap.info.resolution),
            int(costmap.info.width),
            int(costmap.info.height));
  cout << "LOADING " << COSTMAP_TOPIC << endl;
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "safety_checker");
  ros::NodeHandle n;
  ros::Subscriber sub_costmap = n.subscribe(COSTMAP_TOPIC, 1, check_costmap_cb);
  ros::Subscriber sub_pose = n.subscribe("/clicked_point", 1, get_checked_pose_cb);
  ros::spin();
  return 0;
}
