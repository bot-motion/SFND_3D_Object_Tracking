
#include <iostream>
#include <algorithm>
#include <numeric>
#include <string>
#include <utility>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>

#include "camFusion.hpp"
#include "dataStructures.h"

using namespace std;


// Create groups of Lidar points whose projection into the camera falls into the same bounding box
void clusterLidarWithROI(std::vector<BoundingBox> &boundingBoxes, std::vector<LidarPoint> &lidarPoints, float shrinkFactor, cv::Mat &P_rect_xx, cv::Mat &R_rect_xx, cv::Mat &RT)
{
    // loop over all Lidar points and associate them to a 2D bounding box
    cv::Mat X(4, 1, cv::DataType<double>::type);
    cv::Mat Y(3, 1, cv::DataType<double>::type);

    for (auto it1 = lidarPoints.begin(); it1 != lidarPoints.end(); ++it1)
    {
        // assemble vector for matrix-vector-multiplication
        X.at<double>(0, 0) = it1->x;
        X.at<double>(1, 0) = it1->y;
        X.at<double>(2, 0) = it1->z;
        X.at<double>(3, 0) = 1;

        // project Lidar point into camera
        Y = P_rect_xx * R_rect_xx * RT * X;
        cv::Point pt;
        pt.x = Y.at<double>(0, 0) / Y.at<double>(0, 2); // pixel coordinates
        pt.y = Y.at<double>(1, 0) / Y.at<double>(0, 2);

        vector<vector<BoundingBox>::iterator> enclosingBoxes; // pointers to all bounding boxes which enclose the current Lidar point
        for (vector<BoundingBox>::iterator it2 = boundingBoxes.begin(); it2 != boundingBoxes.end(); ++it2)
        {
            // shrink current bounding box slightly to avoid having too many outlier points around the edges
            cv::Rect smallerBox;
            smallerBox.x = (*it2).roi.x + shrinkFactor * (*it2).roi.width / 2.0;
            smallerBox.y = (*it2).roi.y + shrinkFactor * (*it2).roi.height / 2.0;
            smallerBox.width = (*it2).roi.width * (1 - shrinkFactor);
            smallerBox.height = (*it2).roi.height * (1 - shrinkFactor);

            // check wether point is within current bounding box
            if (smallerBox.contains(pt))
            {
                enclosingBoxes.push_back(it2);
            }

        } // eof loop over all bounding boxes

        // check wether point has been enclosed by one or by multiple boxes
        if (enclosingBoxes.size() == 1)
        { 
            // add Lidar point to bounding box
            enclosingBoxes[0]->lidarPoints.push_back(*it1);
        }

    } // eof loop over all Lidar points
}


