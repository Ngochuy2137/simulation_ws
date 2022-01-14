#include "std_msgs/String.h"
#include <geometry_msgs/PointStamped.h>
#include <nav_msgs/Path.h>
#include <nav_msgs/OccupancyGrid.h>
#include "geometry_msgs/Quaternion.h"
#include "geometry_msgs/Twist.h"
#include <map_msgs/OccupancyGridUpdate.h>
#include <visualization_msgs/Marker.h>

#include <typeinfo>
#include <ros/ros.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <tf2_ros/transform_listener.h>
#include <tf2/transform_datatypes.h>

#include <thread>
#include <ros/duration.h>
#include <Eigen/Dense>
#include <vector>
#define COSTMAP_INFO_TOPIC "/move_base/local_costmap/costmap"
#define COSTMAP_TOPIC "/move_base/local_costmap/costmap_updates"
#define SAFETY_CMD_VEL_TOPIC "cmd_vel/safety_checker"
#define SAFE_RESOLUTION  0.05    //  Resolution of safe zone
#define obs_count_threshold 6

using namespace std;
using namespace Eigen;

//  ---------------------------------------
//  some variable and function declarations
geometry_msgs::TransformStamped transformStamped_mapToOdom;
geometry_msgs::TransformStamped transformStamped_baseFootprintToOdom;

double quaternionToRPY(float qx, float qy, float qz, float qw);
//  ---------------------------------------


// double quaternionToRPY(float qx, float qy, float qz, float qw);
class mapInfo
{
public:
  float origin_x;
  float origin_y;
  float resolution;
  int width;
  int height;

  mapInfo();
  mapInfo(float origin_x_, float origin_y_, float resolution_, int width_, int height_);
  ~mapInfo(){};

  void set_info(float origin_x_, float origin_y_, float resolution_, int width_, int height_);
};

mapInfo::mapInfo()
{
  this->origin_x = 0;
  this->origin_y = 0;
  this->resolution = 0.0;
  this->width = 0;
  this->height = 0;
}
mapInfo::mapInfo(float origin_x_, float origin_y_, float resolution_, int width_, int height_)
{
  this->origin_x = origin_x_;
  this->origin_y = origin_y_;
  this->resolution = resolution_;
  this->width = width_;
  this->height = height_;
}
void mapInfo::set_info(float origin_x_, float origin_y_, float resolution_, int width_, int height_)
{
  this->origin_x = origin_x_;
  this->origin_y = origin_y_;
  this->resolution = resolution_;
  this->width = width_;
  this->height = height_;
}

class checkedPoint
{
private:
  
public:
  // position in base_footprint tf frame
  Vector2d base_footprint_frame;
  // position in map tf frame
  Vector2d map_frame;
  // position in cell map
  Vector2i cell_map;
  // position in odom tf frame
  Vector2d odom_frame;
  // cost value;
  unsigned int cost;

  checkedPoint()
  {
    base_footprint_frame(0) = 0.0;
    base_footprint_frame(1) = 0.0;

    map_frame(0) = 0.0;
    map_frame(0) = 0.0;

    cell_map(0) = 0;
    cell_map(0) = 0;

    odom_frame(0) = 0.0;
    odom_frame(0) = 0.0;

    cost = 0;
  }
  ~checkedPoint(){};
  
