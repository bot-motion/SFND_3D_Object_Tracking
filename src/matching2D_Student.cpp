#include <numeric>
#include "matching2D.hpp"

using namespace std;

// Find best matches for keypoints in two camera images based on several matching methods
void matchDescriptors(std::vector<cv::KeyPoint> &kPtsSource, std::vector<cv::KeyPoint> &kPtsRef, cv::Mat &descSource, cv::Mat &descRef,
                     std::vector<cv::DMatch> &matches, std::string descriptorType, std::string matcherType, std::string selectorType)
{
    // configure matcher
    bool crossCheck = false;
    cv::Ptr<cv::DescriptorMatcher> matcher;

    if (matcherType.compare("MAT_BF") == 0)
    {
        int normType = cv::NORM_HAMMING;
        matcher = cv::BFMatcher::create(normType, crossCheck);
    }
    else if (matcherType.compare("MAT_FLANN") == 0)
    {
        matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);

        if(descSource.type()!=CV_32F) 
            descSource.convertTo(descSource, CV_32F);
        if(descRef.type()!=CV_32F) 
            descRef.convertTo(descRef, CV_32F);
    }


    // perform matching task
    if (selectorType.compare("SEL_NN") == 0)
    { 
        // nearest neighbor (best match)
        matcher->match(descSource, descRef, matches); // finds the best match for each descriptor in desc1
    }
    else if (selectorType.compare("SEL_KNN") == 0)
    {
        // k nearest neighbors 
        int k = 2;
        vector<vector<cv::DMatch>> knnMatches;
        matcher->knnMatch(descSource, descRef, knnMatches, k);
        double minDescDistRatio = 0.8;

        for (vector<cv::DMatch> match : knnMatches)
        {
            bool twoKeypointMatchesAreApart = match[0].distance < minDescDistRatio * match[1].distance;
            if (twoKeypointMatchesAreApart) {
                matches.push_back(match[0]);
            }
        }
    }
}

// Use one of several types of state-of-art descriptors to uniquely identify keypoints
float descKeypoints(vector<cv::KeyPoint> &keypoints, cv::Mat &img, cv::Mat &descriptors, string descriptorType)
{
    cv::Ptr<cv::DescriptorExtractor> extractor;
    if (descriptorType.compare("BRISK") == 0)
    {
        int threshold = 30;        // FAST/AGAST detection threshold score.
        int octaves = 3;           // detection octaves (use 0 to do single scale)
        float patternScale = 1.0f; // apply this scale to the pattern used for sampling the neighbourhood of a keypoint.

        extractor = cv::BRISK::create(threshold, octaves, patternScale);
    }
    else if (descriptorType.compare("ORB") == 0) 
    {
        extractor = cv::ORB::create();
    } 
    else if (descriptorType.compare("FREAK") == 0) 
    {
        // extractor = cv::FREAK::create();
        cout << "FREAK could not be run due to standard OpenCV install \n";
    } 
    else if (descriptorType.compare("AKAZE") == 0) 
    {
        extractor = cv::AKAZE::create();
    } 
    else if (descriptorType.compare("SIFT") == 0) 
    {
        extractor = cv::xfeatures2d::SiftDescriptorExtractor::create();
    } 
    else 
    {
      std::cout << descriptorType
                << " is a not valid keypoint descriptor please select from ( "
                   "BRISK, ORB, AKAZE, SIFT)\n";
    }

    // perform feature description
    double t = (double)cv::getTickCount();
    extractor->compute(img, keypoints, descriptors);
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    cout << descriptorType << " descriptor extraction in " << 1000 * t / 1.0 << " ms" << endl;

    return t;
}




/* ------------------------------------------------------------------------------------------------------------------ */


// Detect keypoints in image using the Shi-Thomasi detector
void detKeypointsShiTomasi(vector<cv::KeyPoint> &keypoints, cv::Mat &img)
{
    // compute detector parameters based on image size
    int blockSize = 4;       //  size of an average block for computing a derivative covariation matrix over each pixel neighborhood
    double maxOverlap = 0.0; // max. permissible overlap between two features in %
    double minDistance = (1.0 - maxOverlap) * blockSize;
    int maxCorners = img.rows * img.cols / max(1.0, minDistance); // max. num. of keypoints

    double qualityLevel = 0.01; // minimal accepted quality of image corners
    double k = 0.04;

    // apply corner detection
    vector<cv::Point2f> corners;
    cv::goodFeaturesToTrack(img, corners, maxCorners, qualityLevel, minDistance, cv::Mat(), blockSize, false, k);

    // add corners to result vector
    for (auto c : corners)
    {
        cv::KeyPoint newKeyPoint;
        newKeyPoint.pt = cv::Point2f(c.x, c.y);
        newKeyPoint.size = blockSize;
        keypoints.push_back(newKeyPoint);
    }
}

