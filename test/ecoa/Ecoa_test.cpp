
#include "ecoa_types_2_0.hpp"
#include "ecoa_interface_2_0.hpp"
#include "ecoa_implementation_2_0.hpp"
#include "sca_1_1_cd06_subset_2_0.hpp"
#include "ecoa_deployment_2_0.hpp"
#include "ecoa_logicalsystem_2_0.hpp"
#include "ecoa_project_2_0.hpp"

#include <gtest/gtest.h>

TEST(Ecoa_types, Constructor)
{
    ecoa_types_2_0::Library library;
    library.types.array.emplace_back();
}

TEST(Ecoa_types, union)
{
    std::string data = R"(<?xml version="1.0" encoding="UTF-8"?>
<library xmlns="http://www.ecoa.technology/types-2.0">
  <types>
    <simple maxRange="8" minRange="0" name="Philosopher_id" precision="1" type="ECOA:int32" unit="Philosopher"/>
    <simple maxRange="7" minRange="0" name="Chopstick_id" type="ECOA:int32" unit="Chopstick"/>
  </types>
</library>)";
    ecoa_types_2_0::Library library;
    ecoa_types_2_0::load_data(data, library);
}

TEST(Ecoa_interface, Constructor)
{
    ecoa_interface_2_0::ServiceDefinition serviceDefinition;
}

TEST(Ecoa_implementation, Constructor)
{
    ecoa_implementation_2_0::ComponentImplementation componentImplementation;
}

TEST(Ecoa_implementation, booleanAttribute)
{
    std::string data = R"(<?xml version="1.0" encoding="UTF-8"?>
<componentImplementation xmlns="http://www.ecoa.technology/implementation-2.0" componentDefinition="Philosopher">
  <use library="Dining"/>
  <moduleType name="Philosopher_modMain_t">
    <operations>
      <requestSent name="take" isSynchronous="true" timeout="10.0">
        <input name="which" type="Dining:Chopstick_id"/>
        <input name="who" type="Dining:Philosopher_id"/>
        <output name="taken" type="ECOA:boolean8"/>
      </requestSent>
    </operations>
  </moduleType>
</componentImplementation>)";
    ecoa_implementation_2_0::ComponentImplementation componentImplementation;
    ecoa_implementation_2_0::load_data(data, componentImplementation);
}

TEST(Ecoa_implementation, sequenceMaxOccurs)
{
    std::string data = R"(<?xml version="1.0" encoding="UTF-8"?>
<componentImplementation xmlns="http://www.ecoa.technology/implementation-2.0" componentDefinition="UnitTestComponentDefinition">
  <eventLink>
    <senders>
      <moduleInstance instanceName="SenderModule1_Instance" operationName="EventSent1"/>
      <moduleInstance instanceName="SenderModule2_Instance" operationName="EventSent2"/>
    </senders>
    <receivers>
      <moduleInstance instanceName="ReceiverModuleInstance" operationName="EventReceived"/>
    </receivers>
  </eventLink>
</componentImplementation>)";
    ecoa_implementation_2_0::ComponentImplementation componentImplementation;
    ecoa_implementation_2_0::load_data(data, componentImplementation);

    EXPECT_EQ(componentImplementation.eventLink.back().senders->moduleInstance.size(), 2);
}

TEST(Ecoa_componentType, Constructor)
{
    sca_1_1_cd06_subset_2_0::ComponentType componentType;
}

TEST(Ecoa_componentType, restrictions)
{
    std::string data = R"(<?xml version="1.0" encoding="UTF-8"?>
<csa:componentType xmlns:csa="http://docs.oasis-open.org/ns/opencsa/sca/200912" xmlns:ecoa-sca="http://www.ecoa.technology/sca-extension-2.0">
  <csa:service name="Chopsticks">
    <ecoa-sca:interface syntax="svc_Chopsticks"/>
  </csa:service>
</csa:componentType>)";
    sca_1_1_cd06_subset_2_0::ComponentType componentType;
    sca_1_1_cd06_subset_2_0::load_data(data, componentType);
}

TEST(Ecoa_componentType, anyAttribute)
{
    std::string data = R"(<?xml version="1.0" encoding="UTF-8"?>
<csa:componentType xmlns:csa="http://docs.oasis-open.org/ns/opencsa/sca/200912" xmlns:ecoa-sca="http://www.ecoa.technology/sca-extension-2.0" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
  <csa:property name="ID" type="xsd:string" ecoa-sca:type="Dining:Philosopher_id" ecoa-sca:library="Dining"/>
  <csa:reference name="Chopstick">
    <ecoa-sca:interface syntax="svc_Chopsticks"/>
  </csa:reference>
</csa:componentType>)";
    sca_1_1_cd06_subset_2_0::ComponentType componentType;
    sca_1_1_cd06_subset_2_0::load_data(data, componentType);
    EXPECT_EQ(componentType.property[0].other_attributes[0].name, "ecoa-sca:type");
    EXPECT_EQ(componentType.property[0].other_attributes[0].value, "Dining:Philosopher_id");
    EXPECT_EQ(componentType.property[0].other_attributes[1].name, "ecoa-sca:library");
    EXPECT_EQ(componentType.property[0].other_attributes[1].value, "Dining");
}

TEST(Ecoa_composite, Constructor)
{
    sca_1_1_cd06_subset_2_0::Composite composite;
}

TEST(Ecoa_composite, skipProcessing)
{
    {
        sca_1_1_cd06_subset_2_0::Composite composite;
        sca_1_1_cd06_subset_2_0::load_file(FOLDER "/Milliways.impl.composite", composite);
        EXPECT_EQ(composite.component.size(), 6);
        EXPECT_EQ(composite.component[0].property[0].name, "ID");
        EXPECT_EQ(composite.component[0].property[0], "<csa:value>3</csa:value>");
    }

    {
        std::string data = R"(<csa:composite name="xy" targetNamespace="http://www.ecoa.technology/default">
    <csa:component name="Socrates">
        <csa:property name="ID"><csa:value><csa:property>3</csa:property></csa:value></csa:property>
    </csa:component>
</csa:composite>)";

        sca_1_1_cd06_subset_2_0::Composite composite;
        sca_1_1_cd06_subset_2_0::load_data(data, composite);
        EXPECT_EQ(composite.component.size(), 1);
        EXPECT_EQ(composite.component[0].property[0].name, "ID");
        EXPECT_EQ(composite.component[0].property[0], "<csa:value><csa:property>3</csa:property></csa:value>");
    }
}

TEST(Ecoa_deployment, Constructor)
{
    ecoa_deployment_2_0::Deployment deployment;
}

TEST(Ecoa_logicalsystem, Constructor)
{
    ecoa_logicalsystem_2_0::LogicalSystem logicalSystem;
}

TEST(Ecoa_project, Constructor)
{
    ecoa_project_2_0::EcoaProject project;
}
