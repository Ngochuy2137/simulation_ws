#! /bin/python3

import rospy
from obstacle_detector import ObstacleDetector
from actionlib_msgs.msg import GoalID
from geometry_msgs.msg import PoseStamped
from geometry_msgs.msg import Twist
from lino_msgs.msg import Velocities
from crbot_kinematics.msg import speed_wheel

class Stop():

    def __init__(self):
        self.od = ObstacleDetector(max_obstacle_dist=1.5, scan_topic='scan')
        self.pub_stop = rospy.Publisher("move_base/cancel", GoalID, queue_size=1)
        self.pub_cmd_vel = rospy.Publisher("cmd_vel/safety", Twist, queue_size=1)
        self.pub_goal = rospy.Publisher("move_base_simple/goal", PoseStamped, queue_size=1)
        self.sub_goal = rospy.Subscriber("move_base_simple/goal", PoseStamped, self.get_goal)
        self.raw_sub = rospy.Subscriber("raw_vel", Velocities, self.update_raw_vel)

        self.have_object = False
        self.repub_goals = False

        self.position_x = None
        self.position_y = None
        self.position_z = None
        self.orientation_x = None
        self.orientation_y = None
        self.orientation_z = None
        self.orientation_w = None

        self.position_x_buffer = None
        self.position_y_buffer = None
        self.position_z_buffer = None
        self.orientation_x_buffer = None
        self.orientation_y_buffer = None
        self.orientation_z_buffer = None
        self.orientation_w_buffer = None

        self.linear_vel_x = 0

    def cancel_goal(self):
        if self.od.has_obstacle == True and self.repub_goals == False and self.position_x != None and self.linear_vel_x > 0.1:
            self.position_x_buffer = self.position_x
            self.position_y_buffer = self.position_y
            self.position_z_buffer = self.position_z
            self.orientation_x_buffer = self.orientation_x
            self.orientation_y_buffer = self.orientation_y
            self.orientation_z_buffer = self.orientation_z
            self.orientation_w_buffer = self.orientation_w
            self.pub_stop.publish()
            self.set_stop()
            rospy.loginfo("Cancel goal")
            self.repub_goals = True 

        if self.repub_goals == True:
            i = 0
            for _ in range(30):
                if self.od.has_obstacle == False:
                    i += 1
                rospy.sleep(0.05)
                # print(self.od.has_obstacle)
            if i == 30:
                goal = PoseStamped()
                goal.header.frame_id = "map"
                goal.header.stamp = rospy.Time.now()
                goal.pose.position.x = self.position_x_buffer
                goal.pose.position.y = self.position_y_buffer
                goal.pose.position.z = self.position_z_buffer
                goal.pose.orientation.x = self.orientation_x_buffer
                goal.pose.orientation.y = self.orientation_y_buffer
                goal.pose.orientation.z = self.orientation_z_buffer
                goal.pose.orientation.w = self.orientation_w_buffer
                self.pub_goal.publish(goal)
                rospy.loginfo("Publish goal")
                self.repub_goals = False
    
    def set_stop(self):
        acceleration = 0.8
        current_time = rospy.get_time()
        last_time = rospy.get_time()
        while True :
            current_time = rospy.get_time()
            dt = current_time - last_time
            last_time = current_time
            vel_x = self.linear_vel_x - acceleration * dt
            if vel_x > 0:
                self.set_cmd_vel(vel_x, 0)
            if vel_x < 0:
                self.set_cmd_vel(0, 0)
                break
            rospy.sleep(0.02)

    def update_raw_vel(self,msg):
        self.linear_vel_x = msg.linear_x

    def set_cmd_vel(self, linear_x, angular_z):
        twist = Twist()
        twist.linear.x = linear_x
        twist.angular.z = angular_z
        self.pub_cmd_vel.publish(twist)

    def get_goal(self, data):
        self.position_x = data.pose.position.x
        self.position_y = data.pose.position.y
        self.position_z = data.pose.position.z
        self.orientation_x = data.pose.orientation.x
        self.orientation_y = data.pose.orientation.y
        self.orientation_z = data.pose.orientation.z
        self.orientation_w = data.pose.orientation.w

def main():
    rospy.init_node('Stop')
    stop = Stop()
    od = ObstacleDetector(max_obstacle_dist=1.5, scan_topic='scan')
    rate = rospy.Rate(5)
    while not rospy.is_shutdown():
        stop.cancel_goal()
        rate.sleep()
    rospy.spin()


if __name__ == '__main__':
    main()