  bool odomFrameToCellMap(mapInfo map_info_)
  {
    // cout << "map configuration: [" << map_info_.origin_x << " " << map_info_.origin_y << "] " << map_info_.resolution << " " << map_info_.width << "x" << map_info_.height << endl;
    if (odom_frame(0) < map_info_.origin_x || odom_frame(1) < map_info_.origin_y)
    {
      ROS_ERROR("converting ERROR 1");
      cout << "   [odom_frame(0) odom_frame(1)]:  " << odom_frame(0) << " " << odom_frame(1) << endl;
      cout << "   [cell_map(0) cell_map(1)]:  " << cell_map(0) << " " << cell_map(1) << endl;
      cout << "   origin: " << map_info_.origin_x << " " << map_info_.origin_y << endl;
      cout << "   size:   " << map_info_.width << " " << map_info_.height << endl;
      cout << "   resolution:   " << map_info_.resolution << endl;
      cout << "   ---" << endl;
      return false;
    }
    cell_map(0) = int((odom_frame(0) - map_info_.origin_x) / map_info_.resolution);
    cell_map(1) = int((odom_frame(1) - map_info_.origin_y) / map_info_.resolution);

    if (int(cell_map(0)) < map_info_.width && int(cell_map(1)) < map_info_.height)
    {
      return true;
    }
    ROS_ERROR("converting ERROR 2");
    cout << "   [odom_frame(0) odom_frame(1)]:  " << odom_frame(0) << " " << odom_frame(1) << endl;
    cout << "   [cell_map(0) cell_map(1)]:  " << cell_map(0) << " " << cell_map(1) << endl;
    cout << "   origin: " << map_info_.origin_x << " " << map_info_.origin_y << endl;
    cout << "   size:   " << map_info_.width << " " << map_info_.height << endl;
    cout << "   resolution:   " << map_info_.resolution << endl;
    cout << "   ---" << endl;
    return false;
  }

  // convert coordination from map to odom
  void mapFrameToOdomFrame(float x_trans, float y_trans, double theta)
  {
    MatrixXd map_point(3,1);
    MatrixXd odom_point(3,1);
    MatrixXd transform_matrix(3,3);

    map_point <<  map_frame(0), 
                  map_frame(1), 
                  1;
    transform_matrix << cos(theta), -sin(theta), x_trans,
                        sin(theta), cos(theta), y_trans,
                        0,          0,          1;

    odom_point = transform_matrix * map_point;
    // cout << " map_point: \n" << map_point << endl; 
    // cout << " odom_point: \n" << odom_point << endl; 

    odom_frame(0) = odom_point(0, 0);
    odom_frame(1) = odom_point(1, 0);
  }
  void baseFootprintFrameToOdomFrame(float x_trans, float y_trans, double theta)
  {
    MatrixXd base_foot_print_point(3,1);
    MatrixXd odom_point(3,1);
    MatrixXd transform_matrix(3,3);

    base_foot_print_point <<  base_footprint_frame(0), 
                              base_footprint_frame(1), 
                              1;
    transform_matrix << cos(theta), -sin(theta), x_trans,
                        sin(theta), cos(theta), y_trans,
                        0,          0,          1;

    odom_point = transform_matrix * base_foot_print_point;
    odom_frame(0) = odom_point(0, 0);
    odom_frame(1) = odom_point(1, 0);
  }
};

mapInfo map_info;
nav_msgs::OccupancyGrid costmap_info;
map_msgs::OccupancyGridUpdate costmap;

class triangleSafeZone
{
  /*
                 I
                / \
               /   \
              /     \
             /  Safe \     
            /   zone  \
          A ----------- B 
  */
private:
  // vertices A, I, B are in base_footprint_frame coordination
  Vector2d vertice_A;      //  Base left vertice
  Vector2d vertice_I;      //  Top vertice
  Vector2d vertice_B;      //  Base right vertice
public:
  double height;
  double base_length;
  double resolution;
  unsigned int num_safe_point;
  bool obstacle_flag;
  // an array of points in the safe zone
  vector<checkedPoint> safe_triangle;

