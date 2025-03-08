// References:
// https://learnopencv.com/improving-illumination-in-night-time-images/
// https://link.springer.com/article/10.1186/s13640-018-0251-4 
// https://github.com/ba-san/Python-image-enhancement-with-bright-dark-prior
// https://github.com/spmallick/learnopencv/tree/master/Improving-Illumination-in-Night-Time-Images

// Initial setup:
// https://users.utcluj.ro/~igiosan/Resources/PI/Apps/Windows/

#include "stdafx.h"
#include "common.h"
#include <opencv2/core/utils/logger.hpp>
#include <map>
#include <sstream>

#undef min
#undef max
#include <algorithm>

wchar_t* projectPath;

using ColorImage = Mat_<Vec3d>;
using GrayImage = Mat_<double>;

const int PATCH_RADIUS = 7 / 2;
const double ATMOSPHERIC_TOP_X = 0.1;
const double CORRECTION_THRESHOLD = 0.4;
const double DARK_CHANNEL_CORRECTION_COEFFICIENT = 0.75;
const double REGULARIZATION = 0.2;
const double MIN_TRANSMISSION = 0.1;

ColorImage chooseImage() {
    char fname[MAX_PATH];
    if (!openFileDlg(fname))
        exit(0);

    Mat_<Vec3b> img = imread(fname, IMREAD_COLOR);
    ColorImage doubleImg(img.rows, img.cols);

    for (int i = 0; i < img.rows; i++)
        for (int j = 0; j < img.cols; j++) {
            auto px = img(i, j);
            auto& dPx = doubleImg(i, j);

            for (int ch = 0; ch < 3; ch++)
                dPx[ch] = px[ch] / 255.0;
        }

    return doubleImg;
}

inline bool isOutsideImg(const Mat& img, const int row, const int col) {
    return row < 0 || row >= img.rows ||
        col < 0 || col >= img.cols;
}

inline bool isOutsideImg(const Mat& img, const Point& p) { return isOutsideImg(img, p.y, p.x); }

struct Channels {
    GrayImage dark;
    GrayImage bright;

    Channels clone() const {
        return { dark.clone(), bright.clone() };
    }
};

void goThroughPatch(
    const int row,
    const int col,
    std::function<void(const int, const int)> fn) {
    for(int i = row - PATCH_RADIUS; i <= row + PATCH_RADIUS; i++)
        for(int j = col - PATCH_RADIUS; j <= col + PATCH_RADIUS; j++)
            fn(i, j);
}

Channels computeChannels(const ColorImage& img) {
    std::cout << "Computing prior channels...\n";

    const auto pixelMin = [](const Vec3d& a, const Vec3d& b) {
        return Vec3d(std::min(a[0], b[0]), std::min(a[1], b[1]), std::min(a[2], b[2]));
    };
    const auto pixelMax = [](const Vec3d& a, const Vec3d& b) {
        return Vec3d(std::max(a[0], b[0]), std::max(a[1], b[1]), std::max(a[2], b[2]));
    };

    const int rows = img.rows;
    const int cols = img.cols;

    GrayImage dark(rows, cols, 0.0);
    GrayImage bright(rows, cols, 1.0);

    for (int i = PATCH_RADIUS; i < rows - PATCH_RADIUS; i++)
        for (int j = PATCH_RADIUS; j < cols - PATCH_RADIUS; j++) {
            auto patchMin = img(i, j);
            auto patchMax = img(i, j);

            goThroughPatch(i, j, [&](int y, int x){
                if (isOutsideImg(img, y, x)) return;

                patchMin = pixelMin(patchMin, img(y, x));
                patchMax = pixelMax(patchMax, img(y, x));
            });

            dark(i, j)   = std::min({ patchMin[0], patchMin[1], patchMin[2] });
            bright(i, j) = std::max({ patchMax[0], patchMax[1], patchMax[2] });
        }

    return { dark, bright };
}

