#pragma once 
#include <Arduino.h>

class HtmlHelper {
   public:
    static String generateStatusPage(bool brief);
        static String generateChartPage();
        static String generateMenu();  // you can implement or call existing menu code
        static String generateMetadataPage();
        static String generateAdvancedPage();
    };
