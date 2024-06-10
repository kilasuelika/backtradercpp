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


namespace py = pybind11;

using namespace hmdf;
using namespace backtradercpp;

using ULDataFrame = StdDataFrame<unsigned long>;

struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        std::cout << "Running SimpleStrategy" << std::endl;

        // 每 20 个时间步长执行一次买入操作
        if (time_index() % 20 == 0) {
            std::cout << "assets in buy method: " << data(0).assets() << std::endl;
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    std::cout << "Buying asset " << j << std::endl;
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }

        // 每 20 个时间步长的第 19 个时间步长执行一次卖出操作
        if (time_index() % 20 == 19) {
            std::cout << "assets in sell method: " << data(0).assets() << std::endl;
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    std::cout << "Selling asset " << j << std::endl;
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

void runBacktrader(py::array_t<double> ohlc_data, 
                   const std::vector<std::string>& date_vector, 
                   const std::vector<std::string>& stock_name_vector, 
                   const std::vector<std::string>& stock_vector) {
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "Running runBacktrader" << std::endl;
    py::gil_scoped_acquire acquire;
    try {
        // Debug: Print the input data
        std::cout << "Printing input vectors for debugging..." << std::endl;
        printVector(date_vector, "Date Vector");
        // printVector(stock_name_vector, "Stock Name Vector");
        // printVector(stock_vector, "Stock Vector");

        // Initialize PriceData
        std::shared_ptr<feeds::PriceData> priceData = std::make_shared<feeds::PriceData>(ohlc_data, date_vector, stock_name_vector, stock_vector,feeds::TimeStrConv::delimited_date);

        std::cout << "PriceData codes size: " << priceData->codes().size() << std::endl;
        for (const auto& code : priceData->codes()) {
            std::cout << "PriceData code: " << code << std::endl;
        }

        // Create and configure Cerebro
        Cerebro cerebro;
        std::cout << "Adding broker..." << std::endl;
        cerebro.add_broker(
            broker::BaseBroker(5000000, 0.0005, 0.001)
                .set_feed(*priceData)
        );
        std::cout << "Adding strategy..." << std::endl;
        cerebro.add_strategy(std::make_shared<SimpleStrategy>());

        std::cout << "Running cerebro..." << std::endl;
        cerebro.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

PYBIND11_MODULE(backtradercpp, m) {
    m.def("runBacktrader", &runBacktrader, "Run the backtrader strategy",
        py::arg("ohlc_data"), py::arg("date_vector"), py::arg("stock_name_vector"), py::arg("stock_vector"));
}