  triangleSafeZone(): height(0), base_length(0), resolution(SAFE_RESOLUTION), num_safe_point(0), obstacle_flag(false)
  {
    ROS_INFO("Created a triangle safe zone");
  }
  triangleSafeZone(Vector2d vertice_A_, Vector2d vertice_I_, Vector2d vertice_B_)
  {
    ROS_INFO("Created a triangle safe zone with vertices: ");
    cout << "----------" << endl;
    cout << "          I(" << vertice_I_(0) << " " << vertice_I_(1) << ")" << endl;
    cout << "           / \\                      ^ x" << endl;
    cout << "          /   \\             base    |" << endl;
    cout << "         /     \\         foot_print |" << endl;
    cout << "        /  Safe \\                   |" << endl;     
    cout << "       /   zone  \\       <----------'" << endl;
    cout << "      A --------- B         y         O"  << endl;
    cout << "(" << vertice_A_(0) << " " << vertice_A_(1) << ")    (" << vertice_B_(0) << " " << vertice_B_(1) << ")\n\n" << endl; 

    vertice_A = vertice_A_;
    vertice_I = vertice_I_;
    vertice_B = vertice_B_;

    base_length = vertice_A_(1) - vertice_B_(1);
    height      = vertice_I_(0) - vertice_A_(0);
    // cout << "Vertice A: " << vertice_A_(1) << " " << vertice_B_(1) << endl;

    // cout << "   height: " << height << endl;
    // cout << "   base:   " << base_length << endl;

    if(base_length <0 || height <0)
    {
      ROS_WARN("Triangle base_length or height are not available !!!");
    }
    resolution = SAFE_RESOLUTION;
    obstacle_flag = false;
    initialize_safe_matrix();
    // check_safety();
  }
  ~triangleSafeZone(){};

  void set_vertices(Vector2d vertice_A_, Vector2d vertice_I_, Vector2d vertice_B_)
  {
    vertice_A = vertice_A_;
    vertice_I = vertice_I_;
    vertice_B = vertice_B_;

    base_length = vertice_B_(1) - vertice_A_(1);
    height      = vertice_I_(1) - vertice_A_(1);
    resolution = SAFE_RESOLUTION;
    initialize_safe_matrix();
    // check_safety();
  }
  
  Vector2d get_vertice(string V)
  {
    if(V == "vertice_A" || V == "A")  return vertice_A;
    else if(V == "vertice_B" || V == "B")  return vertice_B;
    else if(V == "vertice_I" || V == "I")  return vertice_I;
    else
    {
      ROS_WARN("Unavailable vertice !!!");
    }
  }
  
  //  Find safe zone with safe point in base_foot_print frame
    //  Chỉ cần gọi 1 lần khi khởi tạo
  void initialize_safe_matrix()
  {
    cout << "Setting up triangle safe zone ..." << endl;
    cout << "Triangle configuration: " << endl;
    cout << "   height: " << height << endl;
    cout << "   base:   " << base_length << endl;
    cout << "   frame: base_footprint" << endl;

    unsigned int num_row = floor(height/resolution);
    unsigned int num_col = ceil(base_length/resolution/2)*2;    //  even number
    Vector2d rectangle_matrix[num_row*num_col];
    Vector2d unit_vector_b(0, resolution);    //  unit vector according to base
    Vector2d unit_vector_h(resolution, 0);    //  unit vector according to height 
    double alpha = atan(2*height/base_length); 
    int count = 0;
    for(int i=0; i< num_row; i++)
    {
      float base_length_row = base_length - 2*i*(resolution/tan(alpha));                  //  length of a row of the triangle
      unsigned int num_ele_row = ceil(base_length_row/resolution/2)*2;                    //  number element on a row of the triangle
      //  consider each row in the triangle

      Vector2d center_point_row(i*resolution + vertice_A(0), 0);                          //  level up                                        //  center point of row
      Vector2d start_point_row = center_point_row - unit_vector_b*(num_ele_row/2);                //  start point of row (to the left)
      
      for(int j=0; j< num_ele_row; j++)
      {
        // cout << "   CHECK: " <<  start_point_row + j*unit_vector_b << endl;
        checkedPoint safe_triangle_ele;
        safe_triangle_ele.base_footprint_frame = start_point_row + j*unit_vector_b;
        safe_triangle.push_back(safe_triangle_ele);
        count ++;
      }
    }
    num_safe_point = count;
    cout << "There are " << num_safe_point << " safe points in the safe zone, \n\tbut cost values is not up to date!!!" << endl;
  }

