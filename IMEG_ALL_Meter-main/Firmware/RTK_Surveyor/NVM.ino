/*
  For any new setting added to the settings struct, we must add it to setting file
  recording and logging, and to the WiFi AP load/read in the following places:

  recordSystemSettingsToFile(); ~REMOVED
  parseLine();
  createSettingsString(); ~REMOVED
  updateSettingWithValue();

  form.h also needs to be updated to include a space for user input. This is best
  edited in the index.html and main.js files.
*/


void recordSystemSettings()
{
    settings.sizeOfSettings = sizeof(settings); // Update to current setting size

}



// Check for extra characters in field or find minus sign.
char *skipSpace(char *str)
{
    while (isspace(*str))
        str++;
    return str;
}


// Loads a given profile name.
// Profiles may not be sequential (user might have empty profile #2, but filled #3) so we load the profile unit, not the
// number Return true if successful
bool getProfileNameFromUnit(uint8_t profileUnit, char *profileName, uint8_t profileNameLength)
{
    uint8_t located = 0;

    // Step through possible profiles looking for the specified unit
    for (int x = 0; x < MAX_PROFILE_COUNT; x++)
    {
        if (activeProfiles & (1 << x))
        {
            if (located == profileUnit)
            {
                snprintf(profileName, profileNameLength, "%s", profileNames[x]); // snprintf handles null terminator
                return (true);
            }

            located++; // Valid settingFileName but not the unit we are looking for
        }
    }
    log_d("Profile unit %d not found", profileUnit);

    return (false);
}

// Return profile number based on units
// Profiles may not be sequential (user might have empty profile #2, but filled #3) so we look up the profile unit and
// return the count
uint8_t getProfileNumberFromUnit(uint8_t profileUnit)
{
    uint8_t located = 0;

    // Step through possible profiles looking for the 1st, 2nd, 3rd, or 4th unit
    for (int x = 0; x < MAX_PROFILE_COUNT; x++)
    {
        if (activeProfiles & (1 << x))
        {
            if (located == profileUnit)
                return (x);

            located++; // Valid settingFileName but not the unit we are looking for
        }
    }
    log_d("Profile unit %d not found", profileUnit);

    return (0);
}


