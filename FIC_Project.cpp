#include <opencv2/core/fast_math.hpp>
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
#include <unistd.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "TCPClient.h"

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
void rotate(float angle);
void goForward();
float distance(float x1, float x2, float y1, float y2);
float angle(float x1, float x2, float y1, float y2);

bool debug = false;
int waitBetweenLoops = 3000000;

bool firstTime = true;
Vec3f playingFieldCircle;

string myColor = "red";
string myDirectionColor = "yellow";
string opponentColor = "blue";
string opponentDirectionColor = "green";

string address = "193.226.12.217";
int port = 20231;
TCPClient client;

int main(int argc, char* argv[]) {
	if (argc >= 2 && strcmp(argv[1], "/d") == 0)
		debug = true;
	if (argc == 3)
		waitBetweenLoops = atoi(argv[2]);
	client.conn(address, port);
	processVideo();
	return 0;
}

void processVideo() {
	int FRAME_WIDTH = 640;
	int FRAME_HEIGHT = 480;

	Mat cameraFeed;
	VideoCapture capture;

	//string videoPath = "rtmp://172.16.254.63/live/live";
	string videoPath =
			"/home/vevu/Desktop/Politehnica/Semestrul V/Fundamente de Inginerie a Calculatoarelor/Proiect/FICProject/recording.mp4";
	capture.open(videoPath);

	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

	while (true) {
		capture.read(cameraFeed);
		if (cameraFeed.empty()) {
			cout << "video not found\n";
			capture.open(videoPath);
		} else {
			processImage(cameraFeed);
			usleep(waitBetweenLoops);
		}
	}
}

void processImage(Mat image) {
	Mat3b hsvImage;
	cvtColor(image, hsvImage, COLOR_BGR2HSV);

	// detect the playing field
	if (firstTime) {

		Mat gray;
		cvtColor(image, gray, CV_BGR2GRAY);
		GaussianBlur(gray, gray, Size(9, 9), 2, 2);
		vector<Vec3f> circles;
		HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 2, 100, 200, 100, 100,
				500);
		if (circles.size() != 1) {
			cout << endl << "Error at detecting the playing field!" << endl;
			exit(-1);
		}

		playingFieldCircle = circles[0];
		cout << endl << "Playing field center coordinates:   "
				<< playingFieldCircle[0] << "   " << playingFieldCircle[1]
				<< endl;
		cout << "Playing field radius:   " << playingFieldCircle[2] << endl
				<< endl;
		if (debug) {
			Point center(cvRound(playingFieldCircle[0]),
					cvRound(playingFieldCircle[1]));
			int radius = cvRound(playingFieldCircle[2]);
			circle(image, center, 3, Scalar(0, 255, 0), -1, 8, 0);
			circle(image, center, radius, Scalar(0, 0, 255), 3, 8, 0);
			imshow("playing field", image);
			waitKey(0);
		}

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

Point2f getCenter(vector<Point> contour, string color) {
	Moments mu = moments(contour, false);
	Point2f center = Point2f(mu.m10 / mu.m00, mu.m01 / mu.m00);
	cout << "coordinaters of the " << color << " center:   " << center.x << "  "
			<< center.y << endl;
	return center;
}

vector<Point> getContour(Mat hsvImage, string color) {
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(getColorMask(hsvImage, color), contours, hierarchy,
			CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	int largest_area = 0;
	int largest_contour_index = 0;
	for (int i = 0; i < (int) contours.size(); i++) {
		double a = contourArea(contours[i], false);
		if (a > largest_area) {
			largest_area = a;
			largest_contour_index = i;
		}
	}

	return contours[largest_contour_index];
}

Mat1b getColorMask(Mat hsvImage, string color) {
	Mat1b mask;

	if (color.compare("red") == 0) {
		Mat1b mask1, mask2;
		inRange(hsvImage, Scalar(0, 150, 50), Scalar(10, 255, 255), mask1);
		inRange(hsvImage, Scalar(170, 150, 50), Scalar(180, 255, 255), mask2);
		mask = mask1 | mask2;
	}
	if (color.compare("yellow") == 0) {
		inRange(hsvImage, Scalar(25, 150, 50), Scalar(35, 255, 255), mask);
	}
	if (color.compare("blue") == 0) {
		inRange(hsvImage, Scalar(90, 150, 50), Scalar(130, 255, 255), mask);
	}
	if (color.compare("green") == 0) {
		inRange(hsvImage, Scalar(50, 150, 50), Scalar(72, 255, 255), mask);
	}

	if (debug) {
		imshow(color.append(" mask"), mask);
		waitKey(0);
	}
	return mask;
}

bool insidePlayingField(Point2f center) {
	bool result = false;
	float distanceToCenter = sqrt(
			pow(abs(playingFieldCircle[0] - center.x), 2)
					+ pow(abs(playingFieldCircle[1] - center.y), 2));
	if (distanceToCenter + 10 < playingFieldCircle[2])
		result = true;
	if (result)
		cout << "inside the playing field" << endl;
	else
		cout << "outside the playing filed" << endl;
	return result;
}

void sendCommand(Point2f myCenter, Point2f myDirectionCenter,
		Point2f opponentCenter, Point2f opponentDirectionCenter) {

	float myAngle = angle(myCenter.x, myDirectionCenter.x, myCenter.y,
			myDirectionCenter.y);
	cout << "my angle:   " << myAngle << endl;
	float opponentAngle = angle(opponentCenter.x, opponentDirectionCenter.x,
			opponentCenter.y, opponentDirectionCenter.y);
	cout << "opponent's angle:   " << opponentAngle << endl;
	float angleTowardsOpponent = angle(myCenter.x, opponentCenter.x, myCenter.y,
			opponentCenter.y);
	cout << "angle towards opponent:   " << angleTowardsOpponent << endl;
	float angleTowardsCenter = angle(myCenter.x, playingFieldCircle[0],
			myCenter.y, playingFieldCircle[1]);
	cout << "angle towards center:   " << angleTowardsCenter << endl;

	if (abs(myAngle) - abs(angleTowardsOpponent) < 20
			&& distance(myCenter.x, opponentDirectionCenter.x, myCenter.y,
					opponentDirectionCenter.y)
					< distance(myCenter.x, opponentCenter.x, myCenter.y,
							opponentCenter.y)) {
		cout << "turning and moving towards center" << endl;
		rotate(myAngle - angleTowardsCenter);
		goForward();
	} else {
		cout << "turning and moving towards the opponent" << endl;
		rotate(myAngle - angleTowardsOpponent);
		goForward();
	}
}

void rotate(float angle) {
	float timeToTurn360Degrees = 1000000.0;
	if (angle < 0)
		client.send_data("r");
	else
		client.send_data("l");

	int timeInMillis = floor(timeToTurn360Degrees * abs(angle) / 360.0);
	cout << "sleep for " << timeInMillis << " microseconds" << endl;
	usleep(timeInMillis);
}

void goForward() {
	client.send_data("f");
	usleep(1000000);
	client.send_data("s");
}

float distance(float x1, float x2, float y1, float y2) {
	return sqrt(pow(abs(x1 - x2), 2) + pow(abs(y1 - y2), 2));
}

float angle(float x1, float x2, float y1, float y2) {
	return atan2(y1 - y2, x1 - x2) * 180 / M_PI;
}
