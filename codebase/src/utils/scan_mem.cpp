#include "scan_mem.h"

/*
* Compares a pattern to a specified memory region.
    *
    * @param data The memory region to search.
    * @param pattern The pattern to search for.
    * @param mask The mask specifying which bytes in the pattern to compare.
    * @return True if the pattern was found, false otherwise.
*/
bool compare(const uint8_t* data, const uint8_t* pattern, const char* mask) {
    for (; *mask; ++mask, ++data, ++pattern) {
        if (*mask == 'x' && *data != *pattern) {
            return false;
        }
    }
    return true;
}

/*
    * Searches for a pattern in a specified memory region.
    *
    * @param startAddress The starting address of the memory region to search.
    * @param regionSize The size of the memory region to search.
    * @param pattern The pattern to search for.
    * @param mask The mask specifying which bytes in the pattern to compare.
    * @return The address of the first occurrence of the pattern, or NULL if not found.
    */
uintptr_t patternScan(uintptr_t startAddress, size_t regionSize, const uint8_t* pattern, const char* mask) {
    size_t patternLength = strlen(mask);

    for (size_t i = 0; i <= regionSize - patternLength; i++) {
        uintptr_t currentAddress = startAddress + i;
        if (compare(reinterpret_cast<const uint8_t*>(currentAddress), pattern, mask)) {
            return currentAddress;
        }
    }

    return NULL;
}

/*
* Finds complete address given a module name and offset, i.e "client.dll" + 0x1234 = 0x12345678.
* @param moduleName The name of the module to search for.
* @param offset The offset from the module base address.
* @return The final calculated address.
*/
uintptr_t resolveModuleAddress(const char* moduleName, const uintptr_t offset) {
    uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName));
	if (!moduleBase) {
        LOG("getting module base failed!");
		return NULL;
	}
	return moduleBase + offset;
}