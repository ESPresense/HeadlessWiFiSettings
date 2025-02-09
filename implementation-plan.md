# HeadlessWiFiSettings JSON Endpoint Organization Plan

## Overview
Reorganize settings to allow for multiple named JSON endpoints while maintaining backward compatibility with existing usage, optimized for ESP32.

## Current Implementation
- Uses two vectors: `primary` and `extras` for storing settings
- `markExtra()` toggles which vector new parameters are added to
- Endpoints:
  * `/settings` - serves primary parameters
  * `/extras` - serves extra parameters

## Memory-Efficient Design

### 1. Data Structure Changes
- Replace current system with parallel vectors:
```cpp
// Parallel vectors for endpoint names and their parameters
std::vector<String> endpointNames;  // Names of endpoints
std::vector<std::vector<HeadlessWiFiSettingsParameter*>> endpointParams;  // Parameters for each endpoint

// Initialize with only main endpoint
endpointNames.push_back("main");     // Index 0 is always main
endpointParams.push_back({});        // Empty vector for main params

uint8_t currentEndpointIndex = 0;    // Default to main
```

### 2. API Changes
- New method: `markEndpoint(const String& name)`
  * Sets which endpoint new parameters are added to
  * Default endpoint is "main"
  * Creates new endpoint if name doesn't exist
- Update `markExtra()`
  * Creates "extra" endpoint if it doesn't exist
  * Sets currentEndpointIndex to the "extra" endpoint
  * Maintains backward compatibility by creating endpoint on demand

### 3. HTTP Endpoint Changes
- New endpoint handler that checks against endpointNames vector
- Maintain old endpoints for compatibility:
  * `/settings` -> serves index 0 (main)
  * `/extras` -> serves "extra" endpoint if it exists
- New endpoint format: `/settings/{name}`
  * Lookup name in endpointNames vector
  * Serve corresponding parameters from endpointParams

### 4. Memory Optimizations
- Only "main" endpoint created by default
- "extra" endpoint created only when markExtra() is called
- Additional endpoints created only when markEndpoint() is called
- Vectors only grow when new endpoints are actually needed
- Reuse existing parameter vectors
- Parallel vectors allow for simple indexing

### 5. Implementation Steps
1. Add parallel vectors for names and parameters
2. Initialize only with "main" endpoint
3. Implement `markEndpoint()` with endpoint creation/lookup
4. Update `markExtra()` to create "extra" endpoint on demand
5. Update parameter storage logic to use endpoint vectors
6. Add new HTTP handler with endpoint name lookup
7. Update documentation and examples

### 6. Migration Path
- Existing code using primary parameters continues working through index 0
- Code using markExtra() continues working by creating "extra" endpoint when needed
- New code can use markEndpoint() to create endpoints as needed

## Example Usage

Old code (continues working):
```cpp
HeadlessWiFiSettings.string("ssid", "");
HeadlessWiFiSettings.markExtra();  // Creates "extra" endpoint if needed
HeadlessWiFiSettings.string("api_key", "");
```

New code:
```cpp
HeadlessWiFiSettings.string("ssid", "");  // Goes to /settings/main
HeadlessWiFiSettings.markEndpoint("network");
HeadlessWiFiSettings.string("dns", "");   // Goes to /settings/network
HeadlessWiFiSettings.markEndpoint("api");
HeadlessWiFiSettings.string("api_key", ""); // Goes to /settings/api
```

## Memory Impact
- Only creates endpoints when they're actually needed
- "main" endpoint created by default
- "extra" endpoint created only if markExtra() is called
- Additional endpoints created only when explicitly requested
- Simple parallel vector structure
- Memory grows proportionally with number of distinct endpoints actually used