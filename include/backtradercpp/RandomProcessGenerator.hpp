#pragma once
#include <cstdint>
#include <random>
#include <span>
namespace backtradercpp
{
    template<typename VT = float>
    struct BaseItoProcess
    {
        VT x0;
        VT t0;
        VT dt;

        std::mt19937_64 rng;
        std::normal_distribution<VT> normal_dist;
        std::bernoulli_distribution bernoulli_dist;

        BaseItoProcess(VT x0_ = 0, VT t0_ = 0, VT t1_ = 1, VT dt_ = 1.0 / 250)
            : x0(x0_), t0(t0_), t1(t1_), dt(dt_), normal_dist(VT(0), VT(1)), bernoulli_dist(0.5), t(t0_)
        {
            rng.seed(std::random_device{}());
        }

        void set_random_seed(uint64_t seed)
        {
            rng.seed(seed);
        }

        virtual VT mu(VT t, VT x)
        {
            return VT(0);
        }

        virtual VT sigma(VT t, VT x)
        {
            return VT(0);
        }

        virtual void reset()
        {
            t = t0;
        }
        virtual bool generate(std::span<VT> out)
        {
            //size_t n = out.size();
            //VT t = t0;
            //VT x = x0;
            //out[0] = x;
            if (t > t1) [[unlikely]] {
                return false;
            }
            if (t == t0) [[unlikely]]
            {
                for (size_t k = 0; k < out.size(); ++k)
                {
                    out[k] = x0;
                }
                t += dt;
                return true;
            }

            VT sqrt_dt = std::sqrt(dt);


            //for (size_t k = 0; k < out.size(); k += 1)
            //{
                //auto& x = out[k];
                //VT deltaW = sqrt_dt * normal_dist(rng);
                //VT Sk = bernoulli_dist(rng) ? VT(1) : VT(-1);  // ±1 with probability 1/2

                //VT K1 = dt * mu(t, x) + (deltaW - Sk * sqrt_dt) * sigma(t, x);

                //VT K2 = dt * mu(t + dt, x + K1) + (deltaW + Sk * sqrt_dt) * sigma(t + dt, x + K1);

                //x = x + VT(0.5) * (K1 + K2);


                // out[k] = x;
            //}
            // variance maybe smaller than above.
            for (size_t k = 0; k < out.size(); k += 2) {
                VT deltaW = sqrt_dt * normal_dist(rng);
                out[k] += dt * mu(t, out[k]) + deltaW * sigma(t, out[k]);
                out[k + 1] += dt * mu(t, out[k + 1]) - deltaW * sigma(t, out[k + 1]);
            }

            t += dt;
            return true;
        }

        // next time
        VT t;
        VT t1;
    };

    template<typename VT>
    struct BrownianMotionProcess : BaseItoProcess<VT>
    {
        BrownianMotionProcess(VT mu, VT sigma, VT x0_ = 0, VT t0_ = 0, VT t1_ = 1, VT dt_ = 1.0 / 250)
            : BaseItoProcess<VT>(x0_, t0_, t1_, dt_), mu_(mu), sigma_(sigma)
        {
        }
        VT mu(VT t, VT x) override
        {
            return mu_;
        }

        VT sigma(VT t, VT x) override
        {
            return sigma_;
        }

        VT mu_, sigma_;
    };

    template<typename VT>
    struct GeometricBrownianMotionProcess : BaseItoProcess<VT>
    {
        GeometricBrownianMotionProcess(VT mu, VT sigma, VT x0_ = 0, VT t0_ = 0, VT t1_ = 1, VT dt_ = 1.0 / 250)
            : BaseItoProcess<VT>(x0_, t0_, t1_, dt_), mu_(mu), sigma_(sigma)
        {
        }
        VT mu(VT t, VT x) override
        {
            return mu_ * x;
        }

        VT sigma(VT t, VT x) override
        {
            return sigma_ * x;
        }

        VT mu_, sigma_;
    };
}
