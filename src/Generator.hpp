
#pragma once

#include "Reader.hpp"

bool generateCpp(const Xsd& xsd, const String& headerOutputDir, const String& cppOutputDir, const List<String>& externalNamespacePrefixes, const List<String>& forceTypeProcessing, const String& wrapNamespace, bool noInnerNamespace, String& error);
