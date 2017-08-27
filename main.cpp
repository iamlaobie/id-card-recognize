    #include "idr.pch"

using namespace cv;
using namespace std;

Mat src; Mat src_gray;
int thresh = 100;
int max_thresh = 255;


bool vsort( Rect v1, Rect v2)
{
    return v1.x < v2.x;
}

int utime ()
{
    struct timeval tm;
    gettimeofday(&tm, NULL);
    return tm.tv_sec * 1000 + tm.tv_usec / 1000;
}

void rotateByAngle(Mat, Mat, double);
Mat thresholdByAngle(Mat);
void toHorizontal(Mat*);
vector<Rect> findWordRect(vector<vector<Point>>);
map<int, vector<Rect>> clusterByY(vector<Rect>);
int getMaxCluster(map<int, vector<Rect>>);


/** @function main */
int main( int argc, char** argv )
{
    RNG rng(utime());
    
    /// Load source image and convert it to gray
    src = imread( argv[1], 1 );
    //src = imread( "/Users/liujun/Documents/software/opencv/7.jpg", 1 );
    string path = argv[2];
    if (src.rows > src.cols) {
        transpose(src, src);
        flip(src, src, 1);
    }
    float scale = 1000.0 / (double)src.cols;
    Size newSize = Size(src.cols * scale, src.rows * scale);
    resize(src, src, newSize, 0,0,CV_INTER_LINEAR);
    cvtColor( src, src_gray, CV_BGR2GRAY );
    //blur( src_gray, src_gray, Size(3, 3) );
    
    Mat threshold_output = thresholdByBright(src_gray);
    
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    vector<Rect> wordRect; //所有近似文字的轮廓
    vector<Rect> numRect; //身份证号区域的文字璐娜
    map<int, vector<Rect>> clusters; //沿Y方向聚类的结果
    map<int, vector<Rect>>::iterator maxIt;
    int maxY = 0;
    findContours(threshold_output.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
    
    wordRect = findWordRect(contours);
    clusters = clusterByY(wordRect);
    maxY = getMaxCluster(clusters);
    maxIt = clusters.find(maxY);
    
    //如果身份证号的位置在图片中线的上方，身份证图片倒立，需要翻转
    //翻转后重新文字位置
    if (maxIt->first < src.rows / 2) {
        flip(threshold_output, threshold_output, 0);
        flip(threshold_output, threshold_output, 1);
        findContours( threshold_output.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
        wordRect = findWordRect(contours);
        clusters = clusterByY(wordRect);
        maxY = getMaxCluster(clusters);
        maxIt = clusters.find(maxY);
    }
    
    numRect = maxIt->second;
    if (numRect.size() < 18) {
        cout << "error: not found id num" << endl;
        exit(0);
    }
    sort(numRect.begin(), numRect.end(), vsort);

    int first = 0, last = 0;
    Rect idStart;
    for (int i = 0; i < numRect.size(); i++) {
        Rect r = numRect[i];
        if (first == 0 && r.height >= 30) {
            first = i;
        }
        
        if (r.height > 30) {
            last = i;
        }
        if (i > 0 && (r.x - numRect[i - 1].x - r.width >= 30)) {
            idStart = r;
        }
    }
    if (idStart.x < 100 && idStart.x > (src.rows / 2)) {
        cout << "error: not found id num" << endl;
        exit(0);
    }
 
    //printf("%d,%d,%d,%d\n", boundRect[last].y, boundRect[first].y, boundRect[last].x,  boundRect[first].x);
    double angle = getAngle(numRect[last], numRect[first]);
    rotateByAngle(threshold_output, threshold_output, angle);
    string key = to_string(rng.uniform(0, 99999));
    Mat id = Mat(600, 100, CV_8UC3);
    threshold_output(Rect(idStart.x - 20, idStart.y - 20, 600, 100)).copyTo(id);
    Mat name = Mat(270, 70, CV_8UC3);
    threshold_output(Rect(idStart.x - 160, idStart.y - idStart.height * 13, 300, 80)).copyTo(name);
        
    imwrite(path + key + "id.jpg", id);
    imwrite(path + key + "name.jpg", name);
    src.release();
    id.release();
    name.release();
    threshold_output.release();
    src_gray.release();
    cout<<key;
    /*
    /// Draw polygonal contour + bonding rects + circles
    Mat drawing = Mat::zeros( threshold_output.size(), CV_8UC3 );
    for( int i = 0; i< boundRect.size(); i++ )
    {
        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        //drawContours( drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
        rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );
        imshow( "Contours", drawing );
        waitKey();
    }
     */
    
}
