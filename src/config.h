#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID "1"
#define WIFI_PASSWORD "1" 

// Add with other configuration
#define PARENT_PIN "1234"  // Change this to your desired PIN

// Timezone Configuration   

#define TIMEZONE_LONDON "GMT0BST,M3.5.0/1,M10.5.0"    // London
#define TIMEZONE_NEW_YORK "EST5EDT,M3.2.0/2,M11.1.0"  // New York
#define TIMEZONE_SINGAPORE "SGT-8"                   // Singapore
#define TIMEZONE_SYDNEY "AEST-10AEDT,M10.1.0/3,M4.1.0/2" // Sydney
#define TIMEZONE_INDIA "IST-5:30"                  // India Standard Time
#define TIMEZONE_MUMBAI "IST-5:30"                  // Mumbai (same as India Standard Time)

// Select your desired timezone here
#define SELECTED_TIMEZONE TIMEZONE_LONDON 

#endif // CONFIG_H