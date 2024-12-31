#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <thread>
#include <future>
#include <atomic>

struct MarketData {
    std::string symbol;
    double price;
    int volume;
};

// Function to read a chunk of data from the file and parse it
std::vector<MarketData> readMarketDataChunk(const std::string& filename, size_t startPosition, size_t chunkSize) {
    std::vector<MarketData> data;
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    file.seekg(startPosition);
    char* buffer = new char[chunkSize];

    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return data;
    }

    file.read(buffer, chunkSize);

    size_t endPosition = file.tellg();
    file.close();

    std::stringstream ss(buffer, std::ios::in | std::ios::binary, endPosition - startPosition);

    std::string line;
    while (std::getline(ss, line, '\n')) {
        std::stringstream ssLine(line);
        std::string symbol;
        std::string priceStr;
        std::string volumeStr;

        std::getline(ssLine, symbol, ',');
        std::getline(ssLine, priceStr, ',');
        std::getline(ssLine, volumeStr, ',');

        MarketData md;
        md.symbol = symbol;
        md.price = std::stod(priceStr);
        md.volume = std::stoi(volumeStr);
        data.push_back(md);
    }

    delete[] buffer;
    return data;
}

// Function to read the file in parallel
std::vector<MarketData> readMarketDataParallel(const std::string& filename, size_t chunkSize) {
    std::vector<MarketData> result;
    const size_t fileSize = std::filesystem::file_size(filename);
    const int numThreads = std::thread::hardware_concurrency();
    const size_t chunkEndPosition = fileSize / numThreads + 1;

    std::atomic<size_t> nextStartPosition(0);

    std::vector<std::future<std::vector<MarketData>>> futures;

    for (int i = 0; i < numThreads; ++i) {
        const size_t endPosition = std::min(nextStartPosition + chunkEndPosition, fileSize);
        futures.emplace_back(std::async(std::launch::async, readMarketDataChunk, filename, nextStartPosition, endPosition - nextStartPosition));
        nextStartPosition = endPosition;
    }

    for (auto& future : futures) {
        result.insert(result.end(), future.get().begin(), future.get().end());
    }

    return result;
}

int main() {
    const std::string filename = "market_data.csv";
    const size_t chunkSize = 1024 * 1024; // Adjust chunk size as needed (1 MB here)

    std::vector<MarketData> marketData = readMarketDataParallel(filename, chunkSize);

    // Now you have the entire market data vector that you can process
    for (const auto& data : marketData) {
        std::cout << "Symbol: " << data.symbol << ", Price: " << data.price << ", Volume: " << data.volume << std::endl;
    }

    return 0;
}