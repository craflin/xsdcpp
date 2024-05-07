
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

TEST(Ecoa_interface, Constructor)
{
    ecoa_interface_2_0::ServiceDefinition serviceDefinition;
}

TEST(Ecoa_implementation, Constructor)
{
    ecoa_implementation_2_0::ComponentImplementation componentImplementation;
}

TEST(Ecoa_componentType, Constructor)
{
    sca_1_1_cd06_subset_2_0::sca_ComponentType componentType;
}

TEST(Ecoa_composite, Constructor)
{
    sca_1_1_cd06_subset_2_0::sca_Composite composite;
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