Vec3d computeAtmosphericIllumination(const ColorImage& img, const GrayImage& bright) {
    std::cout << "Computing atmospheric illumination...\n";

    int rows = bright.rows;
    int cols = bright.cols;

    std::vector<std::pair<double, Point>> flatBright;

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            flatBright.push_back({ -bright(i, j), Point(j, i) });

    std::sort(
        flatBright.begin(),
        flatBright.end(),
        [](const std::pair<double, Point>& a, const std::pair<double, Point>& b) { return a.first < b.first; }
    );

    int topX = flatBright.size() * ATMOSPHERIC_TOP_X;
    Vec3d illumination({ 0, 0, 0 });

    for (int i = 0; i < topX; i++) {
        auto px = img(flatBright[i].second);

        for (int ch = 0; ch < 3; ch++)
            illumination[ch] += px[ch];
    }

    illumination /= topX;

    return illumination;
}

GrayImage normalize(const GrayImage& img) {
    double min = img(0, 0), max = img(0, 0);

    for (int i = 0; i < img.rows; i++)
        for (int j = 0; j < img.cols; j++) {
            min = std::min(min, img(i, j));
            max = std::max(max, img(i, j));
        }

    return (img - min) / (max - min);
}

GrayImage computeTransmission(
    const GrayImage& bright,
    const Vec3d illumination
) {
    std::cout << "Computing transmission...\n";

    const double illuminationMax = std::max({ illumination[0], illumination[1], illumination[2] });

    GrayImage transmission = bright.clone();
    transmission = (transmission - illuminationMax) / (1.0 - illuminationMax);
    transmission = normalize(transmission);

    return transmission;
}

GrayImage correctTransmission(const ColorImage& img, const Channels& channels, const Vec3d& illumination, const GrayImage& transmission) {
    std::cout << "Correcting transmission...\n";

    ColorImage amplifiedImage(img.rows, img.cols);

    for (int i = 0; i < img.rows; i++)
        for (int j = 0; j < img.cols; j++)
            for (int ch = 0; ch < 3; ch++)
                amplifiedImage(i, j)[ch] = img(i, j)[ch] / illumination[ch];
 
    GrayImage correctionMatrix = computeChannels(amplifiedImage).dark;
    correctionMatrix = 1 - DARK_CHANNEL_CORRECTION_COEFFICIENT * correctionMatrix;

    GrayImage correctedTransmission = transmission.clone();
    GrayImage channelsDiff = channels.bright - channels.dark;
 
    for (int i = 0; i < channelsDiff.rows; i++)
        for (int j = 0; j < channelsDiff.cols; j++)
            if (channelsDiff(i, j) < CORRECTION_THRESHOLD) {
                correctedTransmission(i, j) *= correctionMatrix(i, j);
                correctedTransmission(i, j) = std::abs(correctedTransmission(i, j));
            }
 
    return correctedTransmission;
}

GrayImage applyGuidedFilter(const ColorImage& img, const GrayImage& transmission) {
    std::cout << "Applying guided filter...\n";

    GrayImage grayImg(img.rows, img.cols);

    for(int i = 0; i < img.rows; i++)
        for (int j = 0; j < img.cols; j++) {
            const auto& px = img(i, j);

            grayImg(i, j) = (px[0] + px[1] + px[2]) / 3;
        }

    GrayImage filtered(img.rows, img.cols);

    for(int row = 0; row < img.rows; row++)
        for(int col = 0; col < img.cols; col++) {
            double mean = 0;
            int pixels = 0;

            goThroughPatch(row, col, [&](int y, int x){
                if (isOutsideImg(img, y, x)) return;

                mean += grayImg(y, x);
                pixels++;
            });
            mean /= pixels;

            double meanTransmission = 0;
            int pixelsTransmission = 0;

            goThroughPatch(row, col, [&](int y, int x){
                if (isOutsideImg(transmission, y, x)) return;

                meanTransmission += transmission(y, x);
                pixelsTransmission ++;
            });
            meanTransmission /= pixelsTransmission;

            double variance = 0;

            goThroughPatch(row, col, [&](int y, int x) {
                if (isOutsideImg(grayImg, y, x)) return;

                double diff = grayImg(y, x) - mean;
                variance += diff * diff;
            });

            variance /= pixels;

            double a;

            goThroughPatch(row, col, [&](int y, int x) {
                if (isOutsideImg(transmission, y, x)) return;

                a += grayImg(y, x) * transmission(y, x) - mean * transmission(row, col);
            });

            a /= pixels;
            a /= variance + REGULARIZATION;
            a = std::abs(a);

            double b = meanTransmission - a * mean;
            filtered(row, col) = a * transmission(row, col) + b;
        }

    return filtered;
}

