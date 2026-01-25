
#include "input.hpp"

#include <gtest/gtest.h>

// This test verifies that xsdcpp correctly processes the ORE XSD files.
// The ORE XSDs exercise several patterns that required fixes:
// 1. Abstract elements without type attributes
// 2. Elements without type definitions (default to xs:anyType/string)

TEST(Ore, Constructor)
{
    // Verify the main types are constructible
    input::portfolio portfolio;
    input::trade trade;
}

TEST(Ore, load_portfolio)
{
    input::portfolio portfolio;
    input::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<Portfolio>
    <Trade id="trade1">
        <TradeType>Swap</TradeType>
    </Trade>
    <Trade id="trade2">
        <TradeType>FxForward</TradeType>
    </Trade>
</Portfolio>)", portfolio);

    EXPECT_EQ(portfolio.Trade.size(), 2);
    EXPECT_EQ(portfolio.Trade[0].id, "trade1");
    EXPECT_EQ(portfolio.Trade[0].TradeType, "Swap");
    EXPECT_EQ(portfolio.Trade[1].id, "trade2");
    EXPECT_EQ(portfolio.Trade[1].TradeType, "FxForward");
}

TEST(Ore, load_empty_portfolio)
{
    input::portfolio portfolio;
    input::load_data(R"(<?xml version="1.0" encoding="UTF-8"?>
<Portfolio>
</Portfolio>)", portfolio);

    EXPECT_EQ(portfolio.Trade.size(), 0);
}
