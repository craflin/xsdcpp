
#pragma once

#include "Reader.hpp"

bool generateCpp(const Xsd& xsd, const String& outputDir, const List<String>& externalNamespacePrefixes, const List<String>& forceTypeProcessing, String& error);
