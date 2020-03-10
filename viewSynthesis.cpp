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

// Get the intersection of the view plane and the ray from the new viewpoint
Point2d getIntersection(Point3d viewpoint, Point2d imagePlane, double focalLength) {
    double t = viewpoint.z / focalLength;
    return {viewpoint.x + t * imagePlane.x, viewpoint.y + t * imagePlane.y};
}

// Convert from pixel index to image coordinate
inline Point2d pixelTo2d(int x, int y) { return {x * 34 / 511.0 - 17, y * 34 / 511.0 - 17}; }

// Convert the coordinate of view plane to index of viewImageList
// Subtract from 72 because (0, 0) is at bottom left corner
inline int gridToIndex(int s, int t) { return 72 - t * 9 + s; }

// Bilinear interpolation
void interpolate(Color &pixel, int x, int y, double alpha, double beta, Bitmap topLeft, Bitmap topRight, Bitmap botLeft,
                 Bitmap botRight) {
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

    pixel.R = p[0];
    pixel.G = p[1];
    pixel.B = p[2];
}

void getNeighbourRays(Point2d viewPlane, double &alpha, double &beta, int &topLeft, int &topRight, int &botLeft,
                      int &botRight) {
    double x = viewPlane.x, y = viewPlane.y;
    x = (x + 120) / Baseline;
    y = (y + 120) / Baseline;
    int left = floor(x), right = ceil(x), top = ceil(y), bottom = floor(y);

    topLeft = gridToIndex(left, top);
    topRight = gridToIndex(right, top);
    botLeft = gridToIndex(left, bottom);
    botRight = gridToIndex(right, bottom);
    alpha = modf(x, &x); // Extract the decimal part
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

    Point3d viewpoint(Vx, Vy, Vz);
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
		    Color pixel{};
		    Point2d imagePlane = pixelTo2d(c, r);
            Point2d intersection = getIntersection(viewpoint, imagePlane, targetFocalLen);

            if (intersection.x > 120 || intersection.x < -120 || intersection.y > 120 || intersection.y < -120) {
                pixel.R = pixel.G = pixel.B = 0;
            } else {
                double alpha, beta;
                int topLeft, topRight, botLeft, botRight;
                getNeighbourRays(intersection, alpha, beta, topLeft, topRight, botLeft, botRight);
                interpolate(pixel, c, r, alpha, beta, viewImageList[topLeft], viewImageList[topRight], viewImageList[botLeft],
                            viewImageList[botRight]);
            }
			//! record the resampled pixel value
            targetView.setColor(c, r, pixel.R, pixel.G, pixel.B);
		}
	}
	string savePath = "newView.bmp";
	targetView.save(savePath.c_str());
	cout << "Result saved!" << endl;
	return 0;
}