  void check_safety()
  {
    //  Getting cost values for safe zone
    unsigned int obs_count = 0;  // (obstacle count) count the number of safe points whose cost value >=99
    double theta = quaternionToRPY( transformStamped_baseFootprintToOdom.transform.rotation.x,
                                    transformStamped_baseFootprintToOdom.transform.rotation.y,
                                    transformStamped_baseFootprintToOdom.transform.rotation.z,
                                    transformStamped_baseFootprintToOdom.transform.rotation.w);
    for (int i=0; i<num_safe_point; i++)
    {
      safe_triangle[i].baseFootprintFrameToOdomFrame( transformStamped_baseFootprintToOdom.transform.translation.x, 
                                                      transformStamped_baseFootprintToOdom.transform.translation.y, 
                                                      theta);
      bool a = safe_triangle[i].odomFrameToCellMap(map_info);
      if(a)
      {
        int index = int(safe_triangle[i].cell_map(0)) + safe_triangle[i].cell_map(1) * map_info.width;
        unsigned int cost = costmap.data[index];
        safe_triangle[i].cost = cost;
        if (cost >=99)  
        {
          obs_count ++;
        }
      }
      else
      {
        ROS_ERROR("Failed to update cost value of safe point at [%f %f] (base_footprint frame) !!!", safe_triangle[i].base_footprint_frame(0), safe_triangle[i].base_footprint_frame(1));
      }
    }
    if(obs_count >= obs_count_threshold)
    {
      obstacle_flag = true;
      ROS_WARN("Detected obstacle in safe zone !");
    }
    if(obs_count < obs_count_threshold)
    {
      ROS_INFO("--- Safe zone is clear ---");
    }

  }

  visualization_msgs::Marker visualizer()
  {
    visualization_msgs::Marker points;
    points.header.frame_id = "base_footprint";
    points.header.stamp = ros::Time::now();
    points.ns = "points";
    points.action = visualization_msgs::Marker::ADD;
    points.pose.orientation.w = 1.0;
    
    points.id = 0;
    points.type = visualization_msgs::Marker::POINTS;
    points.scale.x = 0.02;
    points.scale.y = 0.02;
    points.color.r = 1.0f;
    points.color.b = 1.0f;
    points.color.a = 1.0;
    for (uint32_t i = 0; i < num_safe_point; ++i)
    {
      geometry_msgs::Point p;
      p.x = safe_triangle.at(i).base_footprint_frame(0);
      p.y = safe_triangle.at(i).base_footprint_frame(1);
      p.z = 0;

      points.points.push_back(p);
    }
    return points;
  }
};
//------------------------------------    Variable declarations   ------------------------------------//
const int num_poses = 100;
nav_msgs::Path checked_path;
checkedPoint clicked_point;


//------------------------------------    Funtion declarations   ------------------------------------//
//  output theta
double quaternionToRPY(float qx, float qy, float qz, float qw)
{
    // the incoming geometry_msgs::Quaternion is transformed to a tf::Quaterion
    tf2::Quaternion quat;
    quat.setX(qx);
    quat.setY(qy);
    quat.setZ(qz);
    quat.setW(qw);

    // the tf::Quaternion has a method to acess roll pitch and yaw
    double roll, pitch, yaw;
    tf2::Matrix3x3(quat).getRPY(roll, pitch, yaw);
    return yaw;
}

void get_clicked_pose_cb(const geometry_msgs::PointStamped::ConstPtr& msg)
{
  clicked_point.map_frame(0) = msg->point.x;
  clicked_point.map_frame(1) = msg->point.y;

  double theta = quaternionToRPY( transformStamped_mapToOdom.transform.rotation.x,
                                  transformStamped_mapToOdom.transform.rotation.y,
                                  transformStamped_mapToOdom.transform.rotation.z,
                                  transformStamped_mapToOdom.transform.rotation.w);
  clicked_point.mapFrameToOdomFrame(transformStamped_mapToOdom.transform.translation.x, transformStamped_mapToOdom.transform.translation.y, theta);

  bool a = clicked_point.odomFrameToCellMap(map_info);
  if(a)
  {
    int index = int(clicked_point.cell_map(0)) + int(clicked_point.cell_map(1)) * map_info.width;
    clicked_point.cost = costmap.data[index];
    cout << "\nMAP frame [ " << clicked_point.map_frame(0) << " " << clicked_point.map_frame(1) << " ]"<< endl;
    cout << "   cell map [ " << clicked_point.cell_map(0) << " " << clicked_point.cell_map(1) << " ]"<< endl;
    cout << "   cost: " << clicked_point.cost << endl;
  }

  cout << "--------------------" << endl;
}

