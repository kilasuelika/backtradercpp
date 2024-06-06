#include <iostream>
#include "../../include/backtradercpp/Cerebro.hpp"
#include <Python.h>
#include <numpy/arrayobject.h>
#include "../../include/backtradercpp/DataFeeds.hpp" // 包含 ULDataFrame 的头文件
#include "DataFrame/DataFrame.h"
#include <range/v3/all.hpp>

using namespace backtradercpp;
using namespace std;
using namespace hmdf;
using ULDataFrame = StdDataFrame<unsigned long>;

#define import_array() {if (_import_array() < 0) {PyErr_Print(); PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import"); return ; } }


struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        // Buy assets at 6th day. Index starts from 0, so index 5 means 6th day.
        if (time_index() == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // Buy 10 asset j at the price of latest day(-1) on the broker 0.
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }
    }
};
int main() {
    Cerebro cerebro;
    // non_delimit_date is a function that convert date string like "20200101" to standard format.
    //  0.0005 and 0.001 are commission rate for long and short trading.
    cerebro.add_broker(
        broker::BaseBroker(0.0005, 0.001)
            .set_feed(feeds::CSVTabPriceData("../../example_data/CSVTabular/djia.csv",
                                             feeds::TimeStrConv::non_delimited_date)));
    cerebro.add_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}



ULDataFrame convert_to_ULDataFrame(PyObject* numpy_array) {
    if (!PyArray_Check(numpy_array)) {
        throw std::invalid_argument("Expected a NumPy array");
    }

    PyArrayObject* array = reinterpret_cast<PyArrayObject*>(numpy_array);
    if (PyArray_NDIM(array) != 2) {
        throw std::invalid_argument("Expected a 2-dimensional NumPy array");
    }

    if (PyArray_TYPE(array) != NPY_DOUBLE) {
        throw std::invalid_argument("Expected a NumPy array with double data type");
    }

    npy_intp* shape = PyArray_SHAPE(array);
    size_t rows = shape[0];
    size_t cols = shape[1];

    ULDataFrame df;

    for (size_t j = 0; j < cols; ++j) {
        std::vector<double> column_data(rows);
        for (size_t i = 0; i < rows; ++i) {
            column_data[i] = *reinterpret_cast<double*>(PyArray_GETPTR2(array, i, j));
        }
        std::string column_name = "Column_" + std::to_string(j);
        df.load_column<double>(column_name.c_str(), column_data, nan_policy::dont_pad_with_nans);
    }

    return df;
}

extern "C" {
    __declspec(dllexport) void runBacktrader(PyObject* numpy_array) {
        // Initialize numpy
        if (!Py_IsInitialized()) {
            Py_Initialize();
        }

        try {
            import_array();

            ULDataFrame df = convert_to_ULDataFrame(numpy_array);
            auto priceData = std::make_shared<backtradercpp::feeds::DataFramePriceData>(df);
            auto basePriceData = std::dynamic_pointer_cast<backtradercpp::feeds::BasePriceDataFeed>(priceData);


            Cerebro cerebro;
            cerebro.add_broker(
                broker::BaseBroker(100000, 0.0005, 0.001)
                    .set_df_feed(*basePriceData)
            );
            cerebro.add_strategy(std::make_shared<SimpleStrategy>());
            cerebro.run();
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}