void show3DObjects(std::vector<BoundingBox> &boundingBoxes, cv::Size2f worldSize, cv::Size imageSize, bool bWait, string imgTitle)
{
	//to better visual lidar point top view, fix the starting world size as 6
	const float START_HEIGHT = 6.8;
	worldSize.height = worldSize.height - START_HEIGHT;
    // create topview image
    cv::Mat topviewImg(imageSize, CV_8UC3, cv::Scalar(255, 255, 255));

    for(auto it1=boundingBoxes.begin(); it1!=boundingBoxes.end(); ++it1)
    {
        // create randomized color for current 3D object
        cv::RNG rng(it1->boxID);
        cv::Scalar currColor = cv::Scalar(rng.uniform(0,150), rng.uniform(0, 150), rng.uniform(0, 150));

        // plot Lidar points into top view image
        int top=1e8, left=1e8, bottom=0.0, right=0.0; 
        float xwmin=1e8, ywmin=1e8, ywmax=-1e8;
        if(it1->lidarPoints.size() < 3){
        	//skip the bounding box display if it contains less than three points.
        	continue;
        }
        for (auto it2 = it1->lidarPoints.begin(); it2 != it1->lidarPoints.end(); ++it2)
        {
            // world coordinates
            float xw = (*it2).x; // world position in m with x facing forward from sensor
            float yw = (*it2).y; // world position in m with y facing left from sensor
            xwmin = xwmin<xw ? xwmin : xw;
            ywmin = ywmin<yw ? ywmin : yw;
            ywmax = ywmax>yw ? ywmax : yw;

            // top-view coordinates
            int y = (-(xw- START_HEIGHT) * imageSize.height / worldSize.height) + imageSize.height;
            int x = (-yw * imageSize.width / worldSize.width) + imageSize.width / 2;

            // find enclosing rectangle
            top = top<y ? top : y;
            left = left<x ? left : x;
            bottom = bottom>y ? bottom : y;
            right = right>x ? right : x;

            // draw individual point
            cv::circle(topviewImg, cv::Point(x, y), 4, currColor, -1);
        }
        // draw enclosing rectangle
        cv::rectangle(topviewImg, cv::Point(left, top), cv::Point(right, bottom),cv::Scalar(0,0,0), 2);

        // augment object with some key data
        char str1[200], str2[200];
        sprintf(str1, "box id=%d, #pts=%d", it1->boxID, (int)it1->lidarPoints.size());
        putText(topviewImg, str1, cv::Point2f(left-250, bottom+50), cv::FONT_ITALIC, 1, currColor);
        sprintf(str2, "xmin=%2.2f m, yw=%2.2f m", xwmin, ywmax-ywmin);
        putText(topviewImg, str2, cv::Point2f(left-250, bottom+125), cv::FONT_ITALIC, 1, currColor);
        cout<<"xmin="<<xwmin<<",yw="<<ywmax-ywmin<<",id="<<it1->boxID<<",pts="<<it1->lidarPoints.size()<<endl;
    }

    // plot distance markers
    int nMarkers = 10;
    float lineSpacing =  worldSize.height / float(nMarkers);

    for (size_t i = 0; i < nMarkers; ++i)
    {
        int y = (-(i * lineSpacing) * imageSize.height / worldSize.height) + imageSize.height;
        cv::line(topviewImg, cv::Point(0, y), cv::Point(imageSize.width, y), cv::Scalar(255, 0, 0));
    }

    if(bWait)
    {
        std::cout << "show3DObjects - press key to continue..." << std::endl;
    	// display image
		string windowName = "3D Objects" + imgTitle;
		cv::namedWindow(windowName, 1);
		cv::imshow(windowName, topviewImg);
        cv::waitKey(0); 
    }
    else
    {
        string fileName = imgTitle; 
        bool result;
        try
        {
            result = imwrite(fileName, topviewImg);
        }
        catch (const cv::Exception& ex)
        {
            std::cout << "Exception converting image in show3DObjects: " << ex.what() << std::endl;
        }
        if (result)
            std::cout << "Saved " << fileName << " file in show3DObjects." << std::endl;
        else
            std::cout << "ERROR: Couldn't save image in show3DObjects." << std::endl;
    }
}


struct ExtendedDMatch
{
    cv::DMatch match;
    double euclideanDistance;
};

bool Compare(ExtendedDMatch a, ExtendedDMatch b) 
{
    return a.euclideanDistance < b.euclideanDistance;
}


