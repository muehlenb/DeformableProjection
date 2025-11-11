// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/Data.h"
#include "src/simulation/scene/components/Projector.h"

#include <vector>
#include <opencv2/opencv.hpp>

#undef J
#define PCL_SILENCE_MALLOC_WARNING 1
#include <pcl/point_types.h>
#include <pcl/registration/transformation_estimation.h>
#include <pcl/registration/transformation_estimation_svd.h>
#include <pcl/registration/correspondence_rejection_sample_consensus.h>

/**
 * Finds the projection area and checks how the minimal and maximal
 * brightness of a projector are perceived in the cameras image.
 */

struct Calibrator {
    Calibrator(){}

    static Mat4f fromEigen(Eigen::Matrix4f mat){
        float tmp[16] = {
            mat(0,0), mat(1,0), mat(2,0), mat(3,0),
            mat(0,1), mat(1,1), mat(2,1), mat(3,1),
            mat(0,2), mat(1,2), mat(2,2), mat(3,2),
            mat(0,3), mat(1,3), mat(2,3), mat(3,3),
        };

        return Mat4f(tmp);
    };

    static Eigen::Matrix4f toEigen(Mat4f mat){
        Eigen::Matrix4f tmp;
        tmp(0,0) = mat.data[0];
        tmp(1,0) = mat.data[1];
        tmp(2,0) = mat.data[2];
        tmp(3,0) = mat.data[3];

        tmp(0,1) = mat.data[4];
        tmp(1,1) = mat.data[5];
        tmp(2,1) = mat.data[6];
        tmp(3,1) = mat.data[7];

        tmp(0,2) = mat.data[8];
        tmp(1,2) = mat.data[9];
        tmp(2,2) = mat.data[10];
        tmp(3,2) = mat.data[11];

        tmp(0,3) = mat.data[12];
        tmp(1,3) = mat.data[13];
        tmp(2,3) = mat.data[14];
        tmp(3,3) = mat.data[15];

        return tmp;
    }

