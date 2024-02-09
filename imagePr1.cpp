#include <iostream>
#include <fstream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <windows.h>
#include <commdlg.h>
#include "lodepng.h"

const int RESIZED_IMAGE_WIDTH = 500;
const int RESIZED_IMAGE_HEIGHT = 500;

std::wstring openFileDialog() {
    OPENFILENAMEW ofn;
    wchar_t fileName[MAX_PATH] = L"";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = L"All Files\0*.*\0\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"";

    if (GetOpenFileNameW(&ofn))
        return std::wstring(fileName);
    else
        return L"";
}

std::string ws2s(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void convertToGrayscale(const cv::Mat& inputImage, const std::string& filename) {
//
}


void saveColorImageWithLodePNG(const cv::Mat& image, const std::string& filename) {
    // Проверяем, является ли изображение цветным
    if (image.channels() < 3) {
        std::cerr << "Image is not in a supported color format." << std::endl;
        return;
    }

    cv::Mat rgbImage;
    // Преобразуем изображение из BGR (OpenCV) в RGB (LodePNG)
    cv::cvtColor(image, rgbImage, cv::COLOR_BGR2RGB);

    // Подготавливаем данные для LodePNG
    std::vector<unsigned char> image_data(rgbImage.data, rgbImage.data + (rgbImage.cols * rgbImage.rows * rgbImage.channels()));
    unsigned width = rgbImage.cols;
    unsigned height = rgbImage.rows;

    // Сохраняем изображение в формате PNG
    unsigned error = lodepng::encode(filename, image_data, width, height, LCT_RGB);
    if (error) {
        std::cerr << "Error " << error << ": " << lodepng_error_text(error) << std::endl;
    }
}

void convertToPngUsingLodePNG(const cv::Mat& image, const std::string& filename) {
    if (image.empty()) {
        std::cerr << "Image is empty." << std::endl;
        return;
    }

    cv::Mat colorImage;
    // Проверяем, является ли изображение одноканальным
    if (image.channels() == 1) {
        // Преобразуем из одноканального в трехканальное (BGR)
        cv::cvtColor(image, colorImage, cv::COLOR_GRAY2BGR);
    }
    else {
        colorImage = image.clone();
    }

    // Подготавливаем данные для LodePNG
    std::vector<unsigned char> image_data(colorImage.data, colorImage.data + (colorImage.cols * colorImage.rows * colorImage.channels()));
    unsigned width = colorImage.cols;
    unsigned height = colorImage.rows;

    // Сохраняем изображение в формате PNG
    unsigned error = lodepng::encode(filename, image_data, width, height, LCT_RGB);
    if (error) {
        std::cerr << "Error " << error << ": " << lodepng_error_text(error) << std::endl;
    }
}

cv::Mat readImageFromFile(const std::string& imagePath) {
    // Проверяем расширение файла
    if (imagePath.substr(imagePath.find_last_of(".") + 1) == "png") {
        // Используем LodePNG для чтения PNG-изображения
        std::vector<unsigned char> image; // Сырые пиксели
        unsigned width, height;
        unsigned error = lodepng::decode(image, width, height, imagePath);

        if (error) {
            std::cerr << "Ошибка при чтении изображения: " << lodepng_error_text(error) << std::endl;
            exit(-1);
        }

        // Преобразуем сырые пиксели в cv::Mat
        cv::Mat matImage(height, width, CV_8UC4, image.data());
        return matImage.clone(); // Возвращаем клонированную копию, чтобы избежать проблем с памятью
    }
    else {
        // Для без форматных изображений используем нашу реализацию
        std::ifstream file(imagePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Не удалось открыть файл" << std::endl;
            exit(-1);
        }

        uint16_t width, height;
        file.read(reinterpret_cast<char*>(&height), sizeof(height));
        file.read(reinterpret_cast<char*>(&width), sizeof(width));

        cv::Mat image(height, width, CV_16UC1);
        std::vector<uint16_t> buffer(width * height);
        file.read(reinterpret_cast<char*>(buffer.data()), width * height * sizeof(uint16_t));

        memcpy(image.data, buffer.data(), buffer.size() * sizeof(uint16_t));
        file.close();

        return image;
    }
}


int main() {
    SetConsoleOutputCP(1251);
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

    while (true) {
        int choice = 0;
        std::cout << "Меню:\n1. Открыть файл для просмотра\n2. Конвертировать файл в .png\n3. Конвертировать файл в полутоновое безформатное\nВыберите действие: ";
        std::cin >> choice;

        std::wstring imagePathW = openFileDialog();

        if (imagePathW.empty()) {
            std::cerr << "Файл не был выбран" << std::endl;
            continue;
        }

        std::string imagePath = ws2s(imagePathW);

        cv::Mat image = readImageFromFile(imagePath);

        if (image.channels() == 4) { // Проверка на наличие альфа-канала
            cv::cvtColor(image, image, cv::COLOR_RGBA2BGRA); // Конвертация из RGBA в BGRA
        }
        else if (image.channels() == 3) {
            cv::cvtColor(image, image, cv::COLOR_RGB2BGR); // Конвертация из RGB в BGR
        }


        cv::Mat displayImage;
        cv::normalize(image, displayImage, 0, 255, cv::NORM_MINMAX);
        displayImage.convertTo(displayImage, CV_8UC1);

        switch (choice) {
        case 1: {
            cv::Mat resizedImage;
            cv::resize(displayImage, resizedImage, cv::Size(RESIZED_IMAGE_WIDTH, RESIZED_IMAGE_HEIGHT));
            cv::namedWindow("Image", cv::WINDOW_AUTOSIZE);
            cv::imshow("Image", resizedImage);
            cv::waitKey(0);
            break;
        }
        case 2: {
            std::string outputPath = imagePath.substr(0, imagePath.find_last_of(".")) + "_converted.png";        
                // Определяем формат изображения и используем соответствующую функцию для сохранения
                if (displayImage.channels() == 1) {
                    // Изображение одноканальное, используем convertToPngUsingLodePNG
                    convertToPngUsingLodePNG(displayImage, outputPath);
                }
                else {
                    // Изображение многоканальное, используем saveColorImageWithLodePNG
                    saveColorImageWithLodePNG(displayImage, outputPath);
                }
            break;
        }
        case 3: {
            std::string outputPath = imagePath.substr(0, imagePath.find_last_of(".")) + "_converted";
            // Определяем формат изображения и используем соответствующую функцию для сохранения
            std::cout << "Конвертация изображения..." << std::endl;
            convertToGrayscale(displayImage, outputPath);
            std::cout << "Конвертация завершена. Результат сохранен в: " << outputPath << std::endl;
            break;
        }
        default:
            std::cerr << "Неверный выбор. Пожалуйста, выберите 1 или 2." << std::endl;
            break;
        }
        //system("cls");
    }

    return 0;
}
