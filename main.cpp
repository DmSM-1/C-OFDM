#include <iostream>
#include "frame.hpp"

int main(){
    std::vector<size_t> frame_size = {5}; 
    std::string src_buf = "One Two Three Four Five Six Seven";

    // for (int i = 0; i < 1024; ++i)
    //     src_buf[i] = uint8_t(std::rand()%0xFF);

    OFDM::Frame_Fabric<OFDM::Data_Frame, uint8_t> fabric(20, 5, frame_size, 10);

    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(src_buf.size());
    std::memcpy(data.get(), src_buf.data(), src_buf.size());

    fabric.gen_frames(std::move(data));
    // frame.dump();
    // std::cout<<frame.read();
    auto frame = fabric.get_frame();
    frame->dump();
}