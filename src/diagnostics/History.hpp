#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
namespace acid {
template<std::size_t Capacity> class History {
    std::array<float, Capacity> samples_{};
    std::size_t next_=0, size_=0;
public:
    void push(float value) {samples_[next_]=value;next_=(next_+1)%Capacity;size_=std::min(size_+1,Capacity);}
    void clear(){next_=size_=0;}
    std::size_t size()const{return size_;}
    float at(std::size_t index)const{return samples_[(next_+Capacity-size_+index)%Capacity];}
    float mean()const{float sum=0;for(std::size_t i=0;i<size_;++i)sum+=at(i);return size_?sum/static_cast<float>(size_):0;}
    static float plot(void* data,int index){return static_cast<History*>(data)->at(static_cast<std::size_t>(index));}
};
}
