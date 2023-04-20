
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
"                <DIS_Stream UID=\"1\" Name=\"Stream1\" Direction=\"In\" SampleMaxSizeBytes=\"2\" SampleMaxNumber=\"10\">"
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
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().FrameFormat.StandardRevision, standard_revision_type::A);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().ComInterface.UDP_Sockets.UDP_Socket.size(), 1);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().ComInterface.UDP_Sockets.UDP_Socket.front().Direction, direction_bidir_type::InOut);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().ComInterface.UDP_Sockets.UDP_Socket.front().DstIP, "224.1.1.1");
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().ComInterface.UDP_Sockets.UDP_Socket.front().DstPort, 2589);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().ComInterface.UDP_Sockets.UDP_Socket.front().MulticastInterfaceIP, "127.0.0.1");
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().ComInterface.UDP_Sockets.UDP_Socket.front().MulticastTTL, 1);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().ComInterface.UDP_Sockets.UDP_Socket.front().SrcIP, "");
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().ComInterface.UDP_Sockets.UDP_Socket.front().SrcPort, 1910);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Streams.DIS_Stream.size(), 2);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Streams.DIS_Stream.front().UID, 0);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Streams.DIS_Stream.front().Name, "Stream0");
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Streams.DIS_Stream.front().Direction, direction_single_type::Out);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Streams.DIS_Stream.front().SampleMaxSizeBytes, 2);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Streams.DIS_Stream.front().SampleMaxNumber, 10);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Streams.DIS_Stream.front().Signals.Signal.size(), 2);
    EXPECT_EQ(ecic.ED247ComponentInstanceConfiguration.Channels.MultiChannel.front().Streams.DIS_Stream[1].Direction, direction_single_type::In);
}
