//-----------------------------------��ͷ�ļ��������֡�---------------------------------------  
//      ����������������������ͷ�ļ�  
//----------------------------------------------------------------------------------------------  
#include <iostream>
#include <stdio.h>
#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/core/ocl.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/xfeatures2d.hpp>
using namespace cv;
using namespace xfeatures2d;

const int LOOP_NUM = 10;
const int GOOD_PTS_MAX = 50;
const float GOOD_PORTION = 0.15f;
//-----------------------------------��ȫ�ֺ����������֡�--------------------------------------  
//      ������ȫ�ֺ���������  
//-----------------------------------------------------------------------------------------------  
static void ShowHelpText();//�����������  
struct SURFDetector
{
	Ptr<Feature2D> surf;
	SURFDetector(double hessian = 800.0)
	{
		surf = cv::xfeatures2d::SURF::create(hessian);
	}
	template<class T>
	void operator()(const T& in, const T& mask, std::vector<cv::KeyPoint>& pts, T& descriptors, bool useProvided = false)
	{
		surf->detectAndCompute(in, mask, pts, descriptors, useProvided);
	}
};
 
template<class KPMatcher>
struct SURFMatcher
{
	KPMatcher matcher;
	template<class T>
	void match(const T& in1, const T& in2, std::vector<cv::DMatch>& matches)
	{
		matcher.match(in1, in2, matches);
	}
};

static Mat drawGoodMatches(
	const Mat& img1,
	const Mat& img2,
	const std::vector<KeyPoint>& keypoints1,
	const std::vector<KeyPoint>& keypoints2,
	std::vector<DMatch>& matches
	
	)
{
	//-- Sort matches and preserve top 10% matches
	std::sort(matches.begin(), matches.end());
	std::vector< DMatch > good_matches;
	double minDist = matches.front().distance;
	double maxDist = matches.back().distance;

	const int ptsPairs = std::min(GOOD_PTS_MAX, (int)(matches.size() * GOOD_PORTION));
	for (int i = 0; i < ptsPairs; i++)
	{
		good_matches.push_back(matches[i]);
	}
	std::cout << "\nMax distance: " << maxDist << std::endl;
	std::cout << "Min distance: " << minDist << std::endl;

	std::cout << "Calculating homography using " << ptsPairs << " point pairs." << std::endl;

	// drawing the results
	Mat img_matches;

	drawMatches(img1, keypoints1, img2, keypoints2,
		good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
		std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

	//-- Localize the object
	std::vector<Point2f> obj;
	std::vector<Point2f> scene;

	for (size_t i = 0; i < good_matches.size(); i++)
	{
		//-- Get the keypoints from the good matches
		obj.push_back(keypoints1[good_matches[i].queryIdx].pt);
		scene.push_back(keypoints2[good_matches[i].trainIdx].pt);
	}
	//-- Get the corners from the image_1 ( the object to be "detected" )
	std::vector<Point2f> obj_corners(4);
	obj_corners[0] = Point(0, 0);
	obj_corners[1] = Point(img1.cols, 0);
	obj_corners[2] = Point(img1.cols, img1.rows);
	obj_corners[3] = Point(0, img1.rows);
	std::vector<Point2f> scene_corners(4);

	Mat H = findHomography(obj, scene, RANSAC);
	perspectiveTransform(obj_corners, scene_corners, H);

	//scene_corners_ = scene_corners;

	//-- Draw lines between the corners (the mapped object in the scene - image_2 )
	line(img_matches,
		scene_corners[0] + Point2f((float)img1.cols, 0), scene_corners[1] + Point2f((float)img1.cols, 0),
		Scalar(0, 255, 0), 2, LINE_AA);
	line(img_matches,
		scene_corners[1] + Point2f((float)img1.cols, 0), scene_corners[2] + Point2f((float)img1.cols, 0),
		Scalar(0, 255, 0), 2, LINE_AA);
	line(img_matches,
		scene_corners[2] + Point2f((float)img1.cols, 0), scene_corners[3] + Point2f((float)img1.cols, 0),
		Scalar(0, 255, 0), 2, LINE_AA);
	line(img_matches,
		scene_corners[3] + Point2f((float)img1.cols, 0), scene_corners[0] + Point2f((float)img1.cols, 0),
		Scalar(0, 255, 0), 2, LINE_AA);
	return img_matches;
}

//-----------------------------------��main( )������--------------------------------------------  
//      ����������̨Ӧ�ó������ں��������ǵĳ�������￪ʼִ��  
//----------------------------------------------------------------------------------------------- 
/*
int main()
{
	//��0���ı�console������ɫ  
	system("color 1A");

	//��0����ʾ��ӭ�Ͱ�������  
	ShowHelpText();

	//��1�������ز�ͼ  
	Mat srcImage1 = imread("p1.png", 1);
	Mat srcImage2 = imread("p2.png", 1);
	if (!srcImage1.data || !srcImage2.data)
	{
		printf("��ȡͼƬ������ȷ��Ŀ¼���Ƿ���imread����ָ����ͼƬ����~�� \n"); return false;
	}
	std::vector<KeyPoint> keypoints1, keypoints2;
	std::vector<DMatch> matches;
	Mat descriptors1,descriptors2;
	SURFDetector surf;
	SURFMatcher<BFMatcher> matcher;

	//��3������detect��������SURF�����ؼ��㣬������vector������  
	//��4������������������������  
	surf(srcImage1, Mat(), keypoints1, descriptors1);
	surf(srcImage2, Mat(), keypoints2, descriptors2);
	//��5��ƥ������ͼ�е������ӣ�descriptors��  
	matcher.match(descriptors1, descriptors2, matches);

	//��6�����ƴ�����ͼ����ƥ����Ĺؼ���  
	Mat imgMatches;
	imgMatches = drawGoodMatches(srcImage1, srcImage2, keypoints1, keypoints2, matches);
	//drawMatches(srcImage1, keypoints1, srcImage2, keypoints2, matches, imgMatches);//���л���  
	//��7����ʾЧ��ͼ  
	imshow("ƥ��ͼ", imgMatches);

	waitKey(0);
	return 0;
}
*/
//-----------------------------------��ShowHelpText( )������----------------------------------    
//      ���������һЩ������Ϣ    
//----------------------------------------------------------------------------------------------    
static void ShowHelpText()
{
	//���һЩ������Ϣ    
	printf("\n\n\n\tSUPF\n\n");
	
}