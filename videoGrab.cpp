#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

#define WIDTH 320
#define HEIGHT 240

double width = 320;
double height = 240;

double im_width;
double im_height;

struct blob_info {
    uchar blobCnt;
    uint *blobSize;
    uint *AvgX;
    uint *AvgY;
};
blob_info *capture_image(uchar debug);
void get_image(Mat imgInput);
vector<Mat> convert_to_hsv(Mat imgBGR);
void write_hsv_images(vector<Mat> hsv);
void write_bgr_images(vector<Mat> bgr);
Mat negate_image(Mat im);
Mat threshold_image(Mat im,uchar threshold,uchar maxVal);
Mat get_intersection(Mat imA, Mat imB);
Mat gaussian_blur(Mat im, uchar bwidth);
blob_info *blob_detect(Mat img);
void print_blobs(blob_info *Blobs);

uchar is_start_condition(blob_info *BlobInfo);
/*
int main(){
    blob_info *start_blobs;
    blob_info *img_blobs;
    uchar RXbyte = 0;
    uchar start = 0;
    while(!start){
        start_blobs = capture_image();
        start = is_start_condition(img_blobs);
    }
    uchar first_nibble_start = 0;
    while(!first_nibble_start){
        img_blobs = capture_image();
        first_nibble_start = is_first_nibble(img_blobs, start_blobs);
    }
    RXbyte = (get_nibble(img_blobs, start_blobs)) << 4;
    uchar second_nibble_start = 0;
    while(!second_nibble_start){
        blob_info * img_blobs = capture_image();
        second_nibble_start = is_second_nibble(img_blobs, start_blobs);
    }
    RXbyte |= get_nibble(img_blobs, start_blobs);
    printf("%d\n",RXbyte);
    return RXbyte
}
*/
int main(){
    blob_info *start_blobs;
    start_blobs = capture_image(1);
    return 0;
}

blob_info *capture_image(uchar debug){
    /* Get single 320x240 frame */
    Mat imgBGR;
    VideoCapture capture(-1);
    //Set height and width parameters, then get them back
    capture.set(CV_CAP_PROP_FRAME_WIDTH,width);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT,height);
    im_width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
    im_height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    if(!capture.isOpened()){
        cout << "Camera not Working" << endl;
    }
    capture >> imgBGR;

    /* Split RGB components */
    vector<Mat> bgr;
    split(imgBGR, bgr);
    if(debug){
        imwrite("imgB.png",bgr[0]);
        imwrite("imgG.png",bgr[1]);
        imwrite("imgR.png",bgr[2]);
    }

    /* Threshold each component */
    Mat imgB_T = Mat::zeros(bgr[0].size(),bgr[0].type());
    Mat imgG_T = Mat::zeros(bgr[1].size(),bgr[1].type());
    Mat imgR_T = Mat::zeros(bgr[2].size(),bgr[2].type());
    threshold(bgr[0],imgB_T,250,255,THRESH_BINARY);
    threshold(bgr[1],imgG_T,250,255,THRESH_BINARY);
    threshold(bgr[2],imgR_T,250,255,THRESH_BINARY);

    if(debug){
    imwrite("imgBT.png",imgB_T);
    imwrite("imgGT.png",imgG_T);
    imwrite("imgRT.png",imgR_T);
    }

    /* Get intersection of R,G,B */
    Mat B_G = Mat::zeros(imgB_T.size(),imgB_T.type());
    bitwise_and(imgB_T,imgG_T,B_G);
    Mat B_G_R = Mat::zeros(imgB_T.size(),imgB_T.type());
    bitwise_and(B_G,imgR_T,B_G_R);

    if(debug)
        imwrite("imgBGR_I.png",B_G_R);

    /* Blur image */
    Mat BGR_B = Mat::zeros(B_G_R.size(),B_G_R.type());
    GaussianBlur(B_G_R,BGR_B,Size(5,5),0,0);

    if(debug)
        imwrite("imgBGR_B.png",BGR_B);

    /* Threshold to get binary image */
    Mat BGR_B_T = Mat::zeros(BGR_B.size(),BGR_B.type());
    threshold(BGR_B,BGR_B_T,250,1,THRESH_BINARY);

    if(debug)
        imwrite("imgBGR_BT.png",BGR_B_T*255);

    /* Blob detection */
    blob_info *Blobs = blob_detect(BGR_B_T);

    if(debug)
        print_blobs(Blobs);

    return Blobs;
}

