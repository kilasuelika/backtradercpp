// #include <pybind11/pybind11.h>
// #include <pybind11/numpy.h>
// #include "../../include/backtradercpp/Cerebro.hpp"
// #include <DataFrame/DataFrame.h>


// using namespace backtradercpp;
// using namespace hmdf;


// namespace py = pybind11;

// struct SimpleStrategy : strategy::GenericStrategy {
//     void run() override {
//         // Buy assets at 6th day. Index starts from 0, so index 5 means 6th day.
//         if (time_index() == 5) {
//             for (int j = 0; j < data(0).assets(); ++j) {
//                 if (data(0).valid(-1, j)) {
//                     // Buy 10 asset j at the price of latest day(-1) on the broker 0.
//                     buy(0, j, data(0).open(-1, j), 10);
//                 }
//             }
//         }
//     }
// };

// ULDataFrame convert_to_ULDataFrame(py::array_t<double> numpy_array) {
//     if (numpy_array.ndim() != 2) {
//         throw std::invalid_argument("Expected a 2-dimensional NumPy array");
//     }

//     py::buffer_info buf_info = numpy_array.request();
//     size_t rows = buf_info.shape[0];
//     size_t cols = buf_info.shape[1];

//     ULDataFrame df;

//     for (size_t j = 0; j < cols; ++j) {
//         std::vector<double> column_data(rows);
//         for (size_t i = 0; i < rows; ++i) {
//             double* ptr = reinterpret_cast<double*>(buf_info.ptr);
//             column_data[i] = *(ptr + i * buf_info.strides[0] / sizeof(double) + j * buf_info.strides[1] / sizeof(double));
//         }
//         std::string column_name = "Column_" + std::to_string(j);
//         df.load_column<double>(column_name.c_str(), column_data);
//     }

//     return df;
// }






#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "../../include/backtradercpp/Cerebro.hpp"
#include <DataFrame/DataFrame.h>
#include <pybind11/stl.h>
#include <iostream>

namespace py = pybind11;

using namespace hmdf;
using namespace backtradercpp;

using ULDataFrame = StdDataFrame<unsigned long>;

struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        std::cout << "Running SimpleStrategy" << std::endl;
        if (time_index() == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    std::cout << "Buying asset " << j << std::endl;
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }
    }
};

ULDataFrame convert_to_ULDataFrame(py::array_t<double> numpy_array, const std::string& test) {
    std::cout << "Converting NumPy array to ULDataFrame" << std::endl;
    auto buf = numpy_array.request();
    if (buf.ndim != 2) {
        throw std::invalid_argument("Expected a 2-dimensional NumPy array");
    }

    size_t rows = buf.shape[0];
    size_t cols = buf.shape[1];

    ULDataFrame df;
    double *ptr = static_cast<double *>(buf.ptr);

    for (size_t j = 0; j < cols; ++j) {
        std::vector<double> column_data(rows);
        for (size_t i = 0; i < rows; ++i) {
            column_data[i] = ptr[i * cols + j];
        }
        std::string column_name = "Column_" + std::to_string(j);
        df.load_column<double>(column_name.c_str(), column_data);
    }

    // if (date_array.size() != rows) {
    //     throw std::invalid_argument("Expected date array to have same size as first dimension of NumPy array");
    // }
    // std::vector<std::string> date_vector(date_array.begin(), date_array.end());
    // df.load_column<std::string>("date", date_vector);

    return df;
}



extern "C" {
    __declspec(dllexport) void runBacktrader(py::array_t<double> numpy_array,const std::string& test) {
    std::cout << "Running runBacktrader" << std::endl;
    std::cout << "test: " << test << std::endl;
    py::gil_scoped_acquire acquire;
    try {
        std::cout << "convert_to_ULDataFrame start.." << std::endl;
        ULDataFrame df = convert_to_ULDataFrame(numpy_array, test);
        std::cout << "convert_to_ULDataFrame success.." << std::endl;

        auto priceData = std::make_shared<backtradercpp::feeds::DataFramePriceData>(df);
        auto basePriceData = std::dynamic_pointer_cast<backtradercpp::feeds::BasePriceDataFeed>(priceData);

        Cerebro cerebro;
        std::cout << "add broker.." << std::endl;
        cerebro.add_broker(
            broker::BaseBroker(100000, 0.0005, 0.001)
                .set_df_feed(*basePriceData)
        );
        std::cout << "add strategy.." << std::endl;
        cerebro.add_strategy(std::make_shared<SimpleStrategy>());
        std::cout << "cerebro run.." << std::endl;
        cerebro.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

PYBIND11_MODULE(backtradercpp, m) {
    m.def("runBacktrader", &runBacktrader, "Run the backtrader strategy",
        py::arg("numpy_array"), py::arg("date_array"));
}
}