void detKeypointsHarris(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img) 
{
    // Detector parameters
    int blockSize = 2;     // for every pixel, a blockSize × blockSize neighborhood is considered
    int apertureSize = 3;  // aperture parameter for Sobel operator (must be odd)
    int minResponse = 100; // minimum value for a corner in the 8bit scaled response matrix
    double k = 0.04;       // Harris parameter (see equation for details)

    // Detect Harris corners and normalize output
    cv::Mat dst, dst_norm, dst_norm_scaled;
    dst = cv::Mat::zeros(img.size(), CV_32FC1);
    cv::cornerHarris(img, dst, blockSize, apertureSize, k, cv::BORDER_DEFAULT);
    cv::normalize(dst, dst_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat());
    cv::convertScaleAbs(dst_norm, dst_norm_scaled);

    // Look for prominent corners and instantiate keypoints

    double maxOverlap = 0.0; // max. permissible overlap between two features in
                             // %, used during non-maxima suppression
    for (size_t j = 0; j < dst_norm.rows; j++)
    {
        for (size_t i = 0; i < dst_norm.cols; i++)
        {
            int response = (int)dst_norm.at<float>(j, i);
            if (response > minResponse)
            {   // only store points above a threshold

                cv::KeyPoint newKeyPoint;
                newKeyPoint.pt = cv::Point2f(i, j);
                newKeyPoint.size = 2 * apertureSize;
                newKeyPoint.response = response;

                // perform non-maximum suppression (NMS) in local neighbourhood around
                // new key point
                bool bOverlap = false;
                for (auto currPoint : keypoints)
                {
                    double kptOverlap = cv::KeyPoint::overlap(newKeyPoint, currPoint);
                    if (kptOverlap > maxOverlap)
                    {
                        bOverlap = true;
                        if (newKeyPoint.response > currPoint.response)
                        {                      // if overlap is >t AND response is higher for
                                               // new kpt
                            currPoint = newKeyPoint; // replace old key point with new one
                            break;             
                        }
                    }
                }
                if (!bOverlap)
                {   // only add new key point if no overlap has been found
                    // in previous NMS
                    keypoints.push_back(newKeyPoint); // store new keypoint in dynamic list
                }
            }
        }
    }
}


float detKeypoints(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                        std::string detectorType, bool bVis, string fileName) 
{
  cv::Ptr<cv::FeatureDetector> detector;
  double t_start, period;

  t_start = (double)cv::getTickCount();

  if (detectorType.compare("SHITOMASI") == 0) 
  {
    detKeypointsShiTomasi(keypoints, img);
  }
  else if (detectorType.compare("HARRIS") == 0) 
  {
    detKeypointsHarris(keypoints, img);
  }
  else if (detectorType.compare("FAST") == 0) 
  {
    int threshold = 30; // difference between intensity of the central pixel and
                        // pixels of a circle around this pixel
    bool bNMS = true;   // perform non-maxima suppression on keypoints
    cv::FastFeatureDetector::DetectorType type =
                    cv::FastFeatureDetector::TYPE_9_16; // TYPE_9_16, TYPE_7_12, TYPE_5_8
    detector = cv::FastFeatureDetector::create(threshold, bNMS, type);
    detector->detect(img, keypoints);
  } 
  else if (detectorType.compare("BRISK") == 0) 
  {
     detector = cv::BRISK::create();
     detector->detect(img, keypoints);
  } 
  else if (detectorType.compare("ORB") == 0) 
  {
    detector = cv::ORB::create();
    detector->detect(img, keypoints);
  } 
  else if (detectorType.compare("AKAZE") == 0) 
  {
    detector = cv::AKAZE::create();
    detector->detect(img, keypoints);
  } 
  else if (detectorType.compare("SIFT") == 0) {
    detector = cv::xfeatures2d::SIFT::create(); 
    detector->detect(img, keypoints);
  } 
  else 
  {
    std::cout << detectorType
              << " is a not valid keypoint detectors please select from ( "
                 "SHITOMASI, HARRIS, FAST, BRIEF, ORB, AKAZE, SIFT)\n";
  }

  period = 1000.0f * ((double)cv::getTickCount() - t_start) / cv::getTickFrequency();
  cout << detectorType << " detector with n= " << keypoints.size() << " keypoints in "
       << period << " ms" << endl;

  cv::Mat visImage = img.clone();
  cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1),
                    cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

  if (bVis) 
  {
    string windowName = detectorType + " Detection Results";
    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
    cv::imshow(windowName, visImage);
    cv::waitKey(0);
  }
  else
  {
    bool result;
    try
    {
        result = imwrite(fileName, visImage);
    }
    catch (const cv::Exception& ex)
    {
        std::cout << "Exception converting image in detKeypoints: " << ex.what() << std::endl;
    }
    if (result)
        std::cout << "Saved JPG file in detKeypoints." << std::endl;
    else
        std::cout << "ERROR: Couldn't save image in detKeypoints: " << fileName << std::endl;
    
  }
  return period;
}