// Associate a given bounding box with the keypoints it contains
void clusterKptMatchesWithROI(BoundingBox &boundingBox, std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, std::vector<cv::DMatch> &kptMatches)
{
    std::vector<cv::DMatch> kptsROI;
    std::vector<struct ExtendedDMatch> matchesWithDistances;

    for (auto match: kptMatches)
    {
        bool keypointInBoundingBox = boundingBox.roi.contains(kptsCurr[match.trainIdx].pt) &&
                                     boundingBox.roi.contains(kptsCurr[match.queryIdx].pt);

        if(keypointInBoundingBox)
        {
            cv::KeyPoint kpInnerCurr = kptsCurr.at(match.trainIdx);
            cv::KeyPoint kpInnerPrev = kptsPrev.at(match.queryIdx);
            float eucDistBetwKpts = std::sqrt(std::pow((kpInnerCurr.pt.x - kpInnerPrev.pt.x),2)
                                             +std::pow((kpInnerCurr.pt.y - kpInnerPrev.pt.y),2));

            struct ExtendedDMatch m;
            m.euclideanDistance = eucDistBetwKpts;
            m.match = match;
            
            matchesWithDistances.push_back(m); 
        }
    }

    if (matchesWithDistances.size() > 2)
    {
        std::sort(matchesWithDistances.begin(),matchesWithDistances.end(),Compare);

        int medianIndex = floor(matchesWithDistances.size()/2);
        int q1Index = floor(matchesWithDistances.size()/4);
        int q3Index = floor(matchesWithDistances.size()/4 * 3);

        double medianDistance = matchesWithDistances[medianIndex].euclideanDistance;
        double q1Distance = matchesWithDistances[q1Index].euclideanDistance;
        double q3Distance = matchesWithDistances[q3Index].euclideanDistance;

        double iqrDistance = q3Distance - q1Distance;

        for(auto md = matchesWithDistances.begin(); md !=matchesWithDistances.end(); md++)
        {
            bool isOutlier = md->euclideanDistance < q1Distance-iqrDistance && md->euclideanDistance > q3Distance+iqrDistance;
            if(!isOutlier)
            {
                kptsROI.push_back(md->match);
            }
        }
    }
    
    boundingBox.kptMatches = kptsROI;

}




// Compute time-to-collision (TTC) based on keypoint correspondences in successive images
void computeTTCCamera(std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, 
                      std::vector<cv::DMatch> kptMatches, double frameRate, double &TTC, cv::Mat *visImg)
{
    double minDist = 100.0; // min. required distance

    if (kptMatches.size() < 2)
    {
        TTC = NAN;
        return;
    }

    // We take two frames (current and previous) and two pairs of matched keypoints (firstMatch and secondMatch).
    // Compute distance ratios between all matched keypoints.

    vector<double> distRatios; // stores the distance ratios for all keypoints between curr. and prev. frame
    for (auto firstMatch = kptMatches.begin(); firstMatch != kptMatches.end() - 1; ++firstMatch) 
    { 
        // get first keypoint and its matched partner in the prev. frame
        cv::KeyPoint kpFirstCurrFrame = kptsCurr.at(firstMatch->trainIdx);
        cv::KeyPoint kpFirstPrevFrame = kptsPrev.at(firstMatch->queryIdx);

        for (auto secondMatch = kptMatches.begin() + 1; secondMatch != kptMatches.end(); ++secondMatch) 
        {   
            // get second keypoint match
            cv::KeyPoint kpSecondCurrFrame = kptsCurr.at(secondMatch->trainIdx);  
            cv::KeyPoint kpSecondPrevFrame = kptsPrev.at(secondMatch->queryIdx);

            // compute distances and distance ratios
            double distanceofKpsInCurrFrame = cv::norm(kpFirstCurrFrame.pt - kpSecondCurrFrame.pt);
            double distanceOfKpsInPrevFrame = cv::norm(kpFirstPrevFrame.pt - kpSecondPrevFrame.pt);

            if (distanceOfKpsInPrevFrame > std::numeric_limits<double>::epsilon() && distanceofKpsInCurrFrame >= minDist)
            { 
                double distRatio = distanceofKpsInCurrFrame / distanceOfKpsInPrevFrame;
                distRatios.push_back(distRatio);
            }
        } 
    }    

    if (distRatios.size() > 0)
    {
        double dT = 1 / frameRate;
        double medianDistRatio = 0.0f;
        
        std::sort(distRatios.begin(), distRatios.end());
        long medianIndex = distRatios.size()/2;

        if (distRatios.size() % 2 == 0)
        {
            medianDistRatio = (distRatios[medianIndex-1]+distRatios[medianIndex])/2;
        }
        else
            medianDistRatio = distRatios[medianIndex];

        TTC = -dT / (1 - medianDistRatio);
    }
    else
    {
        TTC = NAN;
    }
}



