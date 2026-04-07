#pragma once
// CalibrationService — ArUco detection + homography, mirrors src/services/calibration_service.py

#include "models/calibration_model.h"
#include "services/interfaces.h"
#include "utils/constants.h"

#include <opencv2/aruco.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <string>

namespace sag {

class CalibrationService : public ICalibrationService {
public:
    CalibrationService() = default;

    void set_dictionary(const std::string& name) { dictionary_name_ = name; }

    CalibrationModel calibrate(
        const cv::Mat& frame,
        const CalibrationMarkerLayout& layout,
        float playable_width,
        float playable_height,
        float playable_inset) override;

    static std::vector<std::string> supported_dictionary_names();

private:
    std::string dictionary_name_ = "DICT_6X6_1000";
    static cv::Ptr<cv::aruco::Dictionary> get_dictionary(const std::string& name);
};

} // namespace sag
