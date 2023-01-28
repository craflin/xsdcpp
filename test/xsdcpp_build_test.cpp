
#include "ED247ComponentInstanceConfiguration.hpp"

int main()
{
    ED247ComponentInstanceConfiguration config;
    config.Channels.MultiChannel.emplace_back();
    config.Channels.MultiChannel[0].Streams.A429_Stream.emplace_back();
    std::string& str = config.Channels.MultiChannel[0].Streams.A429_Stream[0].Name;


    return 0;
}
