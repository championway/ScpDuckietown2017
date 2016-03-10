#include "ros/ros.h" // main ROS include
#include "std_msgs/Float32.h" // number message datatype
#include <cmath> // needed for nan
// simple class to contain the node's variables and code
class slam_node
{
public:
slam_node(); // constructor

// class variables_
private:
	ros::NodeHandle nh_; // interface to this node
	ros::Subscriber odometry_; // interface to wheel odometry
	ros::Publisher estimatedPoses_; // interface to the trajectory topic publication
	ros::Publisher estimatedLandmarks_; // interface to the landmarks topic pub

	ros::Timer timer_; // the timer object
	double running_sum_; // the running sum since start
	double moving_average_period_; // how often to sample the moving average
	double moving_average_sum_; // sum since the last moving average update
	int moving_average_count_; // number of samples since the last update

	// callback function declarations
	void odometryMeasurementCallback(std_msgs::Float32::ConstPtr const& msg);
  void landmarkMeasurementCallback(std_msgs::Float32::ConstPtr const& msg);
	void timerCallback(ros::TimerEvent const& event);
};

// program entry point
int main(int argc, char *argv[])
{
	// initialize the ROS client API, giving the default node name
	ros::init(argc, argv, "slam_node");
	slam_node node;
	// enter the ROS main loop
	ros::spin();
	return 0;
}

// class constructor; subscribe to topics and advertise intent to publish
slam_node::slam_node() :
running_sum_(0), moving_average_period_(30), moving_average_sum_(0),
moving_average_count_(0){
		// subscribe to the number stream topic
	odometry_ = nh_.subscribe("number_stream", 1, &slam_node::landmarkMeasurementCallback, this);
	// advertise that we'll publish on the sum and moving_average topics
	estimatedPoses_ = nh_.advertise<std_msgs::Float32>("sum", 1);
	estimatedLandmarks_ = nh_.advertise<std_msgs::Float32>("moving_average", 1);
	// get moving average period from parameter server (or use default value if not present)
	ros::NodeHandle private_nh("~");
	private_nh.param("moving_average_period", moving_average_period_,	moving_average_period_);
	if (moving_average_period_ < 0.5)
	moving_average_period_ = 0.5;
	// create the Timer with period moving_average_period_
	timer_ = nh_.createTimer(ros::Duration(moving_average_period_),
	&slam_node::timerCallback, this);
	ROS_INFO("Created timer with period of %f seconds", moving_average_period_);
}

// this callback is executed every time an odometry measurement is received
void slam_node::odometryMeasurementCallback(std_msgs::Float32::ConstPtr const& msg){
  // add the data to the running sums
  running_sum_ = running_sum_ + msg->data;
  moving_average_sum_ = moving_average_sum_ + msg->data;
  // increment the moving average counter
  moving_average_count_++;
  // create a message containing the running total
  std_msgs::Float32 sum_msg;
  sum_msg.data = running_sum_;
  // publish the running sum message
  estimatedPoses_.publish(sum_msg);           
}

// the callback function for the number stream topic subscription
void slam_node::landmarkMeasurementCallback(std_msgs::Float32::ConstPtr const& msg){
	// add the data to the running sums
	running_sum_ = running_sum_ + msg->data;
	moving_average_sum_ = moving_average_sum_ + msg->data;
	// increment the moving average counter
	moving_average_count_++;
	// create a message containing the running total
	std_msgs::Float32 sum_msg;
	sum_msg.data = running_sum_;
	// publish the running sum message
	estimatedPoses_.publish(sum_msg);						
}

// the callback function for the timer event
void slam_node::timerCallback(ros::TimerEvent const& event){
	// create the message containing the moving average
	std_msgs::Float32 moving_average_msg;
	if (moving_average_count_ > 0)
	moving_average_msg.data = moving_average_sum_ / moving_average_count_;
	else
	moving_average_msg.data = nan("");
	// publish the moving average
	estimatedLandmarks_.publish(moving_average_msg);
	// reset the moving average
	moving_average_sum_ = 0;
	moving_average_count_ = 0;
}