#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <cmath>
#include <unordered_map>

namespace fs = std::filesystem;

struct City {
    int index;
    double x;
    double y;
};

double euclideanDistance(const City& a, const City& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return sqrt(dx*dx + dy*dy);
}

std::vector<City> parseTSPFile(const std::string& filename) {
    std::vector<City> cities;
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Cannot open file " << filename << "\n";
        return cities;
    }
    std::string line;
    bool node_coord_section = false;
    while (std::getline(infile, line)) {
        if (line.find("NODE_COORD_SECTION") != std::string::npos) {
            node_coord_section = true;
            continue;
        }
        if (line.find("EOF") != std::string::npos) {
            break;
        }
        if (node_coord_section) {
            std::istringstream iss(line);
            int index;
            double x, y;
            if (!(iss >> index >> x >> y)) {
                continue;
            }
            City city = {index, x, y};
            cities.push_back(city);
        }
    }
    return cities;
}

double totalDistance(const std::vector<int>& tour, const std::vector<City>& cities) {
    double dist = 0.0;
    int n = tour.size();
    for (int i = 0; i < n; ++i) {
        const City& a = cities[tour[i]];
        const City& b = cities[tour[(i+1)%n]];
        dist += euclideanDistance(a, b);
    }
    return dist;
}

void simulatedAnnealing(std::vector<int>& tour, const std::vector<City>& cities) {
    double T = 10000.0;
    double coolingRate = 0.9999;
    double absoluteTemperature = 0.00001;
    int n = tour.size();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, n-1);
    std::uniform_real_distribution<> realDist(0.0, 1.0);
    double currentDistance = totalDistance(tour, cities);
    std::vector<int> bestTour = tour;
    double bestDistance = currentDistance;

    while (T > absoluteTemperature) {
        int i = dist(gen);
        int j = dist(gen);
        if (i == j) continue;
        if (i > j) std::swap(i, j);

        std::reverse(tour.begin() + i, tour.begin() + j);

        double newDistance = totalDistance(tour, cities);
        double delta = newDistance - currentDistance;

        if (delta < 0 || exp(-delta / T) > realDist(gen)) {
            currentDistance = newDistance;
            if (currentDistance < bestDistance) {
                bestDistance = currentDistance;
                bestTour = tour;
            }
        } else {
            std::reverse(tour.begin() + i, tour.begin() + j);
        }
        T *= coolingRate;
    }
    tour = bestTour;
}

// Function to read solutions.txt and return a map of filenames to optimal solutions
std::unordered_map<std::string, double> readSolutions(const std::string& solutionsFile) {
    std::unordered_map<std::string, double> solutionsMap;
    std::ifstream infile(solutionsFile);
    if (!infile) {
        std::cerr << "Cannot open solutions file " << solutionsFile << "\n";
        return solutionsMap;
    }
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string filename;
        double optimalValue;
        char colon;
        if (!(iss >> filename >> colon >> optimalValue)) {
            continue;
        }
        // Remove any extension from the filename
        size_t dotPos = filename.find('.');
        if (dotPos != std::string::npos) {
            filename = filename.substr(0, dotPos);
        }
        solutionsMap[filename] = optimalValue;
    }
    return solutionsMap;
}

std::string getExecutablePath() {
    return std::filesystem::current_path().string();
}

int main() {
    std::string execPath = getExecutablePath();
    std::string datasetFolder = execPath + "/../dataset/";
    std::vector<std::string> tspFiles;
    for (const auto& entry : fs::directory_iterator(datasetFolder)) {
        if (entry.path().extension() == ".tsp") {
            tspFiles.push_back(entry.path().string());
        }
    }

    if (tspFiles.empty()) {
        std::cout << "No .tsp files found in dataset folder.\n";
        return 1;
    }

    std::cout << "Available .tsp files:\n";
    for (size_t i = 0; i < tspFiles.size(); ++i) {
        std::cout << i+1 << ": " << tspFiles[i] << "\n";
    }
    std::cout << "Select a file by entering its number: ";
    size_t choice;
    std::cin >> choice;
    if (choice < 1 || choice > tspFiles.size()) {
        std::cout << "Invalid selection.\n";
        return 1;
    }

    std::string selectedFile = tspFiles[choice - 1];
    std::cout << "You selected: " << selectedFile << "\n";

    std::string selectedFilename = fs::path(selectedFile).stem().string();

    std::vector<City> cities = parseTSPFile(selectedFile);
    if (cities.empty()) {
        std::cerr << "Failed to parse the selected file.\n";
        return 1;
    }

    std::vector<int> tour;
    for (size_t i = 0; i < cities.size(); ++i) {
        tour.push_back(i);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(tour.begin(), tour.end(), g);

    std::cout << "Initial distance: " << totalDistance(tour, cities) << "\n";

    simulatedAnnealing(tour, cities);

    double finalDistance = totalDistance(tour, cities);
    std::cout << "Final distance: " << finalDistance << "\n";
    std::cout << "Tour: ";
    for (int idx : tour) {
        std::cout << cities[idx].index << " ";
    }
    std::cout << "\n";

    // Read solutions.txt
    std::string solutionsFile = datasetFolder + "solutions.txt";
    std::unordered_map<std::string, double> solutionsMap = readSolutions(solutionsFile);

    // Output the correct answer
    if (solutionsMap.find(selectedFilename) != solutionsMap.end()) {
        double correctAnswer = solutionsMap[selectedFilename];
        std::cout << "Correct Answer: " << correctAnswer << "\n";
    } else {
        std::cout << "Correct Answer: Not available in solutions.txt\n";
    }

    return 0;
}
