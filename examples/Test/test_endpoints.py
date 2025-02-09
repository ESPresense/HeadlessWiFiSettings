#!/usr/bin/env python3
import requests
import json
import time
import sys

def print_json(data):
    print(json.dumps(data, indent=2))

def test_endpoints(base_url):
    print(f"\nTesting HeadlessWiFiSettings endpoints at {base_url}\n")

    # Test GET /settings
    print("GET /settings")
    try:
        response = requests.get(f"{base_url}/settings")
        response.raise_for_status()
        print("Response:")
        print_json(response.json())
    except Exception as e:
        print(f"Error: {e}")

    # Test POST /settings
    print("\nPOST /settings")
    try:
        data = {
            "test_string": "updated value",
            "test_int": "50",
            "test_bool": "1"
        }
        response = requests.post(f"{base_url}/settings", data=data)
        response.raise_for_status()
        print("Settings updated successfully")
    except Exception as e:
        print(f"Error: {e}")

    # Verify updated settings
    print("\nVerifying updated settings:")
    try:
        response = requests.get(f"{base_url}/settings")
        response.raise_for_status()
        print_json(response.json())
    except Exception as e:
        print(f"Error: {e}")

    # Test GET /extras
    print("\nGET /extras")
    try:
        response = requests.get(f"{base_url}/extras")
        response.raise_for_status()
        print("Response:")
        print_json(response.json())
    except Exception as e:
        print(f"Error: {e}")

    # Test POST /extras
    print("\nPOST /extras")
    try:
        data = {
            "test_float": "6.28",
            "test_extra": "updated extra"
        }
        response = requests.post(f"{base_url}/extras", data=data)
        response.raise_for_status()
        print("Extra settings updated successfully")
    except Exception as e:
        print(f"Error: {e}")

    # Verify updated extras
    print("\nVerifying updated extras:")
    try:
        response = requests.get(f"{base_url}/extras")
        response.raise_for_status()
        print_json(response.json())
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python test_endpoints.py <esp_ip>")
        print("Example: python test_endpoints.py 192.168.4.1")
        sys.exit(1)

    base_url = f"http://{sys.argv[1]}"
    test_endpoints(base_url)

"""
Example output:

Testing HeadlessWiFiSettings endpoints at http://192.168.4.1

GET /settings
Response:
{
  "test_string": "default value",
  "test_int": "42",
  "test_bool": "true"
}

POST /settings
Settings updated successfully

Verifying updated settings:
{
  "test_string": "updated value",
  "test_int": "50",
  "test_bool": "true"
}

GET /extras
Response:
{
  "test_float": "3.14",
  "test_extra": "extra value"
}

POST /extras
Extra settings updated successfully

Verifying updated extras:
{
  "test_float": "6.28",
  "test_extra": "updated extra"
}
"""