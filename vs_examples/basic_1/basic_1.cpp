#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "../../include/backtradercpp/Cerebro.hpp"
#include "../../include/backtradercpp/DataFeeds.hpp"
#include <DataFrame/DataFrame.h>
#include <pybind11/stl.h>
#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>
#include <windows.h>
#include <thread>
#include <chrono>
#include <memory>
#include <exception>
#include <nlohmann/json.hpp>

// 使用 nlohmann/json 库
using json = nlohmann::json;

namespace py = pybind11;

using namespace hmdf;
using namespace backtradercpp;

using ULDataFrame = StdDataFrame<unsigned long>;

struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        // std::cout << "Running SimpleStrategy" << std::endl;

        // 每 20 个时间步长执行一次买入操作
        if (time_index() % 20 == 0) {
            // std::cout << "assets in buy method: " << data(0).assets() << std::endl;
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // std::cout << "Buying asset " << j << std::endl;
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }

        // 每 20 个时间步长的第 19 个时间步长执行一次卖出操作
        if (time_index() % 20 == 19) {
            // std::cout << "assets in sell method: " << data(0).assets() << std::endl;
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // std::cout << "Selling asset " << j << std::endl;
                    close(0, j, data(0).open(-1, j));
                }
            }
        }
    }
};


void printVector(const std::vector<std::string>& vec, const std::string& label) {
    std::cout << label << ": ";
    for (const auto& str : vec) {
        std::cout << str << " ";
    }
    std::cout << std::endl;
}

struct CustomObject {
    py::array_t<double> array;
    std::vector<std::string> vec1;
    std::vector<std::string> vec2;
    std::vector<std::string> vec3;
};

struct DataObject {
    std::vector<std::vector<double>> ohlc_data;
    std::vector<std::string> date_vector;
    std::vector<std::string> stock_name_vector;
    std::vector<std::string> stock_vector;
};

// 将 DataObject 序列化为 JSON 字符串
std::string serializeData(const DataObject& data) {
    json j;
    j["ohlc_data"] = data.ohlc_data;
    j["date_vector"] = data.date_vector;
    j["stock_name_vector"] = data.stock_name_vector;
    j["stock_vector"] = data.stock_vector;
    return j.dump(); // 返回 JSON 字符串
}

// 从 JSON 字符串反序列化为 DataObject
DataObject deserializeData(const std::string& dataStr) {
    json j = json::parse(dataStr);
    DataObject data;
    data.ohlc_data = j["ohlc_data"].get<std::vector<std::vector<double>>>();
    data.date_vector = j["date_vector"].get<std::vector<std::string>>();
    data.stock_name_vector = j["stock_name_vector"].get<std::vector<std::string>>();
    data.stock_vector = j["stock_vector"].get<std::vector<std::string>>();
    return data;
}


// Helper function to convert std::string to std::wstring
std::wstring stringToWstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// void runBacktrader(py::array_t<double> ohlc_data, 
//                    const std::vector<std::string>& date_vector, 
//                    const std::vector<std::string>& stock_name_vector, 
//                    const std::vector<std::string>& stock_vector) {
//     SetConsoleOutputCP(CP_UTF8);
//     std::cout << "Running runBacktrader" << std::endl;

//     try {

//         // Initialize PriceData
//         std::shared_ptr<feeds::PriceData> priceData = std::make_shared<feeds::PriceData>(ohlc_data, date_vector, stock_name_vector, stock_vector, feeds::TimeStrConv::delimited_date);

//         // Create and configure Cerebro
//         Cerebro cerebro;
//         std::cout << "Adding broker..." << std::endl;
//         cerebro.add_broker(
//             broker::BaseBroker(5000000, 0.0005, 0.001)
//                 .set_feed(*priceData)
//         );
//         cerebro.add_strategy(std::make_shared<SimpleStrategy>());
//         cerebro.run();
//     } catch (const std::exception &e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }
// }

