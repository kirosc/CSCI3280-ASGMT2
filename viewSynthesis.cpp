#include "bmp.h"		//	Simple .bmp library
#include<iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>

using namespace std;

#define Baseline 30.0
#define Focal_Length 100
#define Image_Width 35.0
#define Image_Height 35.0
#define Resolution_Row 512
#define Resolution_Col 512
#define View_Grid_Row 9
#define View_Grid_Col 9

struct Point3d
{
	double x;
	double y;
	double z;
	Point3d(double x_, double y_, double z_) :x(x_), y(y_), z(z_) {}
};

struct Point2d
{
	double x;
	double y;
	Point2d(double x_, double y_) :x(x_), y(y_) {}
};

void interpolate(Point3d &point, int x, int y, double alpha, double beta, Bitmap topLeft, Bitmap topRight, Bitmap botLeft, Bitmap botRight) {
    unsigned char p1[3], p2[3], p[3], topLeftRGB[3], topRightRGB[3], botLeftRGB[3], botRightRGB[3];
    topLeft.getColor(x, y, topLeftRGB[0], topLeftRGB[1], topLeftRGB[2]);
    topRight.getColor(x, y, topRightRGB[0], topRightRGB[1], topRightRGB[2]);
    botLeft.getColor(x, y, botLeftRGB[0], botLeftRGB[1], botLeftRGB[2]);
    botRight.getColor(x, y, botRightRGB[0], botRightRGB[1], botRightRGB[2]);

    for (int i = 0; i < 3; i++) {
        p1[i] = (1.0 - alpha) * topLeftRGB[i] + alpha * topRightRGB[i];
        p2[i] = (1.0 - alpha) * botLeftRGB[i] + alpha * botRightRGB[i];
        p[i] = (1.0 - beta) * p1[i] + beta * p2[i];
    }

    point.x = p[0];
    point.y = p[1];
    point.z = p[2];
}

// Subtract from 72 because (0, 0) is at bottom left corner
inline int gridToIndex(int s, int t) { return 72 - t * 9 + s; }

void getNeighbourRays(double x, double y, double &alpha, double &beta, int &topLeft, int &topRight, int &botLeft,
                      int &botRight) {
    x = (x + 120) / 30.0;
    y = (y + 120) / 30.0;
    int left = floor(x), right = ceil(x), top = ceil(y), bottom = floor(y);

    topLeft = gridToIndex(left, top);
    topRight = gridToIndex(right, top);
    botLeft = gridToIndex(left, bottom);
    botRight = gridToIndex(right, bottom);
    alpha = modf(x, &x);
    beta = modf(y, &y);
}


int main(int argc, char** argv)
{
	if (argc != 6)
	{
		cout << "Arguments prompt: viewSynthesis.exe <LF_dir> <X Y Z> <focal_length>" << endl;
		return 0;
	}
	string LFDir = argv[1];
	double Vx = stod(argv[2]), Vy = stod(argv[3]), Vz = stod(argv[4]);
	double targetFocalLen = stod(argv[5]);

	if (-120.0 > Vx || Vx > 120.0 || -120.0 > Vy || Vy > 120.0) {
	    cout << "X and Y should range between -120 and 120" << endl;
	    return 0;
	}

	double alpha, beta;
    int topLeft, topRight, botLeft, botRight;
    getNeighbourRays(Vx, Vy, alpha, beta, topLeft, topRight, botLeft, botRight);

	vector<Bitmap> viewImageList;
	//! loading light field views
	for (int i = 0; i < View_Grid_Col*View_Grid_Row; i++)
	{
		char name[128];
		sprintf(name, "/cam%03d.bmp", i+1);
		string filePath = LFDir + name;
		Bitmap view_i(filePath.c_str());
		viewImageList.push_back(view_i);
	}

	Bitmap targetView(Resolution_Col, Resolution_Row);
	cout << "Synthesizing image from viewpoint: (" << Vx << "," << Vy << "," << Vz << ") with focal length: " << targetFocalLen << endl;
	//! resample pixels of the target view one by one
	for (int r = 0; r < Resolution_Row; r++)
	{
		for (int c = 0; c < Resolution_Col; c++)
		{
			Point3d rayRGB(0, 0, 0);
			//! resample the pixel value of this ray: TODO
            interpolate(rayRGB, c, r, alpha, beta, viewImageList[topLeft], viewImageList[topRight], viewImageList[botLeft],
                        viewImageList[botRight]);

			//! record the resampled pixel value
            targetView.setColor(c, r, (unsigned char) rayRGB.x, (unsigned char) rayRGB.y, (unsigned char) rayRGB.z);
//			targetView.setColor(c, r, unsigned char(rayRGB.x), unsigned char(rayRGB.y), unsigned char(rayRGB.z));
		}
	}
	string savePath = "newView.bmp";
	targetView.save(savePath.c_str());
	cout << "Result saved!" << endl;
	return 0;
}