void get_costmap_info_cb(const nav_msgs::OccupancyGrid::ConstPtr& msg)
{
  costmap_info = *msg;
  map_info.set_info( costmap_info.info.origin.position.x,
            costmap_info.info.origin.position.y,
            costmap_info.info.resolution,
            costmap_info.info.width,
            costmap_info.info.height);
  ROS_INFO("Received map info from %s", COSTMAP_INFO_TOPIC);
  cout << "                                         map info: [" << map_info.origin_x << " " << map_info.origin_y << "] " << map_info.resolution << " " << map_info.width << "x" << map_info.height << endl;
}

void update_costmap_cb(const map_msgs::OccupancyGridUpdate::ConstPtr& msg)
{
  costmap = *msg;
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "local_costmap_checker_final");
  ros::NodeHandle node;
  ros::Subscriber sub_costmap_info = node.subscribe(COSTMAP_INFO_TOPIC, 1, get_costmap_info_cb);
  ros::Subscriber sub_costmap = node.subscribe(COSTMAP_TOPIC, 1, update_costmap_cb);
  ros::Subscriber sub_pose = node.subscribe("/clicked_point", 1, get_clicked_pose_cb);
  ros::Publisher marker_pub = node.advertise<visualization_msgs::Marker>("visualization_marker", 10);
  ros::Publisher safety_checker_pub = node.advertise<geometry_msgs::Twist>(SAFETY_CMD_VEL_TOPIC, 10);

  tf2_ros::Buffer tfBuffer_mo;
  tf2_ros::TransformListener tfListener_mo(tfBuffer_mo);

  tf2_ros::Buffer tfBuffer_bo;
  tf2_ros::TransformListener tfListener_bo(tfBuffer_bo);

  ros::Rate loopRate(20);

  ROS_INFO("LOADING costmap info from %s",COSTMAP_INFO_TOPIC);

  ROS_INFO("LOADING %s",COSTMAP_TOPIC);
  //  ----- safe zone -----
  Vector2d vertice_A_(0.5, 0.33);
  Vector2d vertice_I_(1.0, 0.0);
  Vector2d vertice_B_(0.5, -0.33);
  triangleSafeZone triangle_safe_zone(vertice_A_, vertice_I_, vertice_B_);

  //=====================================     WHILE LOOP     =====================================
  while(node.ok())
  {
    ros::spinOnce();

    //  Update TF
    try{
      transformStamped_mapToOdom = tfBuffer_mo.lookupTransform("odom", "map", ros::Time(0));
                                                // target_frame - source_frame => from source to target
      transformStamped_baseFootprintToOdom = tfBuffer_bo.lookupTransform("odom", "base_footprint", ros::Time(0));
    }
    catch (tf2::TransformException &ex) {
      ROS_WARN("%s",ex.what());
      ros::Duration(1.0).sleep();
      continue;
    }

    //-------------------------   check safety in the safe zone   -------------------------
    triangle_safe_zone.check_safety();
      //  Stop when obstacle appears
    if(triangle_safe_zone.obstacle_flag == true)
    {
      geometry_msgs::Twist stop_vel;
      stop_vel.linear.x = 0;
      stop_vel.angular.z = 0;

      safety_checker_pub.publish(stop_vel);
    }

    //-------------------------   Rviz visualization   -------------------------
    marker_pub.publish(triangle_safe_zone.visualizer());

    loopRate.sleep();
  }
  // ros::spin();
  return 0;
}