/* Blob detection using double raster algorithm */
blob_info *blob_detect(Mat img){
    CV_Assert(img.channels() == 1);
    uchar ctr = 0;
    uchar equiv_size = 64;
    // List to store equivalences: Initialize so all groups are equivalent to themselves
    uchar *equiv;
    equiv = new uchar[equiv_size];
    for(int i=0; i<equiv_size;i++){
        equiv[i] = i;
    }

    // First raster
    for(int j=0; j<img.rows;j++){
        for(int i=0; i<img.cols;i++){
            if(img.at<uchar>(j,i)){
                uchar Pup = 0;
                uchar Pleft = 0;
                //Handle edge cases of leftmost column and topmost row
                if(j > 0){
                    Pup = img.at<uchar>(j-1,i);
                }
                if(i > 0){
                    Pleft = img.at<uchar>(j,i-1);
                }
                if((Pup == 0) && (Pleft == 0)){
                    //If Both left and up pixels are background, we have new group
                    ctr++;
                    img.at<uchar>(j,i) = ctr;
                } else if((Pup == 0) && (Pleft != 0)){
                    //If left is not in background but up is, continue group
                    CV_Assert(i > 0);
                    img.at<uchar>(j,i) = Pleft;
                } else if((Pup != 0) && (Pleft == 0)){
                    //If left is in background but up is not, continue group
                    CV_Assert(j > 0);
                    img.at<uchar>(j,i) = Pup;
                } else {
                    CV_Assert(Pup > 0);
                    CV_Assert(Pleft > 0);
                    if(Pup == Pleft)    //If both up and left are in same group, continue group
                        img.at<uchar>(j,i) = Pup;
                    else {  //Otherwise note equivalence and continue with lesser group
                        CV_Assert(Pup < equiv_size);
                        CV_Assert(Pleft < equiv_size);
                        if(Pleft < Pup){
                            equiv[Pup] = Pleft;
                            img.at<uchar>(j,i) = Pleft;
                        } else {
                            equiv[Pleft] = Pup;
                            img.at<uchar>(j,i) = Pup;
                        }
                    }
                }
            }
        }
    }
    //List to store blob sizes by index
    int *blobCnt;
    blobCnt = new int[equiv_size];
    for(int i=0; i<equiv_size;i++){
        blobCnt[i] = 0;
    }
    //Reduce equivalences in equiv list
    for(int i = 0; i < equiv_size; i++){
        int ptr = i;
        while(equiv[ptr] != ptr){
            ptr = equiv[ptr];
        }
        equiv[i] = ptr;
    }
    //Second raster to apply equivalences
    for(int j=0; j<img.rows;j++){
        for(int i=0; i<img.cols;i++){
            uchar Pval = img.at<uchar>(j,i);
            if(Pval){
                uchar newPval = equiv[Pval];
                if(Pval != newPval){
                    img.at<uchar>(j,i) = newPval;
                }
                blobCnt[newPval]++;
            }
        }
    }
    //Create blob_info struct to return to parent
    blob_info *BlobInfo = new blob_info();
    BlobInfo->blobCnt = 0;
    for(int i=1; i<equiv_size;i++){
        if(blobCnt[i] == 0)
            continue;
        BlobInfo->blobCnt++;
    }
    uchar *blob_id_list;
    blob_id_list = new uchar [BlobInfo->blobCnt];
    BlobInfo->blobSize = new uint [BlobInfo->blobCnt];
    //// Store blob info into 2d array:
    //// id | Size | x | y
    //int **blob_list;
    int blob_list_ptr = 0;
    //blob_list = new int *[BlobInfo->blobCnt];
    for(int i=1; i<equiv_size;i++){
        if(blobCnt[i] == 0)
            continue;
        blob_id_list[blob_list_ptr] = i;
        BlobInfo->blobSize[blob_list_ptr] = blobCnt[i];
        //blob_list[blob_list_ptr] = new int[4];
        //blob_list[blob_list_ptr][0] = i;
        //blob_list[blob_list_ptr][1] = blobCnt[i];
        blob_list_ptr++;
    }
    //Find Center of blobs
    BlobInfo->AvgX = new uint [BlobInfo->blobCnt];
    BlobInfo->AvgY = new uint [BlobInfo->blobCnt];
    for(int i = 0; i < BlobInfo->blobCnt; i++){
        int avg_x = 0;
        int avg_y = 0;
        int id = blob_id_list[i];
        for(int j=0; j<img.rows; j++){
            for(int k=0; k<img.cols; k++){
                if(img.at<uchar>(j,k) == id){
                    avg_x += j;
                    avg_y += k;
                }
            }
        }
        BlobInfo->AvgX[i] = avg_x / BlobInfo->blobSize[i];
        BlobInfo->AvgY[i] = avg_y / BlobInfo->blobSize[i];
    }
    return BlobInfo;
}

void print_blobs(blob_info *Blobs){
    uchar BlobNum = Blobs->blobCnt;
    printf("Found %d Blobs:\n",Blobs->blobCnt);
    for(int i = 0; i < BlobNum; i++){
        printf("size:%d avgX:%d avgY:%d\n",Blobs->blobSize[i],Blobs->AvgX[i],Blobs->AvgY[i]);
    }
    return;
}

uchar is_start_condition(blob_info *BlobInfo){
    if(BlobInfo->blobCnt != 5)
        return 0;
    uint avg_y = 0;
    for(int i=0; i<5; i++){
        avg_y += BlobInfo->AvgY[i];
    }
    avg_y = avg_y / 5;
    for(int i=0; i<5; i++){
        if((BlobInfo->AvgY[i] - avg_y) > 25)
            return 0;
    }
    return 1;
}

uchar is_first_nibble(blob_info *StartBlob, blob_info *FirstBlob){
}

uchar is_second_nibble(blob_info *StartBlob, blob_info *SecondBlob){
}

uchar get_nibble(blob_info *StartBlob, blob_info *ImgBlob){
    uchar nibble = 0;

}