double nthSmallestDistance(std::vector<LidarPoint> &lidarPoints, double N)
// return the Nth smallest distance in x-direction of a collection of lidar points,
// or the largest distance among n < N x-distances if there are no N distances.
{
	vector<double> nSmallestDistances;

	for (auto lidarPoint = lidarPoints.begin(); lidarPoint != lidarPoints.end(); ++lidarPoint)
	{
        bool foundOneOfNSmallestDistances;
        if (nSmallestDistances.size() == 0)
            foundOneOfNSmallestDistances = true;
        else
            foundOneOfNSmallestDistances = lidarPoint->x < nSmallestDistances.back();

        if (foundOneOfNSmallestDistances)
        {
            nSmallestDistances.push_back(lidarPoint->x);
            std::sort(nSmallestDistances.begin(), nSmallestDistances.end());
            if (nSmallestDistances.size() > N)
                nSmallestDistances.pop_back();
        }
    }

    return nSmallestDistances.back();
}



void computeTTCLidar(std::vector<LidarPoint> &lidarPointsPrev,
                     std::vector<LidarPoint> &lidarPointsCurr, double frameRate, double &TTC)
{
    double dT = 1/frameRate;        
    const int N = 7;

    double minXPrev = nthSmallestDistance(lidarPointsPrev, N);
    double minXCurr = nthSmallestDistance(lidarPointsCurr, N);    

    if ((minXPrev == 0 && minXCurr == 0) || (minXPrev == minXCurr))
        TTC = NAN;
    else    
	    TTC = minXCurr * dT / (minXPrev - minXCurr);
} 


void matchBoundingBoxes(std::vector<cv::DMatch> &matches, std::map<int, int> &bbBestMatches, DataFrame &prevFrame, DataFrame &currFrame)
{
    map<int, map<int, int>> boxMatchings;       // the first key is a boxID in the current frame; the second key is a boxID in the prev frame.
                                                // the value is the number of kp matches between both boxes. 

    for (auto match : matches)
    {
        cv::KeyPoint prevKeypoint = prevFrame.keypoints[match.queryIdx];
        cv::KeyPoint currKeypoint = currFrame.keypoints[match.trainIdx];

        int boxPrev = -1, boxCurr = -1;

        for (auto box : prevFrame.boundingBoxes)
            if(box.roi.contains(prevKeypoint.pt)) 
            {
                boxPrev = box.boxID;
                break;
            }

        for (auto box : currFrame.boundingBoxes)
            if(box.roi.contains(currKeypoint.pt)) 
            {
                boxCurr = box.boxID;
                break;
            }          

        if (boxPrev == -1 || boxCurr == -1) continue;

        bool foundFirstKpMatchForCurrBox = boxMatchings.find(boxCurr) == boxMatchings.end();
        if (foundFirstKpMatchForCurrBox) 
        {
            map<int, int> a;
            boxMatchings.insert(make_pair(boxCurr, a));
        }

        bool foundFirstKpMatchForPrevBox = boxMatchings[boxCurr].find(boxPrev) == boxMatchings[boxCurr].end();
        if (foundFirstKpMatchForPrevBox)
            boxMatchings[boxCurr].insert(make_pair(boxPrev, 0));

        boxMatchings[boxCurr][boxPrev]++;  // increase kp match count
    }

    for(auto boxMatch : boxMatchings)
    {
        int currBox = boxMatch.first;
        int bestPrevBox = max_element(boxMatch.second.begin(), boxMatch.second.end(), [] (const std::pair<int,int>& a, const std::pair<int,int>& b)->bool{ return a.second < b.second; })->first;
        bbBestMatches.insert(make_pair(currBox, bestPrevBox));
    }
}