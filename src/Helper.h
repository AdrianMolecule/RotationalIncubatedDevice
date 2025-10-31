#pragma once
#include <vector>
#include <String.h>
#include "Field.h"

class Helper {
public:
    // Return the default set of fields
    static std::vector<Field> defaultFields();

    // Generate the HTML menu/navigation
    static String generateMenu();

    // Generate HTML page header
    static String htmlHeader(const String& title);

    // Generate HTML page footer
    static String htmlFooter();
};
