# SFND 3D Object Tracking

This is part of the Udacity Sensor Fusion Nanodegree.

<img src="images/course_code_structure.png" width="779" height="414" />

In this project, I implemented the missing parts in the schematic:

1. Developed a way to match 3D objects over time by using keypoint correspondences. 
2. Computed the TTC based on Lidar measurements. 
3. Computed the TTC based on two camera frames, which required to first associate keypoint matches to regions of interest and then to compute the TTC based on those matches.

The following are reports on the various tests conducted with the framework. 

## Instances where the Lidar-based TTC estimate is off

### Ghost points

Once you have found those, describe your observations and provide a sound argumentation why you think this happened.

The task is complete once several examples (2-3) have been identified and described in detail. The assertion that the TTC is off should be based on manually estimating the distance to the rear of the preceding vehicle from a top view perspective of the Lidar points.

## Finding suitable detector/descriptor combos for camera-based TTC estimation

Run the program with the command line argument `-series` and pipe the `stdout` to a text file to get data on all the different detector / descriptor combinations. We observe the following:

* some detectors/descriptors extract far fewer keypoints on the image set than others (e.g. `HARRIS`/`SIFT` or `ORB`/`BRISK` are pretty unsuccessful)
* the ratio of matched to unmatched keypoints varies wildy  

Find out which methods perform best


 and looking at the differences in TTC estimation.  and also include 
 
### Examples where camera-based TTC estimation is off

Whether and how strongly camera-based TTC might be off in its estimation depends on the descriptor and detector type. One combination that performs
particularly poorly is AKAZE/ORB. Since we have no ground truth as to what the actual distance or time-to-collision in any given frame is, we have
to rely on comparisons across lidar and other camera-based combos. 

The first frame where we see an outlier in camera-based TTC is frame 11.

We compare this to frame 12, where the cam-based TTC is more in sync with the other estimates again.




The second example is frame 15: 

<img src="images/AKAZE_ORB_Camera_off/AKAZE_ORB_Camera_off/Object classification_screenshot_frame_15.png" width="300" height="100" />
<img src="images/AKAZE_ORB_Camera_off/AKAZE_ORB_Camera_off/3D Objects_screenshot_frame_15.png" width="300" height="100" />
<img src="images/AKAZE_ORB_Camera_off/AKAZE_ORB_Camera_off/Final Results_TTC_screenshot_frame_15.png" width="300" height="100" />

which we again compare to frame 16, which is more plausible again.

<img src="images/AKAZE_ORB_Camera_off/AKAZE_ORB_Camera_off/Object classification_screenshot_frame_16.png" width="300" height="100" />
<img src="images/AKAZE_ORB_Camera_off/AKAZE_ORB_Camera_off/3D Objects_screenshot_frame_16.png" width="300" height="100" />
<img src="images/AKAZE_ORB_Camera_off/AKAZE_ORB_Camera_off/Final Results_TTC_screenshot_frame_16.png" width="300" height="100" />



Describe your observations again and also look into potential reasons.





## Dependencies for Running Locally
* cmake >= 2.8
  * All OSes: [click here for installation instructions](https://cmake.org/install/)
* make >= 4.1 (Linux, Mac), 3.81 (Windows)
  * Linux: make is installed by default on most Linux distros
  * Mac: [install Xcode command line tools to get make](https://developer.apple.com/xcode/features/)
  * Windows: [Click here for installation instructions](http://gnuwin32.sourceforge.net/packages/make.htm)
* Git LFS
  * Weight files are handled using [LFS](https://git-lfs.github.com/)
* OpenCV >= 4.1
  * This must be compiled from source using the `-D OPENCV_ENABLE_NONFREE=ON` cmake flag for testing the SIFT and SURF detectors.
  * The OpenCV 4.1.0 source code can be found [here](https://github.com/opencv/opencv/tree/4.1.0)
* gcc/g++ >= 5.4
  * Linux: gcc / g++ is installed by default on most Linux distros
  * Mac: same deal as make - [install Xcode command line tools](https://developer.apple.com/xcode/features/)
  * Windows: recommend using [MinGW](http://www.mingw.org/)

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory in the top level project directory: `mkdir build && cd build`
3. Compile: `cmake .. && make`
4. Run it: `./3D_object_tracking`.