void runBacktrader(const std::vector<std::vector<double>>& ohlc_data, 
                   const std::vector<std::string>& date_vector, 
                   const std::vector<std::string>& stock_name_vector, 
                   const std::vector<std::string>& stock_vector) {
    SetConsoleOutputCP(CP_UTF8);
    // std::cout << "Running runBacktrader" << std::endl;

    try {
        // Initialize PriceData
        std::shared_ptr<feeds::PriceData> priceData = std::make_shared<feeds::PriceData>(ohlc_data, date_vector, stock_name_vector, stock_vector, feeds::TimeStrConv::delimited_date);

        // Create and configure Cerebro
        Cerebro cerebro;
        // std::cout << "Adding broker..." << std::endl;
        cerebro.add_broker(
            broker::BaseBroker(5000000, 0.0005, 0.001)
                .set_feed(*priceData)
        );
        cerebro.add_strategy(std::make_shared<SimpleStrategy>());
        cerebro.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}


void runAll(const std::vector<CustomObject>& data_list) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    std::cout << "Running runAll" << std::endl;

    // Ensure GIL is acquired in this thread to convert Python objects to C++ objects
    py::gil_scoped_acquire acquire;

    for (const auto& data : data_list) {
        try {
            // Convert py::array_t<double> to std::vector<std::vector<double>>
            py::buffer_info buf = data.array.request();
            std::vector<std::vector<double>> ohlc_data(buf.shape[0], std::vector<double>(buf.shape[1]));
            double* ptr = static_cast<double*>(buf.ptr);
            for (size_t i = 0; i < buf.shape[0]; ++i) {
                for (size_t j = 0; j < buf.shape[1]; ++j) {
                    ohlc_data[i][j] = ptr[i * buf.shape[1] + j];
                }
            }
            DataObject mydata = {ohlc_data, data.vec1, data.vec2, data.vec3};

            // Create a new thread to runBacktrader with C++ objects
            threads.emplace_back([mydata]() {
                try {
                    // std::cout << "Starting thread for stock code: " << mydata.stock_vector[0] << std::endl;
                    runBacktrader(mydata.ohlc_data, mydata.date_vector, mydata.stock_name_vector, mydata.stock_vector);
                    // std::cout << "Finished thread for stock code: " << mydata.stock_vector[0] << std::endl;
                } catch (const std::exception &e) {
                    std::cerr << "Thread error: " << e.what() << std::endl;
                }
            });
        } catch (const std::exception &e) {
            std::cerr << "Error in converting data: " << e.what() << std::endl;
        }
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "C++ code elapsed time : " << elapsed.count() << " seconds.\n";
}



// void runBacktrader(const std::vector<std::vector<double>>& ohlc_data, 
//                    const std::vector<std::string>& date_vector, 
//                    const std::vector<std::string>& stock_name_vector, 
//                    const std::vector<std::string>& stock_vector) {
//     SetConsoleOutputCP(CP_UTF8);

//     try {
//         std::shared_ptr<feeds::PriceData> priceData = std::make_shared<feeds::PriceData>(ohlc_data, date_vector, stock_name_vector, stock_vector, feeds::TimeStrConv::delimited_date);
//         Cerebro cerebro;
//         cerebro.add_broker(broker::BaseBroker(5000000, 0.0005, 0.001).set_feed(*priceData));
//         cerebro.add_strategy(std::make_shared<SimpleStrategy>());
//         cerebro.run();
//     } catch (const std::exception &e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }
// }


// void createProcessAndRun(const std::wstring& commandLine) {
//     STARTUPINFOW si;
//     PROCESS_INFORMATION pi;

//     ZeroMemory(&si, sizeof(si));
//     si.cb = sizeof(si);
//     ZeroMemory(&pi, sizeof(pi));

//     if (!CreateProcessW(
//         NULL,                   // No module name (use command line)
//         const_cast<wchar_t*>(commandLine.c_str()), // Command line
//         NULL,                   // Process handle not inheritable
//         NULL,                   // Thread handle not inheritable
//         FALSE,                  // Set handle inheritance to FALSE
//         0,                      // No creation flags
//         NULL,                   // Use parent's environment block
//         NULL,                   // Use parent's starting directory 
//         &si,                    // Pointer to STARTUPINFO structure
//         &pi)                    // Pointer to PROCESS_INFORMATION structure
//     ) {
//         std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
//         return;
//     }

//     // Wait until child process exits
//     WaitForSingleObject(pi.hProcess, INFINITE);

//     // Close process and thread handles
//     CloseHandle(pi.hProcess);
//     CloseHandle(pi.hThread);
// }


// void runAll(const std::vector<CustomObject>& data_list) {
//     auto start = std::chrono::high_resolution_clock::now();
//     std::vector<HANDLE> processHandles;

//     std::cout << "Running runAll" << std::endl;

//     for (const auto& data : data_list) {
//         try {
//             py::buffer_info buf = data.array.request();
//             std::vector<std::vector<double>> ohlc_data(buf.shape[0], std::vector<double>(buf.shape[1]));
//             double* ptr = static_cast<double*>(buf.ptr);
//             for (size_t i = 0; i < buf.shape[0]; ++i) {
//                 for (size_t j = 0; j < buf.shape[1]; ++j) {
//                     ohlc_data[i][j] = ptr[i * buf.shape[1] + j];
//                 }
//             }
//             DataObject mydata = {ohlc_data, data.vec1, data.vec2, data.vec3};

//             // Create a command line to runBacktrader in a separate process
//             std::string serializedData = serializeData(mydata);
//             std::wstring commandLine = L"runBacktraderProcess " + stringToWstring(serializedData);

//             // Create a new process
//             createProcessAndRun(commandLine);

//         } catch (const std::exception &e) {
//             std::cerr << "Error in converting data: " << e.what() << std::endl;
//         }
//     }

//     // Wait for all processes to finish
//     for (auto& handle : processHandles) {
//         WaitForSingleObject(handle, INFINITE);
//         CloseHandle(handle);
//     }

//     auto end = std::chrono::high_resolution_clock::now();
//     std::chrono::duration<double> elapsed = end - start;
//     std::cout << "C++ code elapsed time : " << elapsed.count() << " seconds.\n";
// }

void checkCoreUsage() {
    DWORD_PTR processAffinityMask;
    DWORD_PTR systemAffinityMask;

    if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask)) {
        std::cout << "Process affinity mask: " << processAffinityMask << std::endl;
        std::cout << "System affinity mask: " << systemAffinityMask << std::endl;

        // Count the number of cores being used
        int coreCount = 0;
        while (processAffinityMask) {
            coreCount += processAffinityMask & 1;
            processAffinityMask >>= 1;
        }

        std::cout << "Number of cores being used: " << coreCount << std::endl;
    } else {
        std::cerr << "Failed to get process affinity mask (" << GetLastError() << ").\n";
    }
}

PYBIND11_MODULE(backtradercpp, m) {
    py::class_<CustomObject>(m, "CustomObject")
        .def(py::init<>())
        .def_readwrite("array", &CustomObject::array)
        .def_readwrite("vec1", &CustomObject::vec1)
        .def_readwrite("vec2", &CustomObject::vec2)
        .def_readwrite("vec3", &CustomObject::vec3);

    m.def("runBacktrader", &runBacktrader, "Run the backtrader strategy",
          py::arg("ohlc_data"), py::arg("date_vector"), py::arg("stock_name_vector"), py::arg("stock_vector"));
    m.def("runAll", &runAll, "Process a list of CustomObject");
    m.def("checkCoreUsage", &checkCoreUsage, "Check the number of cores being used");
}