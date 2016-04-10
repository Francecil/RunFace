#include<iostream>  
#include<string>
#include <opencv2/core/core.hpp>  
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>  
#include <algorithm>
#include <vector>
#include <list>
using namespace std;
using namespace cv;
#define PI 3.1415926
#define eps 1e-8
Mat srcImage, smallImage, meanshiftImage, fillImage, maskImage, grabCutImage,dstImage;
Mat grayImage;//����ԭʼͼ��Ŀ��ͼ���Ҷ�ͼ����ģͼ  
int spatialRad = 10, colorRad = 10, maxPryLevel = 1;
int MAX_JUDGE_WHITE = 600;
int MAX_WIDTH = 300, MAX_HEIGHT = 400;
double UNCAL_THETA=0.5;
double nearDes = 40.0;
string photo_name = "three_2.jpg";
string desPhoto = "p2.png";
struct MyPoint{
	double x, y;
	bool isFind = false;
	int index = 0;
	int sumP=0;
};
typedef struct Line{
	MyPoint p1, p2;
	double theta;
	double rho;
	
}Line;
int g_nFillMode = 1;//��ˮ����ģʽ  
int g_nLowDifference = 35, g_nUpDifference = 35;//�������ֵ���������ֵ  
int g_nConnectivity = 4;//��ʾfloodFill������ʶ���Ͱ�λ����ֵͨ  
int g_bIsColor = true;//�Ƿ�Ϊ��ɫͼ�ı�ʶ������ֵ  
bool g_bUseMask = true;//�Ƿ���ʾ��Ĥ���ڵĲ���ֵ  
int g_nNewMaskVal = 255;//�µ����»��Ƶ�����ֵ  
void Grabcut();
vector<MyPoint> points;
void addPoint(Line l1, Line l2){
	//�ǶȲ�̫С ���㣬
	double minTheta = (l1.theta < l2.theta ? l1.theta : l2.theta);
	double maxTheta = (l1.theta > l2.theta ? l1.theta : l2.theta);
	if (fabs(l1.theta - l2.theta) < UNCAL_THETA || fabs(minTheta + PI - maxTheta) < UNCAL_THETA)
	{
		cout << "�Ƕ�̫С" << endl;
		return;
	}
	//��������ֱ�ߵĽ���
	MyPoint CrossP;
	//y = a * x + b;
	double a1 = fabs(l1.p1.x - l1.p2.x)<eps?0:(l1.p1.y - l1.p2.y) / (l1.p1.x - l1.p2.x);
	double b1 = l1.p1.y - a1 * (l1.p1.x);
	double a2 = fabs((l2.p1.x - l2.p2.x))<eps?0:(l2.p1.y - l2.p2.y) / (l2.p1.x - l2.p2.x);
	double b2 = l2.p1.y - a2 * (l2.p1.x);
	if (fabs(a2 - a1) > eps){
	CrossP.x = (b1 - b2) / (a2 - a1);
	CrossP.y = a1 * CrossP.x + b1;
	cout << CrossP.x << "  " << CrossP.y << endl;
	points.push_back(CrossP);
	}
}
bool isNear(MyPoint p1, MyPoint p2){
	if (fabs(p1.x - p2.x) + fabs(p1.y - p2.y) < nearDes)return true;
	return false;
}
bool compareMyPoint(MyPoint& a, MyPoint& b)
{	
	int d, f;
	d = a.isFind == true ? 1 : 0;
	f = b.isFind == true ? 1 : 0;
	if (d == f)return a.sumP<b.sumP;
	else return d < f;
}
void getLine(){
	//��2�����б�Ե����ת��Ϊ�Ҷ�ͼ  
	Mat midImage, dstImage;//��ʱ������Ŀ��ͼ�Ķ���  
	Canny(maskImage, midImage, 25, 60, 3);//����һ��canny��Ե���  
	cvtColor(midImage, dstImage, CV_GRAY2BGR);//ת����Ե�����ͼΪ�Ҷ�ͼ  
	//��3�����л����߱任  
	vector<Vec2d> lines;//����һ��ʸ���ṹlines���ڴ�ŵõ����߶�ʸ������ 
	HoughLines(midImage, lines, 1, CV_PI / 180, 50, 0, 0);

	//��4��������ͼ�л��Ƴ�ÿ���߶�  
	cout << lines.size() << endl;
	//�õ��ܶ����(rhp,theta)������Щ����Χ��Ψһһ���ı���
	//rho��������ԭ��((0, 0)��Ҳ����ͼ������Ͻǣ��ľ��롣 theta�ǻ���������ת�Ƕȣ�0~��ֱ�ߣ��� / 2~ˮƽ�ߣ���
	vector<Line> mylines;
	for (size_t i = 0; i < lines.size(); i++)
	{
		cout << " rho,theta " << lines[i][0] << "   " << lines[i][1] << "   ";
		double rho = lines[i][0], theta = lines[i][1];
		Point pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a*rho, y0 = b*rho;
		pt1.x = cvRound(x0 + 400 * (-b)); //�õ�����(x0,y0)�����������չ����400 ���߶γ���Ϊ800
		pt1.y = cvRound(y0 + 400 * (a));  //��������������չ ��һ��һ��
		pt2.x = cvRound(x0 - 400 * (-b));
		pt2.y = cvRound(y0 - 400 * (a));
		MyPoint mp1, mp2;
		mp1.x = pt1.x;
		mp1.y = pt1.y;
		mp2.x = pt2.x;
		mp2.y = pt2.y;
		Line myline;
		myline.p1 = mp1; myline.p2 = mp2;
		myline.theta = theta;
		myline.rho = rho;
		mylines.push_back(myline);
		cout << "p1(x,y):" << pt1.x << "," << pt1.y << " p2(x,y):" << pt2.x << "," << pt2.y << endl;
		line(dstImage, pt1, pt2, Scalar(((256 / lines.size())*i) % 255, (256 / lines.size())*i % 255, (256 / lines.size())*i % 255), 1, CV_AA);

		/*
		Vec4i l = lines[i];
		line(dstImage, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(186, 88, 255), 1, CV_AA);
		*/
	}
	//��6����Ե�����ͼ   
	imshow("����Ե�����ͼ��", midImage);

	//��7����ʾЧ��ͼ    
	imshow("��Ч��ͼ��", dstImage);
	//��mylines�Ľ���
	points.clear();
	for (size_t i = 0; i < mylines.size(); i++){
		for (size_t j = i+1; j < mylines.size(); j++){
			
			addPoint(mylines[i], mylines[j]);
		}
	}
	cout << "����һ��" << points.size() << "����" << endl;
	
	int p_index = 0;
	//����һ����ά����
	vector<int> fourPoint[10];
	for (size_t i = 0; i < points.size(); i++){
		if (points[i].isFind == false){ 
			points[i].index = p_index++; 
			if (p_index > 10)break;//warn
			fourPoint[points[i].index].push_back(i);
			points[i].isFind = true;
		}
		else continue;
		for (size_t j = i+1; j < points.size(); j++){
			if (points[j].isFind == true)continue;
			if (isNear(points[i], points[j])){
				points[j].index = points[i].index;
				points[j].isFind = true;
				fourPoint[points[j].index].push_back(j);
			}
		}
	}
	//����Ŀ����ֻ��4����
	vector<MyPoint> fp;
	for (size_t i = 0; i < 10; i++){
		cout << "��" << i << "��";
		if (fourPoint[i].size() == 0){ 

			cout <<"û��Ԫ��" << endl; continue; }
		cout << endl;
		double sumx = 0,sumy=0;
		/*
		224.538 400
-67.2427 200.173
271.741 162.73
211.596 391.083
		*/
		for (size_t j = 0; j < fourPoint[i].size(); j++){
			sumx += points[fourPoint[i][j]].x;
			sumy += points[fourPoint[i][j]].y;
			cout << "x,y:" << points[fourPoint[i][j]].x << "  " << points[fourPoint[i][j]].y << endl;
		}
		MyPoint pp;
		pp.x = sumx / fourPoint[i].size();
		pp.y = sumy / fourPoint[i].size();
		pp.sumP = fourPoint[i].size();
		fp.push_back(pp);
	}
	int findNear = 0,small=0;
	bool haveNear = true;
	while (fp.size() - findNear > 4&&haveNear){
		cout << "fp.size():" << fp.size() << endl;
		//�ȿ���û�������
		
		haveNear = false;
		for (int i = 0; i < fp.size(); i++){
			for (int j = i + 1; j < fp.size(); j++){
				if (fp[j].isFind == true)continue;
				if (isNear(fp[i], fp[j])){
					fp[i].x = (fp[i].x*fp[i].sumP + fp[j].x*fp[j].sumP) / (fp[i].sumP+fp[j].sumP);
					fp[i].y = (fp[i].y*fp[i].sumP + fp[j].y*fp[j].sumP) / (fp[i].sumP+fp[j].sumP);
					fp[i].sumP += fp[j].sumP;
					haveNear = true;
					findNear += 1;
					fp[j].isFind = true;
					cout << "�ҵ�һ��" << endl;
					
				}
			}
		}	
	}
	if (fp.size() - findNear > 4){
		cout << "ȥ�����" << endl;
		sort(fp.begin(), fp.end(), compareMyPoint);
		int index_s = 0;
		while (fp.size() - findNear - small>4){
			fp[index_s++].isFind = true;
			small++;
		}
	}
	if (fp.size() - findNear -small==4){
		vector<Point2f> corners_trans(4);
		int trans_index = 0;
		for (int i = 0; i < fp.size(); i++){
			if (fp[i].isFind == false){
				cout << "�ĵ�֮" << i << "x,y:" << fp[i].x << " " << fp[i].y << endl;
				corners_trans[trans_index++] = Point2f(fp[i].x, fp[i].y);
			}
		}
		Mat dstPhotoImage = imread(desPhoto, 1);
		imshow("����ͼ", dstPhotoImage);
		int img_height = dstPhotoImage.rows;
		int img_width = dstPhotoImage.cols;
		vector<Point2f> corners(4);
		corners[0] = Point2f(0, 0);
		corners[1] = Point2f(img_width - 1, 0);
		corners[2] = Point2f(0, img_height - 1);
		corners[3] = Point2f(img_width - 1, img_height - 1);
		Mat transform = getPerspectiveTransform(corners, corners_trans);//����4����ά���֮���͸��任���� H��3��x3�У�
		Mat finalImage;
		warpPerspective(dstPhotoImage, finalImage, transform, Size(meanshiftImage.cols, meanshiftImage.rows));
		imshow("finalImage0:", finalImage);
		Mat mixImage0 = finalImage, mixImage1;
		//addWeighted(meanshiftImage, 0.5, finalImage, 0.3, 0., mixImage);
		cvtColor(smallImage, mixImage1, CV_RGBA2RGB);//ͨ���Ȼ���
		//������渳ֵ�ᱨ Assertion failed (dims <= 2 && data && (unsigned)i0 < (unsigned)size.p[0] && (unsigned)(i1 * DataType
		imshow("finalImage1:", mixImage0);
		
		for (int i = 0; i < mixImage0.rows; i++){
			for (int j = 0; j < mixImage0.cols; j++){
				Vec3b &rgba = mixImage0.at<Vec3b>(i, j);
				Vec3b &rgbb = smallImage.at<Vec3b>(i, j);
				if (rgba[0]>0 && rgba[2]>0 && rgba[2]>0 ){
					rgbb[0] = rgba[0];
					rgbb[1] = rgba[1];
					rgbb[2] = rgba[2];
				}
				
			}
		}
		imshow("finalImage1", smallImage);
	}
	else{
		cout << "��������3����" << endl;
	}
	


}
//-----------------------------------��onMouse( )������--------------------------------------    
//      �����������ϢonMouse�ص�����  
//---------------------------------------------------------------------------------------------  
static void onMouse(int event, int x, int y, int, void*)
{
	// ��������û�а��£��㷵��  
	if (event != CV_EVENT_LBUTTONDOWN)
		return;
	cout << "���λ�ã�" << x << "  " << y << endl;
	//-------------------��<1>����floodFill����֮ǰ�Ĳ���׼�����֡�---------------  
	Point seed = Point(x, y);
	int LowDifference = g_nFillMode == 0 ? 0 : g_nLowDifference;//�շ�Χ����ˮ��䣬��ֵ��Ϊ0��������Ϊȫ�ֵ�g_nLowDifference  
	int UpDifference = g_nFillMode == 0 ? 0 : g_nUpDifference;//�շ�Χ����ˮ��䣬��ֵ��Ϊ0��������Ϊȫ�ֵ�g_nUpDifference  
	int flags = g_nConnectivity + (g_nNewMaskVal << 8) +
		(g_nFillMode == 1 ? CV_FLOODFILL_FIXED_RANGE : 0);//��ʶ����0~7λΪg_nConnectivity��8~15λΪg_nNewMaskVal����8λ��ֵ��16~23λΪCV_FLOODFILL_FIXED_RANGE����0��  

	//�������bgrֵ  
	int b = (unsigned)theRNG() & 255;
	int g = (unsigned)theRNG() & 255;
	int r = (unsigned)theRNG() & 255;
	Rect ccomp;//�����ػ��������С�߽��������  

	Scalar newVal = g_bIsColor ? Scalar(b, g, r) : Scalar(r*0.299 + g*0.587 + b*0.114);//���ػ��������ص���ֵ�����ǲ�ɫͼģʽ��ȡScalar(b, g, r)�����ǻҶ�ͼģʽ��ȡScalar(r*0.299 + g*0.587 + b*0.114)  

	Mat dst = dstImage;//Ŀ��ͼ�ĸ�ֵ  
	int area;
	maskImage = Scalar::all(0);//�ǵü���䡣��
	//--------------------��<2>��ʽ����floodFill������-----------------------------  
	threshold(maskImage, maskImage, 1, 128, THRESH_BINARY);
	area = floodFill(dst, maskImage,seed, newVal, &ccomp, Scalar(LowDifference, LowDifference, LowDifference),
		Scalar(UpDifference, UpDifference, UpDifference), flags);
	imshow("mask", maskImage);
	
	cout << area << " �����ر��ػ�\n";
	//�õ����mask ����������һ���ķָ�����÷ָ�ֱ�����߶μ��
	//Grabcut();
	getLine();
}
void Grabcut(){
	cv::Mat result;
	
	resize(maskImage, result, Size(meanshiftImage.cols, meanshiftImage.rows));
	result = cv::Scalar(cv::GC_FGD);
	//ע����Ӿ���ֵ�ķ���
	Mat bgModel, fgModel;
	// GrabCut �ֶ�
	cv::grabCut(meanshiftImage,    //����ͼ��
		result,   //�ֶν��
		Rect(),// ����ǰ���ľ��� 
		bgModel, fgModel, // ǰ��������
		1,        // ��������
		cv::GC_INIT_WITH_MASK); // �þ���

	// �õ�������ǰ��������
	// �������ͼ��
	Mat foreground(meanshiftImage.size(), CV_8UC3, cv::Scalar(255, 255, 255));
	//����ֵΪ GC_BGD=0����Ϊ����
	meanshiftImage.copyTo(foreground, result);
	imshow("mask", foreground);
}
void test0(){
	srcImage = imread(photo_name, 1);
	if (!srcImage.data) { printf("��ȡsrcImage����~��\n"); return ; }
	//imshow("��<0>ԭͼ���ڡ�", srcImage);
	//1.����ȡ������  
	smallImage = srcImage;
	resize(srcImage, smallImage, Size(300, 400));
	//imshow("��<1>��С���ڡ�", smallImage);
	/*
	*Ч������
	//2.��ֵ�˲�
	medianBlur(dstImage, zz, 7);
	imshow("��<2>��ֵ�˲���", zz);
	*/
	/*
	2.����meanshiftԤ����
	*/
	pyrMeanShiftFiltering(smallImage, meanshiftImage ,spatialRad, colorRad, maxPryLevel);
	//imshow("��<2>meanshift�˲���", meanshiftImage);

	meanshiftImage.copyTo(dstImage);//����Դͼ��Ŀ��ͼ  
	
	maskImage.create(meanshiftImage.rows + 2, meanshiftImage.cols + 2, CV_8UC1);//����image0�ĳߴ�����ʼ����Ĥmask  
	maskImage = Scalar::all(0);//�ǵü���䡣��
	imshow("mask", maskImage);
	
	namedWindow("Ч��ͼ", CV_WINDOW_AUTOSIZE);

	//����Trackbar  
	createTrackbar("�������ֵ", "Ч��ͼ", &g_nLowDifference, 255, 0);
	createTrackbar("�������ֵ", "Ч��ͼ", &g_nUpDifference, 255, 0);
	
	//���ص�����  
	setMouseCallback("Ч��ͼ", onMouse, 0);
	//ѭ����ѯ����  
	while (1)
	{
		//����ʾЧ��ͼ  
		imshow("Ч��ͼ",  dstImage);

		//��ȡ���̰���  
		int c = waitKey(0);
		//�ж�ESC�Ƿ��£������±��˳�  
		if ((c & 255) == 27)
		{
			cout << "�����˳�...........\n";
			break;
		}

		
	}
	return ;

}
//kmeans
void test(){
	const int MAX_CLUSTERS = 5;
	Scalar colorTab[] =
	{
		Scalar(0, 0, 255),
		Scalar(0, 255, 0),
		Scalar(255, 100, 100),
		Scalar(255, 0, 255),
		Scalar(0, 255, 255)
	};
	Mat img(500, 500, CV_8UC3);
	RNG rng(12345);
	for (;;)
	{
		int k, clusterCount = rng.uniform(2, MAX_CLUSTERS + 1);
		int i, sampleCount = rng.uniform(1, 1001);
		Mat points(sampleCount, 1, CV_32FC2), labels;
		clusterCount = MIN(clusterCount, sampleCount);
		Mat centers;
		/* generate random sample from multigaussian distribution */
		for (k = 0; k < clusterCount; k++)
		{
			Point center;
			center.x = rng.uniform(0, img.cols);
			center.y = rng.uniform(0, img.rows);
			Mat pointChunk = points.rowRange(k*sampleCount / clusterCount,
				k == clusterCount - 1 ? sampleCount :
				(k + 1)*sampleCount / clusterCount);
			rng.fill(pointChunk, RNG::NORMAL, Scalar(center.x, center.y), Scalar(img.cols*0.05, img.rows*0.05));
		}
		randShuffle(points, 1, &rng);
		kmeans(points, clusterCount, labels,
			TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 10, 1.0),
			3, KMEANS_PP_CENTERS, centers);
		img = Scalar::all(0);
		for (i = 0; i < sampleCount; i++)
		{
			int clusterIdx = labels.at<int>(i);
			Point ipt = points.at<Point2f>(i);
			circle(img, ipt, 2, colorTab[clusterIdx], FILLED, LINE_AA);
		}
		imshow("clusters", img);
		char key = (char)waitKey();
		if (key == 27 || key == 'q' || key == 'Q') // 'ESC'
			break;
	}
}
int main(){
	test0();
	return 0;
}