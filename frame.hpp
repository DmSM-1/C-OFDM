#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <memory>
#include <queue>

namespace OFDM{


    class Frame{

    protected:
        std::string f_type;
        int seq_num;
        std::vector<size_t> f_size;
        std::unique_ptr<uint8_t[]> data;

    public:

        Frame(std::string f_type, int seq_num, std::vector<size_t> f_size)
            :f_type(f_type), seq_num(seq_num), f_size(f_size){}

        virtual ~Frame() = default;
        virtual std::unique_ptr<uint8_t[]> release() = 0;
        virtual void* read() = 0;
        virtual void dump() = 0;

        int& get_seq_num(){
            return seq_num;
        }

        std::string& get_type(){
            return f_type;
        }

        std::vector<size_t>& get_size(){
            return f_size;
        }

    };


    class Data_Frame: public Frame{

    private:
        std::unique_ptr<uint8_t[]> data;
        std::vector<size_t> size;

    public:
        Data_Frame(int seq_num, std::vector<size_t>& size, void* src_data, size_t readable)
            :Frame("DATAFRAME", seq_num, size), data(new uint8_t[size[0]]()), size(size){

            memcpy(data.get(), src_data, std::min(readable, size[0]));
        }
        
        std::unique_ptr<uint8_t[]> release(){
            return std::move(data);
        }

        void* read(){
            return data.get();
        }

        void dump(){
            std::cout<<"Frame DATAFRAME\n";
            for (int i = 0; i < size[0]; ++i){
                std::cout<<data[i];
            }

        }

    };

    template <typename FrameT, typename BlockT>
    class Frame_Fabric{

    private:

        int seq_num;

        size_t source_size;
        size_t res_size;
        size_t block_num;

        std::vector<size_t> frame_size;

        size_t q_cap;

        std::queue<std::unique_ptr<FrameT>> frame_queue;

        std::vector<int> indexes;

    public:

        Frame_Fabric(size_t source_size, size_t res_size, std::vector<size_t> frame_size, size_t q_cap):
            source_size(source_size), 
            res_size(res_size), 
            frame_size(frame_size), 
            q_cap(q_cap),  
            seq_num(0)
        {
            for(int i = 0; i < source_size; i += res_size)
                indexes.push_back(i);
        }

        int gen_frames(std::unique_ptr<BlockT[]> data)
        {
            int i = 0;
            for (; i < indexes.size(); ++i){
                if (frame_queue.size() > q_cap)
                    return -1;
                if (i == indexes.size()-1){
                    frame_queue.push(std::make_unique<FrameT>(seq_num++, frame_size, data.get()+indexes[i], source_size-indexes[indexes.size()-1]));
                }else{
                    frame_queue.push(std::make_unique<FrameT>(seq_num++, frame_size, data.get()+indexes[i], res_size));
                }
            }
            return 0;
        }

         std::unique_ptr<FrameT> get_frame(){
            if (frame_queue.empty())
                return nullptr;

            auto buf_frame = std::move(frame_queue.front());
            frame_queue.pop();

            return buf_frame;
        }

    };



}