ColorImage computeScene(
    std::string name,
    const ColorImage& img,
    const Vec3d& illumination,
    const GrayImage& transmission) {
    name = "scene " + name;

    std::cout << "Computing " << name << "...\n";

    ColorImage scene(img.rows, img.cols);

    for(int i = 0; i < img.rows; i++)
        for(int j = 0; j < img.cols; j++) {
            scene(i, j) = (img(i, j) - illumination);
            scene(i, j) /= std::max(transmission(i, j), MIN_TRANSMISSION);

            for (int ch = 0; ch < 3; ch++)
                scene(i, j)[ch] += illumination[ch];
        }

    return normalize(scene);
}

void __saveImage(const std::string& filename, const Mat& image) {
    bool result = cv::imwrite(filename, image);
    if (result)
        std::cout << "Image saved successfully: " << filename << std::endl;
    else
        std::cout << "Could not save: " << filename << std::endl;
}

void saveImage(const std::string& filename, const Mat_<uchar>& image) {
    __saveImage(filename, image);
}

void saveImage(const std::string& filename, const Mat_<Vec3b>& image) {
    __saveImage(filename, image);
}

void saveImage(const std::string& filename, const Mat_<double>& image) {
    Mat_<uchar> img = image * 255;
    saveImage(filename, img);
}

void saveImage(const std::string& filename, const Mat_<Vec3d>& image) {
    Mat_<Vec3b> img = image * 255;
    saveImage(filename, img);
}

int main() {
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_FATAL);
    projectPath = _wgetcwd(0, 0);

    std::wstring ws(projectPath);
    std::string str(ws.begin(), ws.end());
    std::ostringstream oss;

    oss << str;
    oss << "/saves/";

    std::string savePath = oss.str();

    while (1) {
        const auto img = chooseImage();
        const auto channels = computeChannels(img.clone());
        const auto illumination = computeAtmosphericIllumination(img.clone(), channels.bright.clone());

        const auto transmission = computeTransmission(channels.bright.clone(), illumination);
        const auto correctedTransmission = correctTransmission(img.clone(), channels.clone(), Vec3d(illumination), transmission.clone());
        const auto filteredTransmission = applyGuidedFilter(img.clone(), correctedTransmission.clone());

        std::pair<std::string, const GrayImage&> transmissions[] = {
            { "initial", transmission },
            { "corrected", correctedTransmission },
            { "filtered",  filteredTransmission },
        };

        imshow("input", img);

        imshow("dark channel", channels.dark);
        saveImage(savePath + "dark.bmp", channels.dark);

        imshow("bright channel", channels.bright);
        saveImage(savePath + "bright.bmp", channels.bright);

        for (const auto& entry : transmissions) {
            auto s = computeScene(entry.first, img.clone(), Vec3d(illumination), entry.second.clone());

            imshow(entry.first + " transmission", entry.second);
            saveImage(savePath + entry.first + " transmission" + ".bmp", entry.second);

            imshow(entry.first + " scene", s);
            saveImage(savePath + entry.first + " scene" + ".bmp", s);
        }

        waitKey();
        destroyAllWindows();
        system("cls");
    }

    return 0;
}
