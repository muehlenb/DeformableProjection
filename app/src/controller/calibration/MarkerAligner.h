// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include <vector>
#include <opencv2/opencv.hpp>

#include "src/controller/calibration/CheckerboardCollector.h"

/**
 * Finds the projection area and checks how the minimal and maximal
 * brightness of a projector are perceived in the cameras image.
 */
struct MarkerAligner {
    MarkerAligner(){}

    /**
     * Assumes an array (for each camera) with same sized arrays of FoundCheckerboard's.
     */
    std::pair<std::vector<cv::Point2f>, std::vector<std::vector<Vec4f>>> getCorrespondencePoints(std::vector<std::vector<FoundCheckerboard>> checkerboards){
        // For each camera:
        for(int i = 0; i < checkerboards.size(); ++i){

            // Invalidate checkerboards with unreasonable corner points:
            for(int h = 0; h < checkerboards[i].size(); ++h){
                if(!checkerboards[i][h].isFound)
                    continue;

                std::cout << checkerboards[i][h].cameraPoints3D.size() << std::endl;
                for(int j = 0; j < checkerboards[i][h].cameraPoints3D.size() - 1; ++j){
                    if(j % 6 == 5)
                        continue;

                    float d = (checkerboards[i][h].cameraPoints3D[j + 1] - checkerboards[i][h].cameraPoints3D[j]).length();

                    // If distance between two corners are bigger than 20cm, this is definitely invalid:
                    if(d > 0.2){
                        std::cout << "Distance between " << j << " and " << (j+1) << " is bigger ( " << d << " ) " << std::endl;
                        checkerboards[i][h].isFound = false;
                    }
                }
            }

            // Calculate moving direction (the direction how multiple
            // checkerboards are moving):
            Vec4f movingDirection;
            for(int x = 0; x < 3; ++x){
                for(int y = 0; y < 4; ++y){
                    int c1idx = y * 4 + x;
                    int c2idx = y * 4 + (x + 1);

                    FoundCheckerboard& c1 = checkerboards[i][c1idx];
                    FoundCheckerboard& c2 = checkerboards[i][c2idx];

                    if(c1.isFound && c2.isFound){
                        Vec4f d = (c2.cameraMid3D - c1.cameraMid3D);
                        d.w = 0;
                        movingDirection = movingDirection + d;
                    }
                }
            }
            movingDirection = movingDirection.normalized();

            // Correctly align point order in moving direction:
            for(int h = 0; h < checkerboards[i].size(); ++h){
                if(checkerboards[i][h].isFound){
                    Vec4f dir = (checkerboards[i][h].cameraPoints3D[1] - checkerboards[i][h].cameraPoints3D[0]).normalized();
                    float d = movingDirection.dot(dir);

                    if(d < 0){
                        std::reverse(checkerboards[i][h].cameraPoints3D.begin(), checkerboards[i][h].cameraPoints3D.end());
                        std::reverse(checkerboards[i][h].cameraImagePoints.begin(), checkerboards[i][h].cameraImagePoints.end());
                    }
                }
            }
        }

        // Create Correspondences:
        if(checkerboards.size() > 0){
            std::pair<std::vector<cv::Point2f>, std::vector<std::vector<Vec4f>>> results;
            int maxSize = 999999999;

            // Create an array for every camera:
            for(int i = 0; i < checkerboards.size(); ++i){
                results.second.push_back(std::vector<Vec4f>());
                maxSize = std::min(maxSize, int(checkerboards[i].size()));
            }

            for(int h = 0; h < maxSize; ++h){
                bool isAvailableInEveryImage = true;
                for(int i = 0; i < checkerboards.size(); ++i){
                    if(!checkerboards[i][h].isFound){
                        isAvailableInEveryImage = false;
                    }
                }

                if(isAvailableInEveryImage){
                    for(int i = 0; i < checkerboards.size(); ++i){
                        for(int j = 0; j < checkerboards[i][h].cameraPoints3D.size(); ++j){
                            results.second[i].push_back(checkerboards[i][h].cameraPoints3D[j]);
                        }
                    }

                    for(cv::Point2f& p : checkerboards[0][h].beamerImagePoints)
                        results.first.push_back(cv::Point2f(p.x, p.y));
                }
            }

            return results;
        }

        return std::pair<std::vector<cv::Point2f>, std::vector<std::vector<Vec4f>>>();
    }
};
