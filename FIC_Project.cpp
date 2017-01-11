#include "TCPClient.h"

#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace cv;

void processVideo();
void processImage(Mat image);
Point2f getCenter(vector<Point> contour, string color);
vector<Point> getContour(Mat hsvImage, string color);
Mat1b getColorMask(Mat hsvImage, string color);
bool insidePlayingField(Point2f center);
void sendCommand(Point2f myCenter, Point2f myDirectionCenter,
		Point2f opponentCenter, Point2f opponentDirectionCenter);

bool debug = true;
bool firstTime = true;
vector<Point> playingFieldContour;

string myColor = "red";
string myDirectionColor = "yellow";
string opponentColor = "blue";
string opponentDirectionColor = "green";

string address = "193.226.12.217";
int port = 20231;
TCPClient client;

int main(int argc, char* argv[])
{
	client.conn(address, port);
	processVideo();
	return 0;
}

void processVideo()
{
	int FRAME_WIDTH = 640;
	int FRAME_HEIGHT = 480;

	Mat cameraFeed;
	VideoCapture capture;

	//string videoPath = "rtmp://172.16.254.63/live/live";
	string videoPath = "/home/vevu/Desktop/cmake_version/recording.mp4";
	capture.open(videoPath);

	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

	while (true)
	{
		capture.read(cameraFeed);
		if (cameraFeed.empty())
		{
			cout << "here\n";
			capture.open(videoPath);
		}
		else
			processImage(cameraFeed);
	}
}

void processImage(Mat image)
{
	Mat3b hsvImage;
	cvtColor(image, hsvImage, COLOR_BGR2HSV);

	if (firstTime)
	{
		playingFieldContour = getContour(hsvImage, "all except white");

		firstTime = false;
	}

	Point2f myCenter = getCenter(getContour(hsvImage, myColor), myColor);
	insidePlayingField(myCenter);
	Point2f myDirectionCenter = getCenter(
			getContour(hsvImage, myDirectionColor), myDirectionColor);

	Point2f opponentCenter = getCenter(getContour(hsvImage, opponentColor),
			opponentColor);
	insidePlayingField(opponentCenter);
	Point2f opponentDirectionCenter = getCenter(
			getContour(hsvImage, opponentDirectionColor),
			opponentDirectionColor);

	sendCommand(myCenter, myDirectionCenter, opponentCenter,
			opponentDirectionCenter);
}

Point2f getCenter(vector<Point> contour, string color)
{
	Moments mu = moments(contour, false);
	Point2f center = Point2f(mu.m10 / mu.m00, mu.m01 / mu.m00);
	cout << "coordinaters of the " << color << " center:   " << center.x << "  "
			<< center.y << endl;
	return center;
}

vector<Point> getContour(Mat hsvImage, string color)
{
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(getColorMask(hsvImage, color), contours, hierarchy,
			CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	int largest_area = 0;
	int largest_contour_index = 0;
	for (int i = 0; i < (int) contours.size(); i++)
	{
		double a = contourArea(contours[i], false);
		if (a > largest_area)
		{
			largest_area = a;
			largest_contour_index = i;
		}
	}

	return contours[largest_contour_index];
}

Mat1b getColorMask(Mat hsvImage, string color)
{
	Mat1b mask;

	if (color.compare("all except white") == 0)
	{
		inRange(hsvImage, Scalar(0, 15, 0), Scalar(225, 255, 240), mask);
	}
	if (color.compare("red") == 0)
	{
		Mat1b mask1, mask2;
		inRange(hsvImage, Scalar(0, 150, 50), Scalar(10, 255, 255), mask1);
		inRange(hsvImage, Scalar(170, 150, 50), Scalar(180, 255, 255), mask2);
		mask = mask1 | mask2;
	}
	if (color.compare("yellow") == 0)
	{
		inRange(hsvImage, Scalar(25, 150, 50), Scalar(35, 255, 255), mask);
	}
	if (color.compare("blue") == 0)
	{
		inRange(hsvImage, Scalar(90, 150, 50), Scalar(130, 255, 255), mask);
	}
	if (color.compare("green") == 0)
	{
		inRange(hsvImage, Scalar(50, 150, 50), Scalar(72, 255, 255), mask);
	}

	if (debug)
	{
		imshow(color.append(" mask"), mask);
		waitKey(0);
	}
	return mask;
}

bool insidePlayingField(Point2f center)
{
	bool result = pointPolygonTest(playingFieldContour, center, true) > 0;
	if (result)
		cout << "inside the playing field" << endl;
	else
		cout << "outside the playing filed" << endl;
	return result;
}

void sendCommand(Point2f myCenter, Point2f myDirectionCenter,
		Point2f opponentCenter, Point2f opponentDirectionCenter)
{
	client.send_data("r");
}
