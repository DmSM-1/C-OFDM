#include <stdlib.h>
#include <vector>
#include <complex>
#include <math.h>
#include <utility>


enum mod_type {
    bpsk = 1, 
    qpsk = 1, 
    qam16 = 4,
    qam64 = 6,
    qam256 = 8
};


using complex_double = std::complex<double>;
using comlex_vector = std::pair<std::vector<double>, std::vector<double>>;


// std::vector<complex_double> psk(std::vector<uint8_t> input, double angle, int deg){
//     double step = M_PI*2/(double)deg;
//     complex_double j(0.0, 1.0);

//     std::vector<complex_double> output(input.size(), 0);

//     for (int i = 0; i < input.size(); i++){
//         output[i] = std::exp(j*(step*complex_double(input[i])+angle));
//     }

//     return output;
// }

complex_double psk(uint8_t input, double angle, int deg){
    double step = M_PI*2/(double)deg;
    complex_double j(0.0, 1.0);

    return std::exp(j*(step*complex_double(input)+angle));
}


class Modulation{
    public:
        mod_type modulation;
        size_t mod_index;
        std::vector<complex_double> constell;

        Modulation(mod_type mod): modulation(mod), constell(int(pow(2.0, static_cast<int>(mod))), 0){
            if (modulation == bpsk){
                mod_index = 1;

                for (int i = 0; i < constell.size(); i++)
                    constell[i] = psk(uint8_t(i), M_PI_4*3, 2); 
            }
        }

        comlex_vector mod(std::vector<uint8_t>& input){
            size_t len = input.size();

            comlex_vector output(
                std::vector<double>(len, 0),
                std::vector<double>(len, 0)
            );

            for (int i = 0; i < len; i++){
                output.first[i] = constell[input[i]].real();
                output.second[i] = constell[input[i]].imag();
            }
            return output;
        }
            
        std::vector<uint8_t> demod(comlex_vector& input){
            size_t len = input.first.size();
            std::vector<uint8_t> output(len, 0);
            std::vector<double> sum(len, 0);

            switch (modulation)
            {
            case bpsk:
                vdAdd(len, input.first.data(), input.second.data(), sum.data());
                for(int i = 0; i < len; ++i)
                    output[i] = uint8_t(sum[i] > 0);
                break;
            
            default:
                break;
            }

            return output;
        }
            // for (int i = 0; i < )
        
};