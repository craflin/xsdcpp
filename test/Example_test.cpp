
#include "ED247A_ECIC.hpp"

#include <gtest/gtest.h>

TEST(Example, Constructor)
{
    ED247A_ECIC ecic;
    ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.emplace_back();
    ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel[0].Streams.A429_Stream.emplace_back();
    std::string& str = ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel[0].Streams.A429_Stream[0].Name;
}

TEST(Example, load_content)
{
    std::string xml = 
"<ED247ComponentInstanceConfiguration ComponentType=\"Virtual\" Name=\"VirtualComponent\" StandardRevision=\"A\" Identifier=\"0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"ED247A_ECIC.xsd\">"
"    <Channels>"
"        <MultiChannel Name=\"Channel0\">"
"            <FrameFormat StandardRevision=\"A\"/>"
"            <ComInterface>"
"                <UDP_Sockets>"
"                    <UDP_Socket DstIP=\"224.1.1.1\" MulticastInterfaceIP=\"127.0.0.1\" DstPort=\"2589\" SrcPort=\"1910\"/>"
"                </UDP_Sockets>"
"            </ComInterface>"
"            <Streams>"
"                <DIS_Stream UID=\"0\" Name=\"Stream0\" Direction=\"Out\" SampleMaxSizeBytes=\"2\" SampleMaxNumber=\"10\">"
"                    <Signals SamplingPeriodUs=\"10000\">"
"                        <Signal Name=\"Signal00\" ByteOffset=\"0\"/>"
"                        <Signal Name=\"Signal01\" ByteOffset=\"1\"/>"
"                    </Signals>"
"                </DIS_Stream>"
"                <DIS_Stream UID=\"1\" Name=\"Stream1\" Direction=\"Out\" SampleMaxSizeBytes=\"2\" SampleMaxNumber=\"10\">"
"                    <Signals SamplingPeriodUs=\"10000\">"
"                        <Signal Name=\"Signal10\" ByteOffset=\"0\"/>"
"                        <Signal Name=\"Signal11\" ByteOffset=\"1\"/>"
"                    </Signals>"
"                </DIS_Stream>"
"            </Streams>"
"        </MultiChannel>"
"    </Channels>"
"</ED247ComponentInstanceConfiguration>";

    ED247A_ECIC ecic;
    load_content(xml, ecic);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.size(), 1);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Name, "Channel0");
}
