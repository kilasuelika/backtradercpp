#pragma once

#include "DataFeeds.hpp"

namespace backtradercpp {
    namespace feeds {
        template<typename RandomProcessType>
        class RandomProcessDataImpl : public BasePriceDataImpl {
        public:
            RandomProcessDataImpl(int n_path, const RandomProcessType& proc) :BasePriceDataImpl(n_path), random_process(proc) {

            }

            bool read() override {
                if (random_process.t == random_process.t0) [[unlikely]] {
                    next_.time = boost::date_time::min_date_time;
                }
                else
                {
                    next_.time = next_.time + boost::gregorian::days(1);
                }
                auto base_t = random_process.t;
                auto r = random_process.generate(next_.data.close);
                next_.data.open = next_.data.close;
                next_.data.high = next_.data.high;
                next_.data.low = next_.data.low;

                if (!dump_file_name.empty() && r)
                {
                    dump_file << base_t << ",";
                    for (int i = 0; i < assets_; ++i)
                    {
                        dump_file << next_.data.close[i];
                        if (i < assets_ - 1) {
                            dump_file << ",";
                        }
                    }
                    dump_file << std::endl;
                }
                return r;
            }

            void reset() override
            {
                next_.time = boost::date_time::min_date_time;
                next_.volume.setConstant(100000);
                next_.valid.setConstant(true);

                is_degenerated = true;
                random_process.reset();
            }

            void set_dump_csv(const std::string& csv_file)
            {
                dump_file_name = csv_file;
                if (!csv_file.empty())
                {
                    dump_file.open(csv_file);
                    dump_file << "time" << ",";
                    for (int i = 0; i < assets_; ++i)
                    {
                        if (i < assets_ - 1) {
                            dump_file << codes_[i] << ",";
                        }
                        else
                        {
                            dump_file << codes_[i];
                        }
                    }
                    dump_file << std::endl;
                }


            }
        protected:
            RandomProcessType random_process;
            std::string dump_file_name;
            std::ofstream dump_file;
        };
        template<typename RandomProcessType>
        struct RandomProcessData : BasePriceDataFeed {

            //RandomProcessData(int n_path, const RandomProcessDataImpl<RandomProcessType>& proc)
            //    : BasePriceDataFeed(std::make_shared<RandomProcessDataImpl<RandomProcessType>>(n_path)) {
            //}
            RandomProcessData(int n_path, const RandomProcessType& proc)
                : BasePriceDataFeed(std::make_shared<RandomProcessDataImpl<RandomProcessType>>(n_path, proc)) {
            }

            explicit RandomProcessData(std::shared_ptr<RandomProcessDataImpl<RandomProcessType>> sp) : BasePriceDataFeed(std::move(sp)) {
            }

            RandomProcessData& set_dump_csv(const std::string& csv_file)
            {
                std::dynamic_pointer_cast<RandomProcessDataImpl<RandomProcessType>>(sp)->set_dump_csv(csv_file);
                return *this;
            }
            // BasePriceDataFeed clone() { return RandomProcessData(std::make_shared<RandomProcessDataImpl<RandomProcessType>>(*sp)); }
        };
    }
}