    Mat4f calculateProjectionMatrix(const cv::Mat& cameraMatrix, float width, float height, float nearPlane, float farPlane) {
        float fx = cameraMatrix.at<double>(0, 0);
        float fy = cameraMatrix.at<double>(1, 1);
        float cx = cameraMatrix.at<double>(0, 2);
        float cy = cameraMatrix.at<double>(1, 2);

        Mat4f projection;

        projection.data[0] = 2.0f * fx / width;
        projection.data[5] = 2.0f * fy / height;
        projection.data[8] = 2.0f * cx / width - 1.0f;
        projection.data[9] = 2.0f * cy / height - 1.0f;
        projection.data[10] = (farPlane + nearPlane) / (farPlane - nearPlane);
        projection.data[11] = 1.0f;
        projection.data[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        projection.data[15] = 0;

        std::cout << "Projection 9: " << cy << " / " << height << std::endl;

        return projection;
    }

    std::pair<std::vector<Mat4f>, std::vector<Mat4f>> calibrate(std::vector<std::pair<std::vector<cv::Point2f>, std::vector<std::vector<Vec4f>>>>& correspondencePoints){

        std::vector<std::vector<Vec4f>>& camCorrespondences = correspondencePoints[0].second;

        std::vector<Mat4f> cameraMatrices;

        Vec4f c0Min(99999,99999,99999);
        Vec4f c0Max(-99999,-99999,-99999);

        for(Vec4f& p : camCorrespondences[0]){
            c0Min.x = std::min(c0Min.x, p.x);
            c0Min.y = std::min(c0Min.y, p.y);
            c0Min.z = std::min(c0Min.z, p.z);

            c0Max.x = std::max(c0Max.x, p.x);
            c0Max.y = std::max(c0Max.y, p.y);
            c0Max.z = std::max(c0Max.z, p.z);
        }

        Vec4f c0Mid = (c0Min + c0Max) / 2;
        c0Mid.w = 1;

        Vec4f xAxis = (camCorrespondences[0][5] - camCorrespondences[0][0]).normalized();
        Vec4f zAxis = -(camCorrespondences[0][23] - camCorrespondences[0][0]).normalized();

        Vec4f yAxis = xAxis.cross(zAxis).normalized();
        zAxis = yAxis.cross(xAxis);

        Mat4f cam1ToWorld = Mat4f::translation(0, 1.15f, 0) * Mat4f::rotationY(10 * M_PI / 180.f) * Mat4f(-zAxis, yAxis, xAxis, c0Mid).inverse() ;

        std::cout << "Cam1ToWorld: " << cam1ToWorld << std::endl;

        cameraMatrices.push_back(cam1ToWorld);

        // Register camera 2, 3, etc. into camera 1:
        for(int i=1; i < camCorrespondences.size(); ++i){

            std::shared_ptr<pcl::PointCloud<pcl::PointXYZ>> sourcePC(new pcl::PointCloud<pcl::PointXYZ>());
            std::shared_ptr<pcl::PointCloud<pcl::PointXYZ>> targetPC(new pcl::PointCloud<pcl::PointXYZ>());

            for(int h=0; h < camCorrespondences[i].size();++h){
                Vec4f srcPoint = camCorrespondences[0][h];
                Vec4f dstPoint = camCorrespondences[i][h];

                sourcePC->push_back(pcl::PointXYZ(srcPoint.x, srcPoint.y, srcPoint.z));
                targetPC->push_back(pcl::PointXYZ(dstPoint.x, dstPoint.y, dstPoint.z));
            }

            pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ> estimator;
            pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ>::Matrix4 mat;
            estimator.estimateRigidTransformation(*sourcePC, *targetPC, mat);

            Mat4f myMat = fromEigen(mat).inverse();

            std::cout << "Camera " << i << " to 0:" << std::endl;
            std::cout << myMat << std::endl;
            cameraMatrices.push_back(cam1ToWorld * myMat);
        }

        std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds = Data::instance.cameraManager.getCurrentPointClouds();
        for(int i=0; i < pointClouds.size() && i < cameraMatrices.size(); ++i){
            pointClouds[i]->camera->transformation = cameraMatrices[i];
            pointClouds[i]->camera->enableIRLight();
        }

        for(int i=0; i < correspondencePoints.size(); ++i)
            calibrateProjector(i, cameraMatrices, correspondencePoints[i]);

        Data::instance.cameraManager.save();

        return std::pair<std::vector<Mat4f>, std::vector<Mat4f>>();
    };

    void calibrateProjector(int projectorID, std::vector<Mat4f> cameraMatrices, std::pair<std::vector<cv::Point2f>, std::vector<std::vector<Vec4f>>>& correspondencePoints){
        std::vector<std::vector<Vec4f>>& camCorrespondences = correspondencePoints.second;
        std::vector<cv::Point2f>& beamerCorrespondences = correspondencePoints.first;
        {
            // Kameramatrix erstellen
            cv::Mat camera_matrix = (cv::Mat_<double>(3,3) << 4300.79 * 2, 0, 1920,
                                     0, 4300.79 * 2, 1080,
                                     0, 0, 1);

            // Verzerrungskoeffizienten erstellen, angenommen keine Verzerrung
            cv::Mat dist_coeffs = cv::Mat::zeros(5, 1, CV_64F);  // Typischerweise 5 Elemente, auch wenn keine Verzerrung vorliegt


            std::vector<cv::Point3f> object_points;
            std::vector<cv::Point2f> image_points;

            for(int h=0; h < camCorrespondences[0].size(); ++h){
                Vec4f v;

                int counter = 0;
                for(int i=0; i < camCorrespondences.size(); ++i){
                    Vec4f pCS = camCorrespondences[i][h];
                    if(pCS.valid() && pCS.length() > 0.1f){
                        Vec4f p = cameraMatrices[i] * pCS;
                        v = v + p;
                        ++counter;
                    }
                }

                if(counter > 0){
                    v = v / counter;
                    v.w = 1;

                    object_points.push_back(cv::Point3f(v.x, v.y, v.z));
                    image_points.push_back(correspondencePoints.first[h]);
                }
            }

            // Ausführen von solvePnP
            cv::Mat tvecs = (cv::Mat_<double>(3,1) << 0.0, 2.5, 0.0); // in Metern
            cv::Mat rvecs = (cv::Mat_<double>(3,1) << 3.14159 * 0.5, 0.0, 0.0);

            //bool success = cv::solvePnP(object_points, image_points, camera_matrix, dist_coeffs, rvecs, tvecs, false, cv::SOLVEPNP_SQPNP);

            double error = calibrateCamera(std::vector<std::vector<cv::Point3f>>({object_points}), std::vector<std::vector<cv::Point2f>>({image_points}), cv::Size2i(3840, 2160), camera_matrix, dist_coeffs, rvecs, tvecs,
                                           cv::CALIB_USE_INTRINSIC_GUESS | cv::CALIB_ZERO_TANGENT_DIST,
                                           cv::TermCriteria(cv::TermCriteria::COUNT+cv::TermCriteria::EPS, 500, 1e-6));

            std::cout << "Reprojection error: " << error << std::endl;

            cv::Mat rotationMatrix;
            cv::Rodrigues(rvecs, rotationMatrix);

            double tmp = rvecs.at<double>(1);

            Mat4f transf;
            transf.data[0] = rotationMatrix.at<double>(0, 0);
            transf.data[1] = rotationMatrix.at<double>(1, 0);
            transf.data[2] = rotationMatrix.at<double>(2, 0);

            transf.data[4] = rotationMatrix.at<double>(0, 1);
            transf.data[5] = rotationMatrix.at<double>(1, 1);
            transf.data[6] = rotationMatrix.at<double>(2, 1);

            transf.data[8] = rotationMatrix.at<double>(0, 2);
            transf.data[9] = rotationMatrix.at<double>(1, 2);
            transf.data[10] = rotationMatrix.at<double>(2, 2);

            transf.data[12] = tvecs.at<double>(0);
            transf.data[13] = tvecs.at<double>(1);
            transf.data[14] = tvecs.at<double>(2);

            Data::instance.projectors[projectorID]->transformation = transf.inverse();
            std::cout << "Previous Projection Matrix: " << Data::instance.projectors[projectorID]->projectionMatrix << std::endl;
            Data::instance.projectors[projectorID]->projectionMatrix = /*Mat4f::translation(0, -1.08f, 0) **/ calculateProjectionMatrix(camera_matrix, 3840, 2160, 0.1f, 100);
            std::cout << "Projection Matrix: " << Data::instance.projectors[projectorID]->projectionMatrix << std::endl;

            std::cout << "Projektor transform: " << transf;
        }


    }


};
