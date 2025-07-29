#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <cmath>

#include <windows.h>

#pragma pack(push, 1)
#pragma pack(pop)

struct Pixel {
    uint8_t r, g, b, a = 255;
};

class BmpImage {
public:
    int width = 0, height = 0, bitCount = 0;
    std::vector<std::vector<Pixel>> pixels;

    bool load(const std::string& filename);
    void printToConsole() const;
    void drawLine(int x1, int y1, int x2, int y2);
    void drawX();
    bool save(const std::string& filename) const;

private:
    bool isBlack(const Pixel& p) const { return p.r == 0 && p.g == 0 && p.b == 0; }
    bool isWhite(const Pixel& p) const { return p.r == 255 && p.g == 255 && p.b == 255; }
};

bool BmpImage::load(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return false;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    in.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    in.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.bfType != 0x4D42 || infoHeader.biCompression != 0) {
        std::cerr << "Not a valid uncompressed BMP file.\n";
        return false;
    }

    width = infoHeader.biWidth;
    height = std::abs(infoHeader.biHeight);
    bitCount = infoHeader.biBitCount;

    if (bitCount != 24 && bitCount != 32) {
        std::cerr << "Unsupported bit count: " << bitCount << "\n";
        return false;
    }

    in.seekg(fileHeader.bfOffBits, std::ios::beg);
    size_t rowSize = ((width * bitCount + 31) / 32) * 4;
    std::vector<uint8_t> row(rowSize);
    pixels.resize(height, std::vector<Pixel>(width));

    for (int y = height - 1; y >= 0; --y) {
        in.read(reinterpret_cast<char*>(row.data()), rowSize);
        for (int x = 0; x < width; ++x) {
            Pixel& p = pixels[y][x];
            p.b = row[x * (bitCount / 8) + 0];
            p.g = row[x * (bitCount / 8) + 1];
            p.r = row[x * (bitCount / 8) + 2];
            if (bitCount == 32) {
                p.a = row[x * 4 + 3];
            }
            if (!isBlack(p) && !isWhite(p)) {
                std::cerr << "Image contains colors other than black/white at (" << x << "," << y << ")\n";
                return false;
            }
        }
    }

    return true;
}

void BmpImage::printToConsole() const {
    for (const auto& row : pixels) {
        for (const auto& p : row) {
            std::cout << (isBlack(p) ? '#' : '.');
        }
        std::cout << '\n';
    }
}

void BmpImage::drawLine(int x1, int y1, int x2, int y2) {
    int dx = std::abs(x2 - x1);
    int dy = -std::abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height) {
            pixels[y1][x1] = { 0, 0, 0, 255 }; // Чёрный пиксель
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void BmpImage::drawX() {
    drawLine(0, 0, width - 1, height - 1);
    drawLine(width - 1, 0, 0, height - 1);
}

bool BmpImage::save(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open output file.\n";
        return false;
    }

    BITMAPFILEHEADER fileHeader{};
    BITMAPINFOHEADER infoHeader{};

    size_t rowSize = ((width * bitCount + 31) / 32) * 4;
    size_t imageSize = rowSize * height;

    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(infoHeader);
    fileHeader.bfSize = fileHeader.bfOffBits + imageSize;

    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = bitCount;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = imageSize;

    out.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    out.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    std::vector<uint8_t> row(rowSize);

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const Pixel& p = pixels[y][x];
            row[x * (bitCount / 8) + 0] = p.b;
            row[x * (bitCount / 8) + 1] = p.g;
            row[x * (bitCount / 8) + 2] = p.r;
            if (bitCount == 32) {
                row[x * 4 + 3] = p.a;
            }
        }
        out.write(reinterpret_cast<char*>(row.data()), rowSize);
    }

    return true;
}


int main() {
    std::string inputFile, outputFile;
    BmpImage img;

    std::cout << ">> Enter input BMP file name: ";
    std::cin >> inputFile;

    if (!img.load(inputFile)) {
        std::cerr << "Failed to load BMP file.\n";
        return 1;
    }

    std::cout << "\nOriginal image:\n";
    img.printToConsole();

    img.drawX();

    std::cout << "\nImage with X drawn:\n";
    img.printToConsole();

    std::cout << "\n>> Enter output BMP file name: ";
    std::cin >> outputFile;

    if (!img.save(outputFile)) {
        std::cerr << "Failed to save BMP file.\n";
        return 1;
    }

    std::cout << "BMP saved successfully to " << outputFile << "\n";
    return 0;
}
