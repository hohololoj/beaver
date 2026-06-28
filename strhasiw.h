#define SMALL_STRING_BORDER 64 //The border between Small string and Medium string
#define MEDIUM_STRING_BORDER 2048 //The border between Medium string and Large string
#define EXPECTED_STACK_SIZE 5096 //The expected stack size

/**
 * @defgroup StrHasIWBrute string includes search case-insensitive brute force
 * @{
*/
/**
 * @brief StrHasIWBruteS for small strings
 * 
 * Does not convert strings to lowercase in advance; converts them on the fly.
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
*/
int StrHasIWBruteS(const wchar_t* str, const wchar_t* substr);
/**
 * @brief StrHasIWBruteM for medium strings
 * 
 * Converts both strings to lowercase in advance; uses alloca for memory allocation.
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
 * 
 * @warning Does not validate the length of strings. Incorrect usage may lead to stack overflow. For long strings, use the malloc-based version StrHasIWBruteL.
*/
int StrHasIWBruteM(const wchar_t* str, const wchar_t* substr);
/**
 * @brief StrHasIWBruteL for long strings
 * 
 * Converts both strings to lowercase in advance; uses malloc for memory allocation.
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
 * @return –1 — memory allocation error (malloc failed).
*/
int StrHasIWBruteL(const wchar_t* str, const wchar_t* substr);
/**
 * @brief StrHasIWBruteA for auto-selecting the optimal search strategy
 * 
 * Automatically selects and executes the most efficient implementation based on
 * string lengths. Designed for scenarios where most inputs are small or medium,
 * but occasional large strings may appear — eliminates the need for manual
 * checks and strategy selection in the calling code.
 * 
 * Default bounds:
 * - SMALL_STRING_BORDER  = 64   (can be overridden before including the header)
 * - MEDIUM_STRING_BORDER = 2048 (can be overridden before including the header)
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
 * @return –1 — memory allocation error (malloc failed).
 */
 int StrHasIWBruteA(const wchar_t* str, const wchar_t* substr);
/** @} */

/**
 * @defgroup StrHasIWKMP string includes search case-insensitive Knuth-Morris-Pratt
 * @{
*/

/**
 * @brief StrHasIWKMPS for small strings (char prefix, on-the-fly conversion)
 * 
 * Uses char array for prefix function storage (suitable for substrings up to 255 chars).
 * Does not convert strings to lowercase in advance; converts them on the fly.
 * Memory allocated on stack via alloca.
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
 * 
 * @warning Prefix values are stored in char; for substrings longer than 255 characters,
 *          values may overflow. Use StrHasIWKMPM or StrHasIWKMPL for longer strings.
 */
int StrHasIWKMPS(const wchar_t* str, const wchar_t* substr);

/**
 * @brief StrHasIWKMPM for medium strings (uint16_t prefix, stack allocation)
 * 
 * Uses uint16_t array for prefix function storage (suitable for substrings up to 65535 chars).
 * Converts both strings to lowercase in advance for faster comparison.
 * Memory allocated on stack via alloca.
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
 * 
 * @warning Does not validate the length of strings. Incorrect usage may lead to stack overflow.
 *          For very long strings, use the malloc-based version StrHasIWKMPL.
 */
int StrHasIWKMPM(const wchar_t* str, const wchar_t* substr);

/**
 * @brief StrHasIWKMPL for long strings (uint16_t prefix, heap allocation)
 * 
 * Uses uint16_t array for prefix function storage (suitable for substrings up to 65535 chars).
 * Converts both strings to lowercase in advance.
 * Memory allocated on heap via malloc.
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
 * @return –1 — memory allocation error (malloc failed).
 * 
 * @warning For substrings longer than 65535 characters, prefix values may overflow.
 *          Use StrHasIWKMPXL for extremely long substrings.
 */
int StrHasIWKMPL(const wchar_t* str, const wchar_t* substr);

/**
 * @brief StrHasIWKMPXL for extra long strings (int prefix, heap allocation)
 * 
 * Uses int array for prefix function storage (suitable for substrings of any length).
 * Converts both strings to lowercase in advance.
 * Memory allocated on heap via malloc.
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
 * @return –1 — memory allocation error (malloc failed).
 */
int StrHasIWKMPXL(const wchar_t* str, const wchar_t* substr);

/**
 * @brief StrHasIWKMPA for auto-selecting the optimal KMP search strategy
 * 
 * Automatically selects and executes the most efficient implementation based on
 * string lengths. Designed for scenarios where most inputs are small or medium,
 * but occasional large strings may appear — eliminates the need for manual
 * checks and strategy selection in the calling code.
 * 
 * Default bounds (can be overridden before including the header):
 * - SMALL_STRING_BORDER   = 64    (maximum length for char prefix)
 * - MEDIUM_STRING_BORDER  = 2048  (threshold for heap allocation)
 * - EXPECTED_STACK_SIZE   = 5096  (estimated stack size for safe alloca)
 * 
 * @param str      Original string.
 * @param substr   Search string. If its length exceeds that of the original string, returns 0.
 * 
 * @return 1 — substring found.
 * @return 0 — substring not found or cannot be found.
 * @return –1 — memory allocation error (malloc failed).
 */
int StrHasIWKMPA(const wchar_t* str, const wchar_t* substr